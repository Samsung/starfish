#!/usr/bin/env python3

"""Minimal external CDP client: HTTP discovery plus a raw WebSocket.

This module is deliberately the only place that talks to a browser. It must
not import anything from Starfish, so the suite stays a black-box test.

The WebSocket client is written here rather than taken from PyPI so that CI
needs no package install and cannot fail on a network problem. It implements
only what CDP over a loopback connection needs: RFC 6455 version 13, "ws://"
with no TLS, no extensions, and text frames. It is not a general-purpose
WebSocket library.
"""

import base64
import hashlib
import json
import os
import socket
import struct
import urllib.parse
import urllib.request

# RFC 6455 section 1.3. The server appends this to our key and hashes it, so
# a proxy that merely echoes the request cannot pass as a WebSocket server.
HANDSHAKE_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

OPCODE_CONTINUATION = 0x0
OPCODE_TEXT = 0x1
OPCODE_BINARY = 0x2
OPCODE_CLOSE = 0x8
OPCODE_PING = 0x9
OPCODE_PONG = 0xA


def get_json(endpoint, path):
    url = urllib.parse.urljoin(endpoint.rstrip("/") + "/", path.lstrip("/"))
    with urllib.request.urlopen(url, timeout=10) as response:
        return json.load(response)


def _apply_mask(payload, mask):
    """XOR a payload with a repeating 4-byte mask.

    Done as one big-integer XOR because a per-byte Python loop is slow on the
    multi-megabyte responses that commands such as Accessibility.getFullAXTree
    return.
    """
    if not payload:
        return payload
    repeated = (mask * (len(payload) // 4 + 1))[:len(payload)]
    masked = (int.from_bytes(payload, "big") ^
              int.from_bytes(repeated, "big"))
    return masked.to_bytes(len(payload), "big")


class WebSocket:
    """A client connection carrying CDP JSON messages."""

    def __init__(self, connection):
        self._connection = connection
        # Bytes read from the socket but not yet consumed. The handshake read
        # can overrun into the first frame, so this must survive that step.
        self._buffer = bytearray()

    def send(self, text):
        self._send_frame(OPCODE_TEXT, text.encode())

    def recv(self):
        """Return the next text message, reassembling fragmented frames."""
        message = bytearray()
        message_opcode = None
        while True:
            final, opcode, payload = self._read_frame()
            if opcode == OPCODE_PING:
                self._send_frame(OPCODE_PONG, payload)
                continue
            if opcode == OPCODE_PONG:
                continue
            if opcode == OPCODE_CLOSE:
                raise OSError("the browser closed the WebSocket")
            if opcode != OPCODE_CONTINUATION:
                message_opcode = opcode
            message += payload
            if final:
                break
        if message_opcode != OPCODE_TEXT:
            raise ValueError("expected a text frame, got opcode %s" %
                             message_opcode)
        return message.decode()

    def close(self):
        try:
            self._send_frame(OPCODE_CLOSE, struct.pack(">H", 1000))
        except OSError:
            # The browser may already be gone; closing the socket is enough.
            pass
        self._connection.close()

    def _read_exactly(self, count):
        while len(self._buffer) < count:
            chunk = self._connection.recv(65536)
            if not chunk:
                raise OSError("the WebSocket closed after %d of %d bytes" %
                              (len(self._buffer), count))
            self._buffer += chunk
        data = bytes(self._buffer[:count])
        del self._buffer[:count]
        return data

    def _read_frame(self):
        header = self._read_exactly(2)
        final = bool(header[0] & 0x80)
        opcode = header[0] & 0x0F
        masked = bool(header[1] & 0x80)
        length = header[1] & 0x7F
        if length == 126:
            length = struct.unpack(">H", self._read_exactly(2))[0]
        elif length == 127:
            length = struct.unpack(">Q", self._read_exactly(8))[0]
        # A server must not mask, but unmask anyway rather than return
        # scrambled bytes if one does.
        mask = self._read_exactly(4) if masked else b""
        payload = self._read_exactly(length)
        return final, opcode, _apply_mask(payload, mask) if masked else payload

    def _send_frame(self, opcode, payload):
        header = bytearray([0x80 | opcode])
        mask = os.urandom(4)
        length = len(payload)
        # A client must always mask, so the length byte carries the mask bit.
        if length < 126:
            header.append(0x80 | length)
        elif length < 1 << 16:
            header.append(0x80 | 126)
            header += struct.pack(">H", length)
        else:
            header.append(0x80 | 127)
            header += struct.pack(">Q", length)
        header += mask
        self._connection.sendall(bytes(header) + _apply_mask(payload, mask))

    def _finish_handshake(self, key):
        head = self._read_until(b"\r\n\r\n").decode("latin-1")
        lines = head.split("\r\n")
        if "101" not in lines[0]:
            raise OSError("WebSocket upgrade refused: %s" % lines[0])
        accepted = None
        for line in lines[1:]:
            name, _, value = line.partition(":")
            if name.strip().lower() == "sec-websocket-accept":
                accepted = value.strip()
        expected = base64.b64encode(
            hashlib.sha1((key + HANDSHAKE_GUID).encode()).digest()).decode()
        if accepted != expected:
            raise OSError("WebSocket accept key mismatch")

    def _read_until(self, marker):
        while marker not in self._buffer:
            chunk = self._connection.recv(4096)
            if not chunk:
                raise OSError("the connection closed during the handshake")
            self._buffer += chunk
        index = self._buffer.index(marker)
        head = bytes(self._buffer[:index])
        del self._buffer[:index + len(marker)]
        return head


def connect(url, timeout):
    """Open a WebSocket to a "ws://" URL from /json/list."""
    parts = urllib.parse.urlsplit(url)
    if parts.scheme != "ws":
        raise ValueError("only ws:// is supported, got %s" % url)
    path = parts.path or "/"
    if parts.query:
        path += "?" + parts.query
    port = parts.port or 80
    key = base64.b64encode(os.urandom(16)).decode()
    request = (
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n" % (path, parts.hostname, port, key)
    )
    connection = socket.create_connection((parts.hostname, port),
                                          timeout=timeout)
    socket_wrapper = WebSocket(connection)
    try:
        connection.sendall(request.encode())
        socket_wrapper._finish_handshake(key)
    except Exception:
        connection.close()
        raise
    return socket_wrapper


def call(socket_wrapper, request_id, method, params=None):
    request = {"id": request_id, "method": method}
    if params is not None:
        request["params"] = params
    socket_wrapper.send(json.dumps(request))
    # Responses interleave with protocol events, so skip anything that is not
    # the reply to this request id.
    while True:
        response = json.loads(socket_wrapper.recv())
        if response.get("id") == request_id:
            return response


def debugger_url(endpoint):
    targets = get_json(endpoint, "/json/list")
    if not isinstance(targets, list) or not targets:
        raise ValueError("/json/list did not return a page target")
    url = targets[0].get("webSocketDebuggerUrl")
    if not url:
        raise ValueError("the first target has no webSocketDebuggerUrl")
    return url


def probe_transport(endpoint, timeout):
    """Check that a browser is up and speaking CDP.

    The runner calls this in a retry loop, so every reachability problem must
    surface as OSError or ValueError rather than a partial result.
    """
    version = get_json(endpoint, "/json/version")
    socket_wrapper = connect(debugger_url(endpoint), timeout)
    try:
        schema = call(socket_wrapper, 1, "Schema.getDomains")
    finally:
        socket_wrapper.close()
    if not isinstance(schema.get("result", {}).get("domains"), list):
        raise ValueError("Schema.getDomains did not return a domains list")
    return {
        "browser_version": version,
        "schema_get_domains": schema,
    }
