import time

from driver.support import CLITestCase

CLOSED_DATA = {
    "closed": True,
    "restoreStatus": "not_configured",
    "saveStatus": "not_attempted",
}
INVALID_SESSION = (
    "Invalid session name '%s'. Only alphanumeric characters, hyphens, "
    "and underscores are allowed."
)


class FlowTest(CLITestCase):
    def test_text_flow(self):
        url = self.resource_url("fill-input.html")

        self.assertEqual(self.run_cli("open", url).stdout,
                         "\u2713 Fill test\n  %s\n" % url)
        snapshot = self.run_cli("snapshot", "-i").stdout
        self.assertIn('@e1 [textbox]', snapshot)
        self.assertEqual(self.run_cli("fill", "@e1", "typed text").stdout,
                         "\u2713 Done\n")
        self.assertEqual(self.run_cli("click", "@e2").stdout, "\u2713 Done\n")
        evaluated = self.run_cli("eval", "document.getElementById('mirror')"
                                 ".textContent")
        self.assertEqual(evaluated.stdout, '"typed text"\n')
        self.assertEqual(self.run_cli("close").stdout,
                         "\u2713 Browser closed\n")

    def test_json_flow(self):
        url = self.resource_url("fill-input.html")

        opened, line = self.run_json("open", url)
        self.assertTrue(line.startswith('{"success":true,"data":{"url":'))
        self.assertTrue(line.endswith(',"error":null}'), line)
        self.assertEqual(list(opened), ["success", "data", "error"])
        self.assertEqual(opened["data"]["url"], url)
        self.assertEqual(opened["data"]["title"], "Fill test")
        self.assertTrue(opened["data"]["targetId"])
        self.assertEqual(list(opened["data"]), ["url", "title", "targetId"])

        snapshot, _ = self.run_json("snapshot", "-i")
        text = self.run_cli("snapshot", "-i").stdout
        self.assertEqual(snapshot["data"]["snapshot"], text)
        self.assertEqual(snapshot["data"]["origin"], url)
        self.assertEqual(snapshot["data"]["refs"]["e1"]["role"], "textbox")
        self.assertEqual(snapshot["data"]["refs"]["e2"],
                         {"role": "button", "name": "Mirror"})

        filled, _ = self.run_json("fill", "@e1", "typed text")
        self.assertEqual(filled["data"], {"filled": "@e1"})
        clicked, _ = self.run_json("click", "@e2")
        self.assertEqual(clicked["data"], {"clicked": "@e2"})

        evaluated, line = self.run_json(
            "eval", "({n: 1e21, v: undefined, t: "
            "document.getElementById('mirror').textContent})")
        self.assertEqual(evaluated["data"]["result"],
                         {"n": 1e21, "t": "typed text"})
        self.assertEqual(evaluated["data"]["origin"], url)
        self.assertIn('"n":1e+21', line)
        undefined, _ = self.run_json("eval", "undefined")
        self.assertIsNone(undefined["data"]["result"])
        self.assertIn("result", undefined["data"])

        closed, _ = self.run_json("close")
        self.assertEqual(closed["data"], CLOSED_DATA)


class ParseFailureTest(CLITestCase):
    def test_parse_failures_in_json_and_text(self):
        cases = [
            (("bogus",), "unknown_command", "Unknown command: bogus"),
            (("open",), "missing_arguments", "Missing arguments for: open"),
            (("close", "--session"), "missing_arguments",
             "Missing arguments for: --session"),
            (("--session", "close"), "missing_arguments",
             "Missing arguments for: --session"),
            (("close", "--timeout"), "missing_arguments",
             "Missing arguments for: --timeout"),
            (("close", "--timeout", "1.5"), "invalid_value",
             "--timeout expects a number in ms, got '1.5'"),
            (("close", "--timeout", "0"), "invalid_value",
             "--timeout expects a number in ms, got '0'"),
            (("close", "--session", "a b"), "invalid_session_name",
             INVALID_SESSION % "a b"),
            (("--session", "a/b", "open", "x"), "invalid_session_name",
             INVALID_SESSION % "a/b"),
        ]
        for arguments, error_type, message in cases:
            with self.subTest(arguments=arguments):
                parsed, line = self.run_json(*arguments, exit_code=1)
                self.assertEqual(list(parsed), ["error", "success", "type"])
                self.assertEqual(
                    parsed, {"error": message, "success": False,
                             "type": error_type})

                text = self.run_cli(*arguments, check=False)
                self.assertEqual(text.returncode, 1)
                self.assertEqual(text.stdout, "")
                self.assertIn(message, text.stderr)


class SessionTest(CLITestCase):
    def run_in(self, session, *arguments):
        return self.run_cli("--session", session, *arguments)

    def test_option_position_and_session_isolation(self):
        url = self.resource_url("click-counter.html")

        self.run_cli("--session", "s1", "open", url)
        self.run_cli("open", url, "--session", "s2")
        self.assertTrue((self.session_socket_path().parent
                         / "s1.sock").exists())
        self.assertTrue((self.session_socket_path().parent
                         / "s2.sock").exists())
        # The default session was never started.
        self.assertFalse(self.session_socket_path().exists())
        self.assertEqual(
            self.run_cli("close", check=False).returncode, 1)

        # Same name, same daemon. Different name, different page state.
        self.run_in("s1", "eval", "localStorage.setItem('k', 'one')")
        self.assertEqual(self.run_in("s1", "eval",
                                     "localStorage.getItem('k')").stdout,
                         '"one"\n')
        self.assertEqual(self.run_in("s2", "eval",
                                     "localStorage.getItem('k')").stdout,
                         "null\n")

        self.run_cli("close", "--session", "s1")
        self.run_in("s2", "close")
        self.assertFalse(
            (self.session_socket_path().parent / "s1.sock").exists())
        self.wait_for_session_exit()


class TimeoutTest(CLITestCase):
    def test_timeout_keeps_session_usable(self):
        self.run_cli("open", "about:blank")

        started = time.monotonic()
        parsed, _ = self.run_json(
            "--timeout", "200", "eval",
            "new Promise(r => setTimeout(() => r('late'), 800))",
            exit_code=1)
        elapsed = time.monotonic() - started
        self.assertEqual(
            parsed,
            {"success": False, "data": None, "error": "Timed out after 200ms"})
        self.assertLess(elapsed, 2)

        # The late reply must not answer the next command.
        self.assertEqual(self.run_cli("eval", "1").stdout, "1\n")
        time.sleep(1.2)
        self.assertEqual(self.run_cli("eval", "2").stdout, "2\n")

        text = self.run_cli("--timeout", "200", "eval",
                            "new Promise(() => {})", check=False)
        self.assertEqual(text.returncode, 1)
        self.assertIn("Timed out after 200ms", text.stderr)
        self.assertEqual(self.run_cli("eval", "3").stdout, "3\n")

    def test_timeout_covers_daemon_start(self):
        cli_binary = self.install_stalled_engine()

        started = time.monotonic()
        result = self.run_cli("open", "about:blank", "--timeout", "500",
                              binary=cli_binary, check=False)

        self.assertEqual(result.returncode, 1)
        self.assertIn("Timed out after 500ms", result.stderr)
        self.assertLess(time.monotonic() - started, 5)
        self.wait_for_session_exit()


class StaleRefTest(CLITestCase):
    def test_stale_and_unknown_refs(self):
        url = self.resource_url("stale-refs.html")
        self.run_cli("open", url)

        # Before any snapshot, no ref was issued.
        unknown, _ = self.run_json("click", "@e1", exit_code=1)
        self.assertEqual(unknown, {
            "success": False, "data": None, "error": "Unknown ref: e1"})

        self.run_cli("snapshot", "-i")
        unknown, line = self.run_json("click", "@e9", exit_code=1)
        self.assertEqual(unknown["error"], "Unknown ref: e9")
        self.assertNotIn("code", unknown)

        # A DOM change that keeps the node keeps the ref.
        self.run_cli("click", "@e3")
        self.run_cli("click", "@e3")
        self.assertEqual(
            self.run_cli("eval", "document.querySelectorAll('hr').length"
                         ).stdout, "2\n")

        # A removed node is stale. It is never found again by name.
        self.run_cli("click", "@e1")
        stale, line = self.run_json("click", "@e2", exit_code=1)
        message = "Element @e2 no longer exists. Take a new snapshot."
        self.assertEqual(stale["error"], message)
        self.assertEqual(stale["code"], "stale_ref")
        self.assertTrue(line.endswith(',"error":"%s","code":"stale_ref"}'
                                      % message), line)
        failed = self.run_cli("fill", "@e2", "x", check=False)
        self.assertEqual(failed.returncode, 1)
        self.assertIn(message, failed.stderr)

        # A new snapshot replaces the ref set.
        snapshot = self.run_cli("snapshot", "-i").stdout
        self.assertNotIn("Victim", snapshot)
        self.run_cli("click", "@e2")

        # Every ref is stale after a navigation, until the next snapshot.
        self.run_cli("open", url)
        stale, _ = self.run_json("click", "@e2", exit_code=1)
        self.assertEqual(stale["code"], "stale_ref")
        self.run_cli("snapshot", "-i")
        self.run_cli("click", "@e2")
