import subprocess

from driver.support import CLI_BINARY, CLITestCase


class SessionLifecycleIntegrationTest(CLITestCase):
    def test_failed_session_start_leaves_no_processes(self):
        # The daemon opens its socket only after the engine answers CDP. The
        # stalled engine opens the port but never answers, so open must fail
        # and must leave no daemon and no engine running.
        cli_binary = self.install_stalled_engine()

        result = self.run_cli(
            "open", "about:blank", binary=cli_binary, check=False)

        self.assertNotEqual(result.returncode, 0)
        self.wait_for_session_exit()

    def test_concurrent_open_uses_one_session(self):
        # Two open commands started at once must share one session. The
        # client takes start.lock and then retries the socket, so the loser
        # of the race finds the session the winner just started. Both must
        # succeed, and one close must be enough to end the session.
        command = [
            str(CLI_BINARY),
            "open",
            self.resource_url("interactive-elements.html"),
        ]

        first = subprocess.Popen(
            command,
            env=self.environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        second = subprocess.Popen(
            command,
            env=self.environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        first_stdout, first_stderr = first.communicate(timeout=45)
        second_stdout, second_stderr = second.communicate(timeout=45)

        self.assertEqual(first.returncode, 0, first_stderr + first_stdout)
        self.assertEqual(second.returncode, 0, second_stderr + second_stdout)
        self.run_cli("close")
        self.wait_for_session_exit()
