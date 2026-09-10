import functools
import http.server
import os
from pathlib import Path
import shutil
import signal
import subprocess
import threading
import time
import unittest

from common import storage

from . import STORAGE_GROUP

CLI_BINARY = Path(os.environ["LWE_CLI_TEST_BINARY"])
DRIVER_DIR = Path(__file__).resolve().parent
REPOSITORY_DIR = DRIVER_DIR.parents[2]
FIXTURES_DIR = REPOSITORY_DIR / "test" / "cli" / "fixtures"
STALLED_ENGINE = DRIVER_DIR / "fixtures" / "stalled-engine.py"

# The build names both binaries after TARGETNAME: the CLI is "<name>-cli" and
# the engine it starts is "<name>". README also builds with
# -DTARGETNAME=Starfish, so derive both names from the binary under test
# instead of assuming the default. The same two names reach the C++ side as
# LWE_CLI_PROGRAM_NAME and LWE_CLI_ENGINE_BINARY_NAME (see build/cli.cmake).
CLI_PROGRAM_NAME = CLI_BINARY.name
ENGINE_BINARY_NAME = CLI_BINARY.name.removesuffix("-cli")

# kSessionDirectory in src/launcher/cli/Constants.h is a fixed literal, so
# this path does not follow TARGETNAME.
SESSION_SOCKET_PATH = Path("lightweight-web-engine") / "cli" / "default.sock"


class QuietRequestHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, _format, *args):
        pass


class CLITestCase(unittest.TestCase):
    def setUp(self):
        # Under .tmp rather than /tmp, so what a failing test wrote is still
        # there to look at, and so a run killed before tearDown leaves its
        # directory where the next run's sweep can find it.
        self.root = storage.private_home(STORAGE_GROUP)
        self.home = self.root / "home"
        self.home.mkdir()

        handler = functools.partial(
            QuietRequestHandler,
            directory=str(FIXTURES_DIR),
        )
        self.server = http.server.ThreadingHTTPServer(
            ("127.0.0.1", 0),
            handler,
        )
        self.server_thread = threading.Thread(
            target=self.server.serve_forever,
            daemon=True,
        )
        self.server_thread.start()

        # owned_environment stamps this run as the owner. The variable reaches
        # the daemon and the engine because each inherits it, so a later sweep
        # can tell what this run left behind from what a running one owns.
        self.environment = storage.owned_environment()
        self.environment["HOME"] = str(self.home)
        self.environment.pop("DISPLAY", None)

    def tearDown(self):
        self.run_cli("close", check=False)
        for process in self.session_processes():
            try:
                os.kill(process, signal.SIGTERM)
            except ProcessLookupError:
                pass
        self.server.shutdown()
        self.server.server_close()
        self.server_thread.join(timeout=5)
        shutil.rmtree(self.root, ignore_errors=True)

    def run_cli(self, *arguments, check=True, binary=None):
        result = subprocess.run(
            [str(binary or CLI_BINARY), *arguments],
            capture_output=True,
            check=False,
            env=self.environment,
            text=True,
            timeout=45,
        )
        if check:
            self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def resource_url(self, name):
        port = self.server.server_address[1]
        return "http://127.0.0.1:%d/%s" % (port, name)

    def session_socket_path(self):
        return self.home / SESSION_SOCKET_PATH

    def install_stalled_engine(self):
        # The CLI starts the engine binary that sits next to itself, so copy
        # both into one temporary directory. The fake engine opens the CDP
        # port and then stops, so the CLI never finishes its handshake.
        # Returns the copied CLI, which callers pass to run_cli(binary=...).
        directory = self.root / "bin"
        directory.mkdir()
        cli_binary = directory / CLI_BINARY.name
        shutil.copy2(CLI_BINARY, cli_binary)
        engine_binary = directory / ENGINE_BINARY_NAME
        shutil.copy2(STALLED_ENGINE, engine_binary)
        engine_binary.chmod(0o755)
        return cli_binary

    def session_processes(self):
        home_entry = ("HOME=" + str(self.home)).encode()
        processes = []
        for entry in Path("/proc").iterdir():
            if not entry.name.isdigit():
                continue
            try:
                environment = (entry / "environ").read_bytes().split(b"\0")
            except OSError:
                continue
            if home_entry in environment:
                processes.append(int(entry.name))
        return processes

    def wait_for_session_exit(self):
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if not self.session_processes():
                return
            time.sleep(0.05)
        self.fail("CLI session processes remain: %s" % self.session_processes())
