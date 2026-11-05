#!/usr/bin/env python3
"""Run the Web Audio API probes in api_probes.html against a Starfish build.

Covers behaviour no runnable WPT gates on the CI build. Run under xvfb-run.
STARFISH_BIN selects the browser executable (default ./Starfish). Only a
loopback HTTP server is used; no audio device or external network.
"""

from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import resource
import shutil
import subprocess
import tempfile
import threading
import unittest


class ProbeServer(ThreadingHTTPServer):
    """Serves the probe page and limits the browser's address space on
    request.

    A limit measured on another run of the browser does not leave the same
    room in this one: its threads' malloc arenas and other mappings differ
    by hundreds of MB from run to run. The page therefore asks once its
    setup is done, and the limit is set relative to what this browser
    process uses at that moment.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.margin = None
        self.pid = None
        self.started = threading.Event()
        self.limited = None
        self.error = None

    def expect(self, margin):
        self.margin = margin
        self.pid = None
        self.started.clear()
        self.limited = None
        self.error = None

    def attach(self, pid):
        self.pid = pid
        self.started.set()

    def detach(self):
        self.margin = None
        self.pid = None
        self.started.clear()

    def limit_address_space(self):
        if not self.margin:
            return
        if not self.started.wait(10):
            self.error = 'the browser was not started'
            return
        try:
            with open(f'/proc/{self.pid}/status') as status:
                size = next(int(line.split()[1]) * 1024 for line in status
                            if line.startswith('VmSize:'))
            limit = size + self.margin
            resource.prlimit(self.pid, resource.RLIMIT_AS, (limit, limit))
            self.limited = limit
        except (OSError, StopIteration) as error:
            self.error = repr(error)


class ProbeRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path != '/address-space-limit':
            super().do_GET()
            return
        self.server.limit_address_space()
        self.send_response(200)
        self.send_header('Content-Length', '0')
        self.end_headers()


class APIProbesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.environ.get('DISPLAY'):
            raise RuntimeError('Run this test under xvfb-run')
        root = Path(__file__).resolve().parents[2]
        cls.browser = Path(os.environ.get('STARFISH_BIN',
                                          root / 'Starfish')).resolve()
        if not cls.browser.is_file():
            raise RuntimeError(f'Missing browser: {cls.browser}')
        cls.directory = tempfile.TemporaryDirectory(prefix='starfish-api-probes-')
        cls.addClassCleanup(cls.directory.cleanup)
        cls.path = Path(cls.directory.name)
        shutil.copyfile(Path(__file__).with_name('api_probes.html'),
                        cls.path / 'index.html')
        cls.server = ProbeServer(('127.0.0.1', 0),
            partial(ProbeRequestHandler, directory=str(cls.path)))
        thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        thread.start()
        cls.addClassCleanup(cls.server.server_close)
        cls.addClassCleanup(thread.join)
        cls.addClassCleanup(cls.server.shutdown)

    def command(self, group):
        return [
            str(self.browser),
            f'http://127.0.0.1:{self.server.server_port}/?{group}',
            '--hide-window', '--timeout=20',
            f'--storage-dir={self.path / "storage"}',
        ]

    def environment(self):
        return dict(os.environ,
                    PULSE_SERVER=f'unix:{self.path}/no-audio-device')

    def run_group(self, group, address_space_margin=None):
        """Runs one group. With |address_space_margin| (bytes), the page's
        limitAddressSpace() leaves the browser that much more address space
        than it uses at that point."""
        self.server.expect(address_space_margin)
        process = subprocess.Popen(
            self.command(group), env=self.environment(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.server.attach(process.pid)
        try:
            stdout, stderr = process.communicate(timeout=40)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise
        finally:
            self.server.detach()
        output = stdout + stderr
        if address_space_margin:
            self.assertIsNone(self.server.error, self.server.error)
            self.assertIsNotNone(self.server.limited,
                                 f'{group} never asked for the limit')
        self.assertEqual(process.returncode, 0, output)
        self.assertNotIn('API_PROBE_FAIL', output, output)
        self.assertNotIn('API_PROBE_SKIP', output, output)
        self.assertIn('API_PROBE_PASS', output, output)

    def test_shared_array_buffer_views_are_rejected(self):
        self.run_group('shared-views')

    def test_media_element_source_constructor(self):
        self.run_group('media-element-source')

    def test_reverse_loop_with_sub_sample_start(self):
        self.run_group('reverse-loop')

    def test_scheduled_sources_use_exact_times(self):
        self.run_group('exact-times')

    def test_script_processor(self):
        self.run_group('script-processor')

    def test_render_quantum_size(self):
        self.run_group('render-quantum')

    def test_detached_document(self):
        self.run_group('detached-document')

    def test_source_node_options(self):
        self.run_group('source-options')

    def test_detached_channel(self):
        self.run_group('detached-channel')

    def test_attribute_accessor_names(self):
        self.run_group('accessor-names')

    def test_structured_clone_transfer(self):
        self.run_group('structured-clone-transfer')

    def test_out_of_memory(self):
        # Each group asks for the limit once its graph is built, so the
        # margin is what is left for the allocations under test, regardless
        # of host RAM: less than the failing ones need, more than the rest.
        self.run_group('oom-channel-count', 4 << 30)
        self.run_group('oom-node-creation', 512 << 20)

    def test_audio_worklet(self):
        # Needs a build with AUDIO_WORKLET=1, as CI builds.
        self.run_group('audio-worklet')
        self.run_group('audio-worklet-node')
        self.run_group('audio-worklet-slow-render')
        self.run_group('audio-worklet-failed-node')


if __name__ == '__main__':
    unittest.main()
