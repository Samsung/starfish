#!/usr/bin/env python3
"""Check Linux media PCM through an isolated PulseAudio null sink.

Run under xvfb-run. Requires pulseaudio, pactl, parec, and a WebAudio build.
STARFISH_BIN selects the browser executable. No physical device is used.
"""

import array
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
import time
import unittest


HTML = """<!doctype html><meta charset="utf-8"><title>PCM loop test</title>
<script>
const bytes = new Uint8Array(44 + 9600 * 2);
const view = new DataView(bytes.buffer);
function tag(at, value) {
    for (let i = 0; i < 4; i++) bytes[at + i] = value.charCodeAt(i);
}
tag(0, 'RIFF'); view.setUint32(4, bytes.length - 8, true);
tag(8, 'WAVE'); tag(12, 'fmt '); view.setUint32(16, 16, true);
view.setUint16(20, 1, true); view.setUint16(22, 1, true);
view.setUint32(24, 48000, true); view.setUint32(28, 96000, true);
view.setUint16(32, 2, true); view.setUint16(34, 16, true);
tag(36, 'data'); view.setUint32(40, 19200, true);
for (let i = 0; i < 9600; i++) view.setInt16(44 + i * 2, 8192, true);
const audio = new Audio();
audio.loop = true;
audio.onloadedmetadata = () => {
    audio.playbackRate = __PLAYBACK_RATE__;
    console.log('PCM_TEST_RATE_' + audio.playbackRate);
    audio.play().then(() => {
        const started = performance.now();
        console.log('PCM_TEST_PLAYING');
        setTimeout(() => {
            audio.pause();
            console.log('PCM_TEST_STOPPED ' + (performance.now() - started));
        }, 2500);
    });
};
audio.src = URL.createObjectURL(new Blob([bytes], {type: 'audio/wav'}));
audio.load();
</script>"""


class MediaOutputTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for program in ('pulseaudio', 'pactl', 'parec'):
            if not shutil.which(program):
                raise unittest.SkipTest(f'{program} is unavailable')
        if not os.environ.get('DISPLAY'):
            raise RuntimeError('Run this test under xvfb-run')
        root = Path(__file__).resolve().parents[2]
        cls.browser = Path(os.environ.get('STARFISH_BIN', root / 'Starfish')).resolve()
        if not cls.browser.is_file():
            raise RuntimeError(f'Missing browser: {cls.browser}')
        cls.directory = tempfile.TemporaryDirectory(prefix='starfish-media-pcm-')
        cls.addClassCleanup(cls.directory.cleanup)
        cls.path = Path(cls.directory.name)
        socket = cls.path / 'native'
        runtime = cls.path / 'runtime'
        runtime.mkdir(mode=0o700)
        cls.env = dict(os.environ, PULSE_SERVER=f'unix:{socket}',
                       PULSE_SINK='media_test', PULSE_RUNTIME_PATH=str(runtime),
                       DBUS_SESSION_BUS_ADDRESS=f'unix:path={runtime}/no-dbus')
        log = (cls.path / 'pulse.log').open('w')
        cls.addClassCleanup(log.close)
        cls.server = subprocess.Popen([
            'pulseaudio', '-n', '--daemonize=no', '--exit-idle-time=-1',
            '--use-pid-file=no', '--disable-shm=yes', '--enable-memfd=no',
            '--load=module-native-protocol-unix '
            f'socket={socket} auth-anonymous=1',
            # Avoid the null sink's two-second render-ahead window: the
            # monitor otherwise omits much of a short real-time stream.
            '--load=module-null-sink sink_name=media_test rate=48000 '
            'channels=2 norewinds=1',
        ], env=cls.env, stdout=subprocess.DEVNULL, stderr=log)
        cls.addClassCleanup(cls.stop_server)
        for _ in range(100):
            if cls.server.poll() is not None:
                raise RuntimeError((cls.path / 'pulse.log').read_text())
            if socket.exists():
                result = subprocess.run(['pactl', 'list', 'short', 'sinks'],
                    env=cls.env, capture_output=True, timeout=2)
                if result.returncode == 0 and b'media_test' in result.stdout:
                    return
            time.sleep(0.05)
        raise RuntimeError('Isolated PulseAudio server did not become ready')

    @classmethod
    def stop_server(cls):
        cls.server.terminate()
        try:
            cls.server.wait(timeout=5)
        except subprocess.TimeoutExpired:
            cls.server.kill()
            cls.server.wait()

    def test_loop_remains_audible_at_each_rate(self):
        for rate in (0.5, 1, 2):
            with self.subTest(playback_rate=rate):
                page = self.path / f'loop-{rate}.html'
                page.write_text(HTML.replace('__PLAYBACK_RATE__', str(rate)),
                                encoding='utf-8')
                recording = self.path / f'loop-{rate}.raw'
                capture = subprocess.Popen([
                    'parec', '--device=media_test.monitor', '--rate=48000',
                    '--latency-msec=10',
                    '--channels=2', '--format=s16le', '--raw',
                    str(recording),
                ], env=self.env, stdout=subprocess.DEVNULL,
                    stderr=subprocess.PIPE)
                try:
                    time.sleep(0.2)
                    # Normal wall-clock playback: regression mode accelerates
                    # timers and cannot validate an actual output device.
                    result = subprocess.run([
                        str(self.browser), str(page), '--hide-window',
                        '--timeout=6', f'--storage-dir={self.path / "storage"}',
                    ], env=self.env, capture_output=True, timeout=15)
                finally:
                    capture.send_signal(signal.SIGINT)
                    try:
                        _, errors = capture.communicate(timeout=5)
                    except subprocess.TimeoutExpired:
                        capture.kill()
                        capture.communicate()
                        raise
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))
                self.assertIn(capture.returncode, (0, -signal.SIGINT),
                              errors.decode(errors='replace'))
                self.assertIn(b'PCM_TEST_PLAYING', result.stdout)
                self.assertIn(b'PCM_TEST_STOPPED', result.stdout)
                self.assertIn(f'PCM_TEST_RATE_{rate}'.encode(), result.stdout)
                self.assertTrue(recording.exists(), errors.decode(errors='replace'))
                samples = array.array('h', recording.read_bytes())
                channels = 2
                sample_rate = 48000
                active_seconds = sum(abs(value) > 1000
                    for value in samples[::channels]) / sample_rate
                print(f'rate={rate}: {active_seconds:.3f} s PCM, '
                      f'{len(samples) / channels / sample_rate:.3f} s capture')
                # A first-loop-only bug produces at most 0.4 s at these
                # rates. Allow device startup/shutdown latency, but require
                # sustained PCM across several wraps, independently of DOM.
                self.assertGreater(active_seconds, 1.0,
                    f'PCM lasted only {active_seconds:.3f} s at rate {rate}')

    def test_repeated_context_lifecycle(self):
        page = self.path / 'lifecycle.html'
        page.write_text('''<!doctype html><script>
async function run() {
    for (let iteration = 0; iteration < 32; iteration++) {
        const context = new AudioContext({sampleRate: 48000});
        const source = context.createOscillator();
        const gain = context.createGain();
        source.connect(gain).connect(context.destination);
        source.start();
        await context.resume();
        for (let i = 0; i < 100; i++) {
            gain.gain.setValueAtTime(i / 100, context.currentTime);
            gain.disconnect();
            gain.connect(context.destination);
        }
        await context.suspend();
        const paused = context.currentTime;
        await new Promise(resolve => setTimeout(resolve, 5));
        if (context.currentTime !== paused) throw Error('suspended clock');
        await context.resume();
        await context.close();
        const closed = context.currentTime;
        await new Promise(resolve => setTimeout(resolve, 5));
        if (context.currentTime !== closed) throw Error('closed clock');
    }
    console.log('LIFECYCLE_PASS');
}
run().catch(error => console.log('LIFECYCLE_FAIL ' + error));
</script>''', encoding='utf-8')
        result = subprocess.run([
            str(self.browser), str(page), '--hide-window', '--timeout=12',
            f'--storage-dir={self.path / "storage"}',
        ], env=self.env, capture_output=True, timeout=20)
        output = (result.stdout + result.stderr).decode(errors='replace')
        self.assertEqual(result.returncode, 0, output)
        self.assertNotIn('LIFECYCLE_FAIL', output, output)
        self.assertIn('LIFECYCLE_PASS', output, output)

    def test_rendering_continues_while_javascript_blocks(self):
        page = self.path / 'blocked.html'
        page.write_text('''<!doctype html><script>
async function run() {
    const context = new AudioContext({sampleRate: 48000});
    const source = context.createConstantSource();
    const gain = context.createGain();
    source.connect(gain).connect(context.destination);
    await context.resume();
    const start = context.currentTime;
    gain.gain.setValueAtTime(0.25, start);
    gain.gain.setValueAtTime(0.5, start + 0.5);
    source.start(start);
    const end = Date.now() + 2000;
    while (Date.now() < end) {}
    await context.close();
    console.log('BLOCKED_PCM_DONE');
}
run();
</script>''', encoding='utf-8')
        recording = self.path / 'blocked.raw'
        capture = subprocess.Popen([
            'parec', '--device=media_test.monitor', '--rate=48000',
            '--latency-msec=10', '--channels=2', '--format=s16le', '--raw',
            str(recording),
        ], env=self.env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        try:
            time.sleep(0.2)
            result = subprocess.run([
                str(self.browser), str(page), '--hide-window', '--timeout=5',
                f'--storage-dir={self.path / "storage"}',
            ], env=self.env, capture_output=True, timeout=15)
        finally:
            capture.send_signal(signal.SIGINT)
            try:
                _, errors = capture.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                capture.kill()
                capture.communicate()
                raise
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))
        self.assertIn(b'BLOCKED_PCM_DONE', result.stdout)
        self.assertIn(capture.returncode, (0, -signal.SIGINT),
                      errors.decode(errors='replace'))
        self.assertTrue(recording.exists(), errors.decode(errors='replace'))
        samples = array.array('h', recording.read_bytes())[::2]
        low = sum(abs(value - 8192) < 128 for value in samples) / 48000
        high = sum(abs(value - 16384) < 128 for value in samples) / 48000
        print(f'blocked JS: {low:.3f} s initial PCM, {high:.3f} s automated PCM')
        # Require actual device samples, not merely an advancing context clock.
        # The automation transition must also occur without main-loop tasks.
        self.assertGreater(low, 0.2)
        self.assertGreater(high, 1.0)


if __name__ == '__main__':
    unittest.main()
