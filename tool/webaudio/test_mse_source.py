#!/usr/bin/env python3
"""Run MSE -> Web Audio PCM checks with an FFmpeg-enabled Linux build.

Requires the ffmpeg CLI to generate a small AAC fixture. Run under xvfb-run.
Only a loopback HTTP server is used; no external network or speakers needed.
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


class MSESourceTest(unittest.TestCase):
    def test_pcm_controls(self):
        self.run_pcm_controls('')

    def test_attach_during_playback(self):
        self.run_pcm_controls('?late')

    def run_pcm_controls(self, query):
        if not shutil.which('ffmpeg'):
            self.skipTest('ffmpeg CLI is unavailable')
        if not os.environ.get('DISPLAY'):
            raise RuntimeError('Run this test under xvfb-run')
        root = Path(__file__).resolve().parents[2]
        browser = Path(os.environ.get('STARFISH_BIN',
                                     root / 'out/ffmpeg/bin/Starfish')).resolve()
        self.assertTrue(browser.is_file(), f'Missing browser: {browser}')
        with tempfile.TemporaryDirectory(prefix='starfish-mse-pcm-') as directory:
            path = Path(directory)
            subprocess.run([
                'ffmpeg', '-nostdin', '-v', 'error', '-f', 'lavfi',
                '-i', 'sine=frequency=440:sample_rate=44100:duration=8',
                '-c:a', 'aac', '-b:a', '64k',
                '-movflags', 'frag_keyframe+empty_moov+default_base_moof',
                '-frag_duration', '200000', str(path / 'tone.mp4'),
            ], check=True, timeout=30)
            shutil.copyfile(Path(__file__).with_name('mse_source.html'),
                            path / 'index.html')
            server = ThreadingHTTPServer(('127.0.0.1', 0),
                partial(SimpleHTTPRequestHandler, directory=directory))
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                result = subprocess.run([
                    str(browser), f'http://127.0.0.1:{server.server_port}/{query}',
                    '--hide-window', '--timeout=15',
                    f'--storage-dir={path / "storage"}',
                ], env=dict(os.environ,
                            PULSE_SERVER=f'unix:{path}/no-audio-device'),
                    capture_output=True, text=True, timeout=25)
            finally:
                server.shutdown()
                thread.join()
                server.server_close()
            output = result.stdout + result.stderr
            self.assertEqual(result.returncode, 0, output)
            self.assertNotIn('MSE_PCM_FAIL', output, output)
            self.assertIn('MSE_PCM_PASS', output, output)
            print('MSE PCM: playback, volume, mute, gain, pause, seek passed')


if __name__ == '__main__':
    unittest.main()
