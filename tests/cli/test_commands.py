import base64

from driver.support import CLI_PROGRAM_NAME, CLITestCase, FIXTURES_DIR


class CommandIntegrationTest(CLITestCase):
    def test_help_uses_public_cli_name(self):
        result = self.run_cli("--help")

        self.assertEqual(result.stderr, "")
        self.assertIn("Usage: %s" % CLI_PROGRAM_NAME, result.stdout)

    def test_open_snapshot_and_close_share_one_session(self):
        # Each command runs as its own process. snapshot must still see the
        # page that open loaded, which only works if both reached the same
        # daemon. close then ends that session: the socket is gone and the
        # daemon and engine exit.
        self.run_cli("open", self.resource_url("interactive-elements.html"))
        snapshot = self.run_cli("snapshot", "-i")
        expected = (
            FIXTURES_DIR / "snapshot-interactive-expected.txt"
        ).read_text(encoding="utf-8")

        self.assertEqual(snapshot.stdout, expected)
        self.run_cli("close")
        self.assertFalse(self.session_socket_path().exists())
        self.wait_for_session_exit()

    def test_snapshot_prints_element_states(self):
        self.run_cli("open", self.resource_url("element-states.html"))
        snapshot = self.run_cli("snapshot", "-i")
        expected = (FIXTURES_DIR / "snapshot-states-expected.txt").read_text(
            encoding="utf-8"
        )

        self.assertEqual(snapshot.stdout, expected)
        self.run_cli("close")

    def test_click_changes_page_state(self):
        # open a page with a counter button, snapshot to get the @eN
        # reference, click it, then snapshot again to verify the button
        # text changed.
        self.run_cli("open", self.resource_url("click-counter.html"))
        snapshot = self.run_cli("snapshot", "-i")

        self.assertIn("[button]", snapshot.stdout)

        invalid = self.run_cli("click", "@e1junk", check=False)
        self.assertEqual(invalid.returncode, 1)
        self.assertIn("valid @eN reference", invalid.stderr)

        self.run_cli("click", "@e1")
        after = self.run_cli("snapshot", "-i")

        self.assertIn("Count: 1", after.stdout)
        self.run_cli("close")
        self.assertFalse(self.session_socket_path().exists())
        self.wait_for_session_exit()

    def test_fill_replaces_input_value(self):
        # open a page with a pre-filled input and a mirror button,
        # snapshot to get @eN refs, fill the input with new text,
        # click mirror to expose the value, then snapshot to verify.
        self.run_cli("open", self.resource_url("fill-input.html"))
        snapshot = self.run_cli("snapshot", "-i")

        self.assertIn("[textbox]", snapshot.stdout)

        self.run_cli("fill", "@e1", "hello fill")
        self.run_cli("click", "@e2")
        after = self.run_cli("snapshot", "-i")

        self.assertIn("hello fill", after.stdout)
        self.run_cli("close")
        self.assertFalse(self.session_socket_path().exists())
        self.wait_for_session_exit()

    def test_eval_prints_scalar_string_and_object_values(self):
        self.run_cli("open", "about:blank")

        scalar = self.run_cli("eval", "6 * 7")
        string = self.run_cli("eval", "'ready'")
        object_value = self.run_cli("eval", "({answer: 42})")

        self.assertEqual(scalar.stdout, "42\n")
        self.assertEqual(scalar.stderr, "")
        self.assertEqual(string.stdout, '"ready"\n')
        self.assertEqual(string.stderr, "")
        self.assertEqual(object_value.stdout, '{\n  "answer": 42\n}\n')
        self.assertEqual(object_value.stderr, "")

    def test_eval_prints_numbers_like_json_stringify(self):
        self.run_cli("open", "about:blank")

        result = self.run_cli(
            "eval", "[2 ** 64, 2 ** 53, 1e21, 1e-7, 0.1 + 0.2, 1.5]")

        self.assertEqual(
            result.stdout,
            "[\n"
            "  18446744073709552000,\n"
            "  9007199254740992,\n"
            "  1e+21,\n"
            "  1e-7,\n"
            "  0.30000000000000004,\n"
            "  1.5\n"
            "]\n")

    def test_eval_awaits_resolved_promise(self):
        self.run_cli("open", "about:blank")

        result = self.run_cli(
            "eval", "Promise.resolve({status: 'resolved'})")

        self.assertEqual(result.stdout, '{\n  "status": "resolved"\n}\n')
        self.assertEqual(result.stderr, "")

    def test_eval_awaits_timer_promises_and_keeps_session_after_rejection(self):
        self.run_cli("open", "about:blank")

        resolved = self.run_cli(
            "eval",
            "new Promise(resolve => "
            "setTimeout(() => resolve(42), 20))",
        )
        rejected = self.run_cli(
            "eval",
            "new Promise((resolve, reject) => "
            "setTimeout(() => reject(new Error('timer rejected')), 20))",
            check=False,
        )

        self.assertEqual(resolved.stdout, "42\n")
        self.assertEqual(resolved.stderr, "")
        self.assertEqual(rejected.returncode, 1)
        self.assertEqual(rejected.stdout, "")
        self.assertIn("Evaluation error:", rejected.stderr)
        self.assertIn("timer rejected", rejected.stderr)

        recovered = self.run_cli("eval", "6 * 7")
        self.assertEqual(recovered.stdout, "42\n")
        self.assertEqual(recovered.stderr, "")

    def test_eval_handles_empty_values_and_rejected_promise(self):
        self.run_cli("open", "about:blank")

        null_value = self.run_cli("eval", "null")
        undefined_value = self.run_cli("eval", "undefined")
        rejected = self.run_cli(
            "eval",
            "Promise.reject(new Error('rejected'))",
            check=False,
        )

        self.assertEqual(null_value.stdout, "null\n")
        self.assertEqual(null_value.stderr, "")
        self.assertEqual(undefined_value.stdout, "null\n")
        self.assertEqual(undefined_value.stderr, "")
        self.assertEqual(rejected.returncode, 1)
        self.assertEqual(rejected.stdout, "")
        self.assertIn("Evaluation error:", rejected.stderr)
        self.assertIn("rejected", rejected.stderr)

        recovered = self.run_cli("eval", "6 * 7")
        self.assertEqual(recovered.stdout, "42\n")
        self.assertEqual(recovered.stderr, "")

    def test_eval_javascript_exception_does_not_end_session(self):
        self.run_cli("open", "about:blank")

        failed = self.run_cli(
            "eval", "throw new Error('eval failed')", check=False)

        self.assertEqual(failed.returncode, 1)
        self.assertEqual(failed.stdout, "")
        self.assertIn(
            "Evaluation error: Error: eval failed", failed.stderr)

        recovered = self.run_cli("eval", "21 * 2")
        self.assertEqual(recovered.stdout, "42\n")
        self.assertEqual(recovered.stderr, "")

    def test_eval_joins_unquoted_script_arguments(self):
        self.run_cli("open", "about:blank")

        result = self.run_cli("eval", "1", "+", "2")

        self.assertEqual(result.stdout, "3\n")
        self.assertEqual(result.stderr, "")

    def test_eval_accepts_short_and_long_base64_options(self):
        self.run_cli("open", "about:blank")
        encoded = base64.b64encode(b"'base64 input'").decode("ascii")

        for option in ("-b", "--base64"):
            with self.subTest(option=option):
                result = self.run_cli("eval", option, encoded)

                self.assertEqual(result.stdout, '"base64 input"\n')
                self.assertEqual(result.stderr, "")

    def test_eval_rejects_invalid_option_shapes_and_utf8(self):
        invalid_utf8 = (
            b"\x80",
            b"\xe2\x28\xa1",
            b"\xc0\x80",
            b"\xed\xa0\x80",
            b"\xf4\x90\x80\x80",
        )
        invalid_base64 = ("%%%%", "AB==", "YQ==AAAA")
        commands = [
            ("eval",),
            ("eval", "-b"),
            ("eval", "--base64"),
            ("eval", "--stdin", "extra"),
        ]
        commands.extend(
            ("eval", "--base64", base64.b64encode(value).decode("ascii"))
            for value in invalid_utf8
        )
        commands.extend(
            ("eval", "--base64", value) for value in invalid_base64
        )

        for arguments in commands:
            with self.subTest(arguments=arguments):
                result = self.run_cli(*arguments, check=False)

                self.assertEqual(result.returncode, 1)
                self.assertEqual(result.stdout, "")
                self.assertIn("Usage:", result.stderr)

    def test_eval_preserves_embedded_null_from_stdin(self):
        self.run_cli("open", "about:blank")

        result = self.run_cli("eval", "--stdin", input="//\0\n42")

        self.assertEqual(result.stdout, "42\n")
        self.assertEqual(result.stderr, "")

    def test_eval_reads_script_from_stdin(self):
        self.run_cli("open", "about:blank")

        result = self.run_cli(
            "eval", "--stdin", input="Promise.resolve('stdin input')")

        self.assertEqual(result.stdout, '"stdin input"\n')
        self.assertEqual(result.stderr, "")

    def test_complete_interactive_flow_can_inspect_page_with_eval(self):
        self.run_cli("open", self.resource_url("fill-input.html"))
        snapshot = self.run_cli("snapshot", "-i")
        self.assertIn("[textbox]", snapshot.stdout)
        self.assertIn("[button]", snapshot.stdout)

        self.run_cli("fill", "@e1", "complete flow")
        self.run_cli("click", "@e2")
        result = self.run_cli(
            "eval", "document.getElementById('mirror').textContent")

        self.assertEqual(result.stdout, '"complete flow"\n')
        self.assertEqual(result.stderr, "")
        self.run_cli("close")
        self.assertFalse(self.session_socket_path().exists())
        self.wait_for_session_exit()
