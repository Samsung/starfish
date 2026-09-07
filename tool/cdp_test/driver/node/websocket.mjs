// Minimal RFC 6455 client over a plain TCP socket.
//
// Written here rather than taken from npm, and not using Node's own global
// WebSocket, so that the suite needs nothing but a Node binary and runs on
// the Node that Ubuntu ships. The global WebSocket only became available
// unflagged in Node 21.
//
// Implements only what CDP over a loopback connection needs: version 13,
// "ws://" with no TLS, no extensions, text frames. Not a general-purpose
// WebSocket library.

import crypto from 'node:crypto';
import net from 'node:net';

// RFC 6455 section 1.3. The server appends this to our key and hashes it, so
// a proxy that merely echoes the request cannot pass as a WebSocket server.
const HANDSHAKE_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';

const OPCODE_CONTINUATION = 0x0;
const OPCODE_TEXT = 0x1;
const OPCODE_CLOSE = 0x8;
const OPCODE_PING = 0x9;
const OPCODE_PONG = 0xa;

function applyMask(payload, mask) {
  const out = Buffer.allocUnsafe(payload.length);
  for (let i = 0; i < payload.length; i++) {
    out[i] = payload[i] ^ mask[i & 3];
  }
  return out;
}

export class WebSocketClient {
  constructor(onMessage, onError) {
    this._onMessage = onMessage;
    this._onError = onError;
    this._buffer = Buffer.alloc(0);
    this._fragments = [];
    this._handshakeDone = false;
    this._closed = false;
  }

  connect(url, timeoutMs) {
    const parsed = new URL(url);
    if (parsed.protocol !== 'ws:') {
      return Promise.reject(new Error(`only ws:// is supported, got ${url}`));
    }
    const key = crypto.randomBytes(16).toString('base64');
    this._expectedAccept = crypto
      .createHash('sha1').update(key + HANDSHAKE_GUID).digest('base64');

    return new Promise((resolve, reject) => {
      const timer = setTimeout(
        () => reject(new Error('connect timed out')), timeoutMs);
      this._resolveHandshake = () => { clearTimeout(timer); resolve(this); };
      this._rejectHandshake = (error) => { clearTimeout(timer); reject(error); };

      this._socket = net.createConnection({
        host: parsed.hostname,
        port: Number(parsed.port) || 80,
      }, () => {
        const path = (parsed.pathname || '/') + (parsed.search || '');
        this._socket.write(
          `GET ${path} HTTP/1.1\r\n` +
          `Host: ${parsed.hostname}:${parsed.port}\r\n` +
          'Upgrade: websocket\r\n' +
          'Connection: Upgrade\r\n' +
          `Sec-WebSocket-Key: ${key}\r\n` +
          'Sec-WebSocket-Version: 13\r\n' +
          '\r\n');
      });
      this._socket.on('data', (chunk) => this._receive(chunk));
      this._socket.on('error', (error) => {
        if (!this._handshakeDone) this._rejectHandshake(error);
        else this._onError(error);
      });
      this._socket.on('close', () => {
        if (!this._handshakeDone) {
          this._rejectHandshake(new Error('closed during handshake'));
        }
      });
    });
  }

  send(text) {
    if (this._closed) return;
    this._socket.write(this._frame(OPCODE_TEXT, Buffer.from(text, 'utf8')));
  }

  close() {
    if (this._closed) return;
    this._closed = true;
    try {
      const payload = Buffer.alloc(2);
      payload.writeUInt16BE(1000, 0);
      this._socket.write(this._frame(OPCODE_CLOSE, payload));
    } catch { /* the browser may already be gone */ }
    this._socket.destroy();
  }

  _frame(opcode, payload) {
    // A client must always mask, so the length byte carries the mask bit.
    const mask = crypto.randomBytes(4);
    const length = payload.length;
    let header;
    if (length < 126) {
      header = Buffer.from([0x80 | opcode, 0x80 | length]);
    } else if (length < 0x10000) {
      header = Buffer.alloc(4);
      header[0] = 0x80 | opcode;
      header[1] = 0x80 | 126;
      header.writeUInt16BE(length, 2);
    } else {
      header = Buffer.alloc(10);
      header[0] = 0x80 | opcode;
      header[1] = 0x80 | 127;
      header.writeBigUInt64BE(BigInt(length), 2);
    }
    return Buffer.concat([header, mask, applyMask(payload, mask)]);
  }

  _receive(chunk) {
    this._buffer = Buffer.concat([this._buffer, chunk]);
    if (!this._handshakeDone && !this._finishHandshake()) return;
    this._drainFrames();
  }

  _finishHandshake() {
    const end = this._buffer.indexOf('\r\n\r\n');
    if (end === -1) return false;
    const head = this._buffer.subarray(0, end).toString('latin1');
    // Bytes past the header already belong to the frame stream.
    this._buffer = this._buffer.subarray(end + 4);
    const lines = head.split('\r\n');
    if (!lines[0].includes('101')) {
      this._rejectHandshake(new Error(`upgrade refused: ${lines[0]}`));
      return false;
    }
    const accepted = lines.slice(1)
      .map((line) => line.split(':'))
      .filter(([name]) => name.trim().toLowerCase() === 'sec-websocket-accept')
      .map(([, ...rest]) => rest.join(':').trim())[0];
    if (accepted !== this._expectedAccept) {
      this._rejectHandshake(new Error('accept key mismatch'));
      return false;
    }
    this._handshakeDone = true;
    this._resolveHandshake();
    return true;
  }

  _drainFrames() {
    for (;;) {
      const frame = this._readFrame();
      if (!frame) return;
      const { final, opcode, payload } = frame;
      if (opcode === OPCODE_PING) {
        this._socket.write(this._frame(OPCODE_PONG, payload));
        continue;
      }
      if (opcode === OPCODE_PONG) continue;
      if (opcode === OPCODE_CLOSE) {
        this._closed = true;
        this._socket.destroy();
        return;
      }
      if (opcode !== OPCODE_CONTINUATION) this._messageOpcode = opcode;
      this._fragments.push(payload);
      if (!final) continue;
      const message = Buffer.concat(this._fragments);
      this._fragments = [];
      if (this._messageOpcode === OPCODE_TEXT) {
        this._onMessage(message.toString('utf8'));
      }
    }
  }

  _readFrame() {
    const buffer = this._buffer;
    if (buffer.length < 2) return null;
    const final = (buffer[0] & 0x80) !== 0;
    const opcode = buffer[0] & 0x0f;
    const masked = (buffer[1] & 0x80) !== 0;
    let length = buffer[1] & 0x7f;
    let offset = 2;
    if (length === 126) {
      if (buffer.length < offset + 2) return null;
      length = buffer.readUInt16BE(offset);
      offset += 2;
    } else if (length === 127) {
      if (buffer.length < offset + 8) return null;
      length = Number(buffer.readBigUInt64BE(offset));
      offset += 8;
    }
    // A server must not mask, but unmask anyway rather than hand back
    // scrambled bytes if one does.
    let mask = null;
    if (masked) {
      if (buffer.length < offset + 4) return null;
      mask = buffer.subarray(offset, offset + 4);
      offset += 4;
    }
    if (buffer.length < offset + length) return null;
    let payload = buffer.subarray(offset, offset + length);
    if (mask) payload = applyMask(payload, mask);
    this._buffer = buffer.subarray(offset + length);
    return { final, opcode, payload };
  }
}
