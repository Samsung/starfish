#!/usr/bin/env python3
"""Run the common FFmpeg media regression with a native Windows or Linux shell.

Windows: python tool/windows/test_progressive_media.py --browser build/windows-x64/Release/StarfishShell.exe
Linux: STARFISH_BIN=... xvfb-run -s '-screen 0 1920x1080x24' -a python3 tool/windows/test_progressive_media.py
No physical audio device or FFmpeg command-line executable is required.
"""

import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import io
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import threading
import time
from urllib.parse import parse_qs, urlsplit
import wave


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--browser', default=os.environ.get('STARFISH_BIN'))
    args = parser.parse_args()
    if not args.browser:
        parser.error('--browser or STARFISH_BIN is required')
    root = Path(__file__).resolve().parent
    pcm = io.BytesIO()
    with wave.open(pcm, 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(48000)
        output.writeframes(b''.join(struct.pack('<h', 8192 if i % 100 < 50 else -8192)
                                    for i in range(48000 * 3)))
    assets = {'/': (root / 'progressive_media.html').read_bytes(),
              '/tone.wav': pcm.getvalue(),
              '/clip.mp4': (root / 'fixtures/progressive.mp4').read_bytes()}
    finished = threading.Event()
    result = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def do_GET(self):
            request = urlsplit(self.path)
            if request.path == '/result':
                result.append(parse_qs(request.query).get('status', ['FAIL'])[0])
                self.send_response(200)
                self.send_header('Content-Length', '0')
                self.end_headers()
                finished.set()
                return
            data = assets.get(request.path)
            if data is None:
                self.send_error(404)
                return
            start, end = 0, len(data) - 1
            byte_range = self.headers.get('Range')
            if byte_range:
                match = re.fullmatch(r'bytes=(\d+)-(\d*)', byte_range)
                if not match:
                    self.send_error(416)
                    return
                start = int(match.group(1))
                if match.group(2):
                    end = min(end, int(match.group(2)))
                if start > end:
                    self.send_error(416)
                    return
            self.send_response(206 if byte_range else 200)
            if byte_range:
                self.send_header('Content-Range', f'bytes {start}-{end}/{len(data)}')
            self.send_header('Accept-Ranges', 'bytes')
            self.send_header('Content-Type', 'text/html' if request.path == '/' else
                             'audio/wav' if request.path.endswith('.wav') else 'video/mp4')
            self.send_header('Content-Length', str(end - start + 1))
            self.end_headers()
            try:
                self.wfile.write(data[start:end + 1])
            except (BrokenPipeError, ConnectionResetError):
                pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    url = f'http://127.0.0.1:{server.server_port}/'
    command = [str(Path(args.browser).resolve()), url]
    if os.name == 'nt':
        command.append('--timeout-ms=20000')
    else:
        command += ['--exit-after-load', '--delay=18']
    try:
        with tempfile.TemporaryDirectory(prefix="starfish-progressive-") as directory, \
                tempfile.TemporaryFile(mode="w+b") as log:
            if os.name == "nt":
                # Pbuffer rendering also works without an interactive desktop.
                command += [f"--screenshot={Path(directory) / 'unused.bmp'}",
                            "--screenshot-frames=100000"]
            environment = os.environ.copy()
            bypass = environment.get('NO_PROXY', environment.get('no_proxy', ''))
            environment['NO_PROXY'] = bypass + ',127.0.0.1,localhost,::1'
            environment['no_proxy'] = environment['NO_PROXY']
            child = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                     env=environment)
            deadline = time.monotonic() + 20
            while not finished.wait(0.1) and child.poll() is None and time.monotonic() < deadline:
                pass
            if child.poll() is None:
                child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
            log.seek(0)
            output = log.read().decode('utf-8', errors='replace')
            if result != ['PASS']:
                print(output[-12000:])
                raise SystemExit('PROGRESSIVE_MEDIA_FAIL: no successful browser result')
            print('PROGRESSIVE_MEDIA_PASS: audio/video, real-time clock, pause, paused seek, ended')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
