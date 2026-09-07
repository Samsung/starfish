// Run one Chromium inspector-protocol test and print its log to stdout.
//
// Chromium's inspector-protocol-test.js runs unmodified. It already takes
// log, completeTest and fetch as constructor arguments, and sends every
// outbound protocol message through DevToolsHost.sendMessageToEmbedder, so
// that function plus those three callbacks is the whole surface to supply.
// Its log formatting, which every -expected.txt is diffed against, is
// therefore Chromium's own.
//
// Usage: node driver/node/main.mjs <ws-url> <suite-dir> <test.js> [timeout]

import { readFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import vm from 'node:vm';

import { WebSocketClient } from './websocket.mjs';

const [wsUrl, suiteDir, testPath, timeoutArg] = process.argv.slice(2);
if (!wsUrl || !suiteDir || !testPath) {
  console.error(
    'usage: main.mjs <ws-url> <suite-dir> <test.js> [timeout-seconds]');
  process.exit(2);
}
const timeoutMs = (Number(timeoutArg) || 30) * 1000;

// The suite, including the harness Chromium ships with it, is checked out in
// the test submodule rather than beside this file.
const VENDORED = resolve(suiteDir, 'resources/inspector-protocol-test.js');

const output = [];
let finish;
const finished = new Promise((resolve_) => { finish = resolve_; });
let socket;

// In content_shell a test script runs in a real browser context, so it may
// reach for any web platform global. Node has most of them, and leaving one
// out fails the test here before it sends a single CDP message. Pass through
// everything Node already provides rather than guessing what tests use.
const WEB_GLOBALS = [
  'atob', 'btoa', 'AbortController', 'AbortSignal', 'Blob', 'ByteLengthQueuingStrategy',
  'CompressionStream', 'CountQueuingStrategy', 'crypto', 'Crypto', 'CryptoKey',
  'CustomEvent', 'DecompressionStream', 'DOMException', 'Event', 'EventTarget',
  'fetch', 'File', 'FormData', 'Headers', 'MessageChannel', 'MessageEvent',
  'MessagePort', 'performance', 'ReadableStream', 'Request', 'Response',
  'structuredClone', 'SubtleCrypto', 'TextDecoderStream', 'TextEncoderStream',
  'TransformStream', 'WebSocket', 'WritableStream',
];

// The suite is written for Chromium's web test server, and a test that builds
// an absolute URL takes the origin from here.
const TEST_ORIGIN = 'http://127.0.0.1:8000';

const sandbox = {
  console,
  setTimeout, clearTimeout, setInterval, clearInterval, queueMicrotask,
  URL, URLSearchParams, TextEncoder, TextDecoder,
  DevToolsHost: {
    // content_shell points this at inspector-protocol-page.html. There is no
    // such page here, and a blank one is what startBlank means anyway.
    dummyPageURL: 'about:blank',
    sendMessageToEmbedder(json) {
      const message = JSON.parse(json);
      if (message.method === 'dispatchProtocolMessage') {
        socket.send(message.params[0]);
      }
      // Other embedder messages, such as setAllowUnsafeOperations, have no
      // meaning outside content_shell.
    },
  },
  // Load-time stubs. The DevToolsAPI statics that would touch document or
  // XMLHttpRequest are all replaced below, so these need only exist.
  window: { addEventListener() {}, location: { search: '', href: '' } },
  self: { origin: TEST_ORIGIN },
  document: {},
  location: { search: '', href: '', origin: TEST_ORIGIN },
};
for (const name of WEB_GLOBALS) {
  if (name in globalThis) {
    sandbox[name] = globalThis[name];
  }
}
sandbox.globalThis = sandbox;
vm.createContext(sandbox);

// Append an epilogue instead of editing the vendored file: a top-level class
// declaration is not a property of the context, so it is otherwise
// unreachable from here.
const source = readFileSync(VENDORED, 'utf8')
  + '\nglobalThis.__harness = { TestRunner, DevToolsAPI };\n';
vm.runInContext(source, sandbox, { filename: VENDORED });
const { TestRunner, DevToolsAPI } = sandbox.__harness;

DevToolsAPI._log = (text) => { output.push(String(text)); };
DevToolsAPI._completeTest = () => { finish(); };
DevToolsAPI._fetch = async (url) => {
  const path = url.startsWith('file://') ? fileURLToPath(url) : url;
  return readFileSync(path, 'utf8');
};

function report(exitCode) {
  process.stdout.write(output.join('\n') + '\n');
  process.exit(exitCode);
}

socket = new WebSocketClient(
  (text) => {
    try {
      DevToolsAPI.dispatchMessage(text);
    } catch (error) {
      output.push(`Error dispatching message: ${error}`);
      finish();
    }
  },
  (error) => {
    output.push(`WebSocket error: ${error.message}`);
    finish();
  });

try {
  await socket.connect(wsUrl, timeoutMs);
} catch (error) {
  output.push(`Could not connect to ${wsUrl}: ${error.message}`);
  report(1);
}

const testBaseURL = pathToFileURL(dirname(resolve(testPath))).href + '/';
const runner = new TestRunner(
  testBaseURL, testBaseURL,
  DevToolsAPI._log, DevToolsAPI._completeTest, DevToolsAPI._fetch,
  new URLSearchParams(''));

// A test file is already a parenthesized function expression, sometimes with
// a trailing semicolon. Evaluate it as is, exactly as content_shell does, and
// take the completion value. Adding another pair of parentheses here breaks
// every file that ends in ");".
const testFunction = vm.runInContext(
  readFileSync(testPath, 'utf8'), sandbox, { filename: testPath });
if (typeof testFunction !== 'function') {
  output.push(`Test script did not evaluate to a function: ${testPath}`);
  report(1);
}

const timer = setTimeout(() => {
  output.push('HARNESS TIMEOUT');
  finish();
}, timeoutMs);

try {
  // A test signals completion through completeTest, which may happen before
  // or after its own promise settles, so wait for whichever comes first and
  // then for completion.
  await Promise.race([Promise.resolve(testFunction(runner)), finished]);
  await Promise.race([
    finished,
    new Promise((ok) => setTimeout(ok, timeoutMs)),
  ]);
} catch (error) {
  output.push(`Error while executing test script: ${error}\n${error.stack}`);
}
clearTimeout(timer);
socket.close();
report(0);
