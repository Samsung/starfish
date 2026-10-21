#!/usr/bin/env python3

import json
import pathlib
import subprocess
import sys
import unittest

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR.parent))

from driver import behavior, launcher, static_server


class BehaviorReasonTest(unittest.TestCase):
    def test_output_difference_does_not_store_the_repository_path(self):
        local_url = (
            launcher.REPOSITORY_DIR
            / "test/cdp/inspector-protocol/resources/test-page.html"
        ).as_uri()
        error = AssertionError(
            "line 4: expected 'current location: "
            "http://127.0.0.1:8000/inspector-protocol/resources/test-page.html', "
            "got 'current location: %s'" % local_url
        )

        category, detail = behavior.reason(error)

        self.assertEqual("output-differs", category)
        self.assertNotIn(str(launcher.REPOSITORY_DIR), detail)
        self.assertIn("file://<repo>/test/cdp/inspector-protocol/", detail)

    def test_harness_error_does_not_store_the_repository_path(self):
        script = (
            launcher.REPOSITORY_DIR
            / "test/cdp/inspector-protocol/example/example.js"
        )

        category, detail = behavior.reason(
            RuntimeError("node exited 1: %s:23" % script)
        )

        self.assertEqual("harness-error", category)
        self.assertNotIn(str(launcher.REPOSITORY_DIR), detail)
        self.assertEqual(
            "node exited 1: <repo>/test/cdp/inspector-protocol/"
            "example/example.js:23",
            detail,
        )


class TargetBaseURLTest(unittest.TestCase):
    def test_upstream_and_internal_test_paths_use_posix_http_bases(self):
        cases = (
            (
                "inspector-protocol/accessibility/accessibility-getRootNode.js",
                "http://127.0.0.1:8000/inspector-protocol/accessibility/",
            ),
            (
                "inspector-protocol-internal/page/internal-case.js",
                "http://127.0.0.1:8000/"
                "inspector-protocol-internal/page/",
            ),
        )

        for relative, expected in cases:
            with self.subTest(relative=relative):
                test_path = static_server.DOCUMENT_ROOT / relative
                self.assertEqual(
                    expected, static_server.target_base_url(test_path)
                )


class SessionFanoutTest(unittest.TestCase):
    def test_duplicate_sessions_fan_out_until_successful_disconnect(self):
        module_url = (
            SCRIPT_DIR / "driver/node/session_fanout.mjs"
        ).resolve().as_uri()
        script = """
import { installSessionFanout } from __MODULE__;

const sessions = new Map();
installSessionFanout(sessions);
const events = [];
const makeSession = name => ({
  _dispatchMessage(message) {
    events.push(`${name}:${message.method}`);
  },
  async disconnect() {
    return `${name}:disconnected`;
  },
});
const first = makeSession('first');
const second = makeSession('second');
sessions.set('wire-session', first);
sessions.set('wire-session', second);

const dispatch = message => {
  sessions.get(message.sessionId)?._dispatchMessage(message);
};
const assert = (condition, message) => {
  if (!condition)
    throw new Error(message);
};

dispatch({sessionId: 'wire-session', method: 'Event.active'});
assert(events.join(',') === 'first:Event.active,second:Event.active',
       'active duplicate sessions did not each receive one event');

assert(await first.disconnect() === 'first:disconnected',
       'disconnect result was not preserved');
dispatch({sessionId: 'wire-session', method: 'Event.remaining'});
assert(events.join(',') ===
       'first:Event.active,second:Event.active,second:Event.remaining',
       'disconnected session received an event or remaining session did not');

await second.disconnect();
assert(!sessions.has('wire-session'),
       'empty session group left a dispatcher behind');
console.log('ok');
""".replace("__MODULE__", json.dumps(module_url))

        result = subprocess.run(
            ["node", "--input-type=module", "--eval", script],
            capture_output=True,
            text=True,
            timeout=10,
        )

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual("ok", result.stdout.strip())


if __name__ == "__main__":
    unittest.main()
