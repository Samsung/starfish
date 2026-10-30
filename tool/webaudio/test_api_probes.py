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
import shutil
import subprocess
import tempfile
import threading
import unittest


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

    def run_group(self, group):
        result = subprocess.run([
            str(self.browser),
            f'http://127.0.0.1:{self.server.server_port}/?{group}',
            '--hide-window', '--timeout=20',
            f'--storage-dir={self.path / "storage"}',
        ], env=dict(os.environ,
                    PULSE_SERVER=f'unix:{self.path}/no-audio-device'),
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


if __name__ == '__main__':
    unittest.main()
