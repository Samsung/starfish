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
import time
import unittest

# Address space the out-of-memory groups may use beyond the browser's own:
# room for their graph, but far less than their largest allocations need.
OOM_HEADROOM = 4 << 30


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
        cls.server = ThreadingHTTPServer(('127.0.0.1', 0),
            partial(SimpleHTTPRequestHandler, directory=str(cls.path)))
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

    def address_space(self):
        """Peak virtual size (bytes) of the browser on an idle probe page."""
        process = subprocess.Popen(self.command('idle'),
                                   env=self.environment(),
                                   stdout=subprocess.DEVNULL,
                                   stderr=subprocess.DEVNULL)
        peak = 0
        while process.poll() is None:
            try:
                with open(f'/proc/{process.pid}/status') as status:
                    for line in status:
                        if line.startswith('VmPeak:'):
                            peak = max(peak, int(line.split()[1]) * 1024)
            except OSError:
                pass
            time.sleep(0.05)
        self.assertGreater(peak, 0, 'could not read the browser VmPeak')
        return peak

    def run_group(self, group, address_space_limit=None):
        def limit():
            resource.setrlimit(resource.RLIMIT_AS,
                               (address_space_limit, address_space_limit))
        result = subprocess.run(
            self.command(group), env=self.environment(),
            preexec_fn=limit if address_space_limit else None,
            capture_output=True, text=True, timeout=40)
        output = result.stdout + result.stderr
        self.assertEqual(result.returncode, 0, output)
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

    def test_out_of_memory(self):
        # The limit is relative to this build's own peak, so the groups
        # fail allocations deterministically regardless of host RAM.
        limit = self.address_space() + OOM_HEADROOM
        self.run_group('oom-channel-count', limit)
        self.run_group('oom-node-creation', limit)

    def test_audio_worklet(self):
        # Needs a build with AUDIO_WORKLET=1, as CI builds.
        self.run_group('audio-worklet')


if __name__ == '__main__':
    unittest.main()
