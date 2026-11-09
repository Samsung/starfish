import base64
import os
import socket
import subprocess
import time
import urllib.request

from driver.support import CLI_BINARY, ENGINE_BINARY_NAME, CLITestCase

ENGINE_BINARY = CLI_BINARY.parent / ENGINE_BINARY_NAME


def free_port():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


class CDPTransportIntegrationTest(CLITestCase):
    # The CLI drives the engine over CDP, so the engine's listener is the
    # boundary these tests check: a CDP client controls every page, the
    # cookie jar and TLS verification, so only local, non-browser clients
    # may reach it (docs/CDP.md).

    def setUp(self):
        super().setUp()
        self.port = free_port()
        self.start_engine()

    def start_engine(self, **extra_environment):
        environment = dict(self.environment)
        environment["STARFISH_ENABLE_CDP"] = "1"
        environment["STARFISH_CDP_PORT"] = str(self.port)
        environment.update(extra_environment)
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
        self.wait_for_discovery()

    def engine_log(self):
        # The engine's stderr, so a failed start reports why it stopped.
        with open(self.engine_log_path, "rb") as log:
            return log.read().decode(errors="replace")[-2000:]

    def stop_engine(self):
        self.engine.terminate()
        try:
            self.engine.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.engine.kill()
            self.engine.wait()

    def wait_for_discovery(self):
        url = "http://127.0.0.1:%d/json/version" % self.port
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if self.engine.poll() is not None:
                self.fail("the engine exited with %d\n%s"
                          % (self.engine.returncode, self.engine_log()))
            try:
                with urllib.request.urlopen(url, timeout=1) as response:
                    if response.status == 200:
                        return
            except OSError:
                time.sleep(0.1)
        self.fail("the engine did not answer CDP discovery on %s\n%s"
                  % (url, self.engine_log()))

    def upgrade(self, path, origin=None):
        """Return the status line of a WebSocket upgrade, or "" if the
        engine closed the connection without answering."""
        key = base64.b64encode(os.urandom(16)).decode()
        request = (
            "GET %s HTTP/1.1\r\n"
            "Host: 127.0.0.1:%d\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n" % (path, self.port, key)
        )
        if origin:
            request += "Origin: %s\r\n" % origin
        request += "\r\n"
        with socket.create_connection(("127.0.0.1", self.port),
                                      timeout=10) as connection:
            connection.sendall(request.encode())
            response = b""
            while b"\r\n" not in response:
                chunk = connection.recv(4096)
                if not chunk:
                    break
                response += chunk
        return response.split(b"\r\n", 1)[0].decode() if response else ""

    def listening_addresses(self):
        # /proc/net/tcp lists "ADDR:PORT" in hex, ADDR in host byte order on
        # little-endian hosts; state 0A is LISTEN.
        addresses = []
        with open("/proc/net/tcp") as table:
            next(table)
            for line in table:
                fields = line.split()
                address, port = fields[1].split(":")
                if int(port, 16) == self.port and fields[3] == "0A":
                    addresses.append(address)
        return addresses

    def test_listens_on_loopback_only(self):
        self.assertEqual(self.listening_addresses(), ["0100007F"])

    def test_bind_any_opt_in_needs_the_documented_value(self):
        # A value that reads as "off" must not expose the port.
        self.stop_engine()
        self.start_engine(STARFISH_CDP_BIND_ANY="0")
        self.assertEqual(self.listening_addresses(), ["0100007F"])

        self.stop_engine()
        self.start_engine(STARFISH_CDP_BIND_ANY="1")
        self.assertEqual(self.listening_addresses(), ["00000000"])

    def test_upgrade_from_web_page_is_rejected(self):
        # A browser always sends Origin on a WebSocket upgrade, so a page on
        # this host must not be able to drive the engine.
        self.assertEqual(self.upgrade("/", origin="http://example.com"), "")
        self.assertEqual(
            self.upgrade("/devtools/browser", origin="null"), "")

    def test_upgrade_to_unknown_path_is_rejected(self):
        self.assertEqual(self.upgrade("/unknown"), "")

    def test_devtools_client_upgrade_is_accepted(self):
        # Also after rejected attempts: a refused client must not keep the
        # single connection slot.
        self.upgrade("/", origin="http://example.com")
        self.assertIn(" 101 ", self.upgrade("/"))
        self.assertIn(" 101 ", self.upgrade("/devtools/page/TID-0000000001"))
