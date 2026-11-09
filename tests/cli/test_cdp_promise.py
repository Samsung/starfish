import base64
import json
import os
import socket
import struct
import subprocess
import time
import urllib.parse
import urllib.request

from driver.support import CLI_BINARY, ENGINE_BINARY_NAME, CLITestCase

ENGINE_BINARY = CLI_BINARY.parent / ENGINE_BINARY_NAME


def free_port():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


class CDPClient:
    """A minimal WebSocket CDP client, so the test needs no extra package."""

    def __init__(self, port, path, timeout=10):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=timeout)
        key = base64.b64encode(os.urandom(16)).decode()
        self.sock.sendall(
            ("GET %s HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nUpgrade: websocket\r\n"
             "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n\r\n" % (path, port, key)).encode())
        response = b""
        while b"\r\n\r\n" not in response:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("upgrade refused")
            response += chunk
        if b" 101 " not in response.split(b"\r\n", 1)[0]:
            raise ConnectionError("upgrade refused")
        self.buffer = response.split(b"\r\n\r\n", 1)[1]
        self.next_id = 0

    def close(self):
        self.sock.close()

    def send(self, method, **params):
        self.next_id += 1
        payload = json.dumps(
            {"id": self.next_id, "method": method, "params": params}).encode()
        mask = os.urandom(4)
        header = bytes([0x81])
        if len(payload) < 126:
            header += bytes([0x80 | len(payload)])
        else:
            header += bytes([0x80 | 126]) + struct.pack(">H", len(payload))
        masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
        self.sock.sendall(header + mask + masked)
        return self.next_id

    def _read(self, count):
        while len(self.buffer) < count:
            chunk = self.sock.recv(65536)
            if not chunk:
                raise ConnectionError("closed")
            self.buffer += chunk
        data, self.buffer = self.buffer[:count], self.buffer[count:]
        return data

    def receive(self):
        """Return the next JSON message, or raise socket.timeout."""
        message = b""
        while True:
            first, second = self._read(2)
            length = second & 0x7F
            if length == 126:
                length = struct.unpack(">H", self._read(2))[0]
            elif length == 127:
                length = struct.unpack(">Q", self._read(8))[0]
            payload = self._read(length)
            if first & 0x0F in (1, 0):
                message += payload
                if first & 0x80:
                    return json.loads(message)

    def reply(self, command_id, timeout=10):
        """Wait for the reply to `command_id`, skipping events."""
        self.sock.settimeout(timeout)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            message = self.receive()
            if message.get("id") == command_id:
                return message
        raise socket.timeout()

    def call(self, method, timeout=10, **params):
        return self.reply(self.send(method, **params), timeout)


class CDPPromiseTest(CLITestCase):
    # Runtime.evaluate with awaitPromise must answer when a Promise settles
    # later (timer), without blocking the connection while it is pending.
    # The expected replies are what Chrome 155.0.8059.79 sends.

    def setUp(self):
        super().setUp()
        self.port = free_port()
        environment = dict(self.environment)
        environment["STARFISH_ENABLE_CDP"] = "1"
        environment["STARFISH_CDP_PORT"] = str(self.port)
        # The harness removes DISPLAY, so the engine must not open a window
        # (tool/cli_test/README.md); --headless keeps it off the display.
        self.engine_log_path = os.path.join(self.root, "engine.log")
        with open(self.engine_log_path, "wb") as log:
            self.engine = subprocess.Popen(
                [str(ENGINE_BINARY), "about:blank", "--disable-console",
                 "--headless"],
                env=environment,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
                stderr=log,
            )
        self.addCleanup(self.stop_engine)
        self.client = self.connect()
        self.addCleanup(self.client.close)

    def engine_log(self):
        # The engine's stderr, so a failed connection reports why it stopped.
        with open(self.engine_log_path, "rb") as log:
            return log.read().decode(errors="replace")[-2000:]

    def stop_engine(self):
        self.engine.terminate()
        try:
            self.engine.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.engine.kill()
            self.engine.wait()

    def connect(self):
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            try:
                return CDPClient(self.port, self.page_path())
            except (OSError, ConnectionError, ValueError):
                time.sleep(0.1)
        self.fail("the engine did not accept a CDP connection\n%s"
                  % self.engine_log())

    def page_path(self):
        # Connect to the page target, as a client of Chrome would.
        with urllib.request.urlopen(
                "http://127.0.0.1:%d/json/list" % self.port, timeout=2) as reply:
            targets = json.load(reply)
        pages = [target for target in targets if target.get("type") == "page"]
        if not pages:
            raise ValueError("no page target")
        return urllib.parse.urlsplit(pages[0]["webSocketDebuggerUrl"]).path

    def evaluate(self, expression, **extra):
        return self.client.send(
            "Runtime.evaluate", expression=expression, awaitPromise=True,
            returnByValue=True, **extra)

    def test_timer_promise_resolves_and_rejects(self):
        resolved = self.client.call(
            "Runtime.evaluate",
            expression="new Promise(r => setTimeout(() => r(42), 100))",
            awaitPromise=True, returnByValue=True)
        self.assertEqual(resolved["result"]["result"]["value"], 42)

        rejected = self.client.call(
            "Runtime.evaluate",
            expression="new Promise((_, r) => setTimeout(() => r('boom'), 100))",
            awaitPromise=True, returnByValue=True)
        self.assertIn("exceptionDetails", rejected["result"])
        self.assertEqual(rejected["result"]["result"]["value"], "boom")

    def test_never_settled_promise_does_not_block_the_connection(self):
        pending = self.evaluate("new Promise(() => {})")
        later = self.client.call(
            "Runtime.evaluate", timeout=3, expression="1 + 1")
        self.assertEqual(later["result"]["result"]["value"], 2)

        # No reply may arrive for the pending command.
        self.client.sock.settimeout(0.5)
        with self.assertRaises(socket.timeout):
            while True:
                self.assertNotEqual(self.client.receive().get("id"), pending)

    def test_navigation_fails_a_pending_evaluation(self):
        # Both a CDP navigation and one the page starts by itself end the
        # document that owns the Promise.
        navigations = [
            ("Page.navigate", {"url": self.resource_url("click-counter.html")}),
            ("Runtime.evaluate", {
                "expression": "location.href = %r"
                % self.resource_url("fill-input.html")}),
        ]
        for method, params in navigations:
            with self.subTest(method=method):
                pending = self.evaluate("new Promise(() => {})")
                self.client.send(method, **params)
                reply = self.client.reply(pending, timeout=15)
                self.assertEqual(reply["error"]["code"], -32000)
                self.assertEqual(
                    reply["error"]["message"],
                    "Inspected target navigated or closed")

    def test_reconnect_never_receives_the_old_clients_reply(self):
        # Client A leaves a slow Promise pending and disconnects. Client B
        # starts a later command with the same id. Only B's own reply may
        # arrive on B's connection.
        self.evaluate("new Promise(r => setTimeout(() => r('old'), 500))")
        self.client.close()

        client = self.connect()
        self.addCleanup(client.close)
        marker = client.call(
            "Runtime.evaluate", expression="'new'", returnByValue=True)
        self.assertEqual(marker["result"]["result"]["value"], "new")

        time.sleep(1)
        client.sock.settimeout(0.5)
        with self.assertRaises(socket.timeout):
            while True:
                message = client.receive()
                self.assertNotIn("old", json.dumps(message))
