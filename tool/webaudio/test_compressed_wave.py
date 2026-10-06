#!/usr/bin/env python3
"""Check valid compressed WAVE fallback with an FFmpeg-enabled browser."""

import base64
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class CompressedWaveTest(unittest.TestCase):
    def test_mulaw_wave(self):
        if not os.environ.get('DISPLAY'):
            raise RuntimeError('Run under xvfb-run')
        root = Path(__file__).resolve().parents[2]
        browser = Path(os.environ.get('STARFISH_BIN',
                                     root / 'out/ffmpeg/bin/Starfish')).resolve()
        with tempfile.TemporaryDirectory(prefix='starfish-wave-') as directory:
            path = Path(directory)
            fixture = path / 'tone.wav'
            subprocess.run([
                'ffmpeg', '-nostdin', '-v', 'error', '-f', 'lavfi',
                '-i', 'sine=frequency=440:sample_rate=48000:duration=0.1',
                '-c:a', 'pcm_mulaw', str(fixture),
            ], check=True, timeout=30)
            encoded = base64.b64encode(fixture.read_bytes()).decode('ascii')
            page = path / 'decode.html'
            page.write_text('''<!doctype html><script>
async function run() {
    const binary = atob('__FIXTURE__');
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
    const context = new OfflineAudioContext(1, 4800, 48000);
    const decoded = await context.decodeAudioData(bytes.buffer);
    if (decoded.length !== 4800 || decoded.sampleRate !== 48000 ||
        !decoded.getChannelData(0).some(x => Math.abs(x) > 0.01)) {
        throw Error('invalid decoded PCM');
    }
    console.log('COMPRESSED_WAVE_PASS');
}
run().catch(error => console.log('COMPRESSED_WAVE_FAIL ' + error));
</script>'''.replace('__FIXTURE__', encoded), encoding='utf-8')
            result = subprocess.run([
                str(browser), str(page), '--hide-window', '--timeout=5',
                f'--storage-dir={path / "storage"}',
            ], capture_output=True, text=True, timeout=15)
            output = result.stdout + result.stderr
            self.assertEqual(result.returncode, 0, output)
            self.assertNotIn('COMPRESSED_WAVE_FAIL', output, output)
            self.assertIn('COMPRESSED_WAVE_PASS', output, output)


if __name__ == '__main__':
    unittest.main()
