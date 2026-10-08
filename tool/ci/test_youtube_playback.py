#!/usr/bin/env python3
"""Verify a real YouTube iframe, changing uploaded frames, framebuffer and PCM.

Needs Pillow; Linux also needs an X11 display (run under xvfb-run).
The explicit PCM test sink exercises the writer queue without a physical
speaker. This is not a hardware audio endpoint test.
"""

import argparse
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import ssl
import struct
import subprocess
import threading
import time
from urllib.parse import parse_qs, quote, urlsplit
from urllib.request import Request, urlopen

from PIL import Image, ImageGrab, ImageStat


def executable_bits(path):
    with path.open('rb') as stream:
        header = stream.read(64)
        if header[:4] == b'\x7fELF':
            return {1: 32, 2: 64}[header[4]]
        if header[:2] == b'MZ':
            stream.seek(struct.unpack_from('<I', header, 60)[0] + 4)
            return {0x14c: 32, 0x8664: 64}[struct.unpack('<H', stream.read(2))[0]]
    raise RuntimeError('Unsupported browser executable architecture')


def check_framebuffer(path):
    with Image.open(path) as source:
        if source.width < 960 or source.height < 540:
            raise RuntimeError('Framebuffer is smaller than the iframe')
        region = source.convert('RGB').crop((80, 80, 850, 450))
        stats = ImageStat.Stat(region)
        colors = len(region.resize((64, 48)).getcolors(4096) or [])
        if max(stats.stddev) < 12 or colors < 64:
            raise RuntimeError('YouTube framebuffer is blank or nearly uniform')
        return {'colors': colors, 'stddev': stats.stddev}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--browser', required=True)
    parser.add_argument('--video', default='M7lc1UVf-VE')
    parser.add_argument('--timeout', type=int, default=90)
    parser.add_argument('--expected-bits', type=int, choices=[32, 64])
    parser.add_argument('--output', default='youtube-playback-results')
    parser.add_argument('--ignore-certificate-errors', action='store_true',
                        help='Skip preflight TLS checks; use only with a CI playback build')
    args = parser.parse_args()
    browser = Path(args.browser).resolve()
    bits = executable_bits(browser)
    if args.expected_bits and bits != args.expected_bits:
        raise RuntimeError(f'Expected {args.expected_bits}-bit browser, got {bits}')
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    metrics = {'bits': bits, 'video': args.video, 'max_time': 0.0,
               'frames': 0, 'audio_samples': 0, 'audio_peak': 0, 'audio_rms': 0.0,
               'verify_certificates': not args.ignore_certificate_errors}
    signatures = set()
    errors = []
    lock = threading.Lock()
    capture_ready = threading.Event()
    page = Path(__file__).with_name('youtube_playback.html').read_bytes()
    if os.name == 'nt':
        # Hold top-level onload until playback is proven, then let the shell's
        # existing one-shot screenshot capture the current frame immediately.
        # A fixed frame count varies with runner speed and can exceed timeout.
        page += b'<img src="/capture-gate" width="1" height="1" alt="">'

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *unused):
            pass

        def do_GET(self):
            request = urlsplit(self.path)
            if request.path == '/yt-status':
                try:
                    event = json.loads(parse_qs(request.query)['event'][0])
                    with lock:
                        if event.get('event') == 'error':
                            errors.append(f'YouTube player error {event.get("code")}')
                        if event.get('event') == 'playing':
                            if event.get('muted') or event.get('volume', 0) <= 0:
                                errors.append('YouTube player is muted')
                            metrics['max_time'] = max(metrics['max_time'], float(event['time']))
                    print('YOUTUBE_STATUS ' + json.dumps(event), flush=True)
                except (ValueError, KeyError, TypeError):
                    self.send_error(400)
                    return
                data = b''
            elif request.path == '/capture-gate' and os.name == 'nt':
                capture_ready.wait(args.timeout)
                data = bytes.fromhex('47494638396101000100800000000000ffffff21f90401000000002c00000000010001000002024401003b')
            elif request.path == '/':
                data = page
            else:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header('Content-Type', 'image/gif' if request.path == '/capture-gate'
                             else 'text/html; charset=utf-8')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            try:
                self.wfile.write(data)
            except BrokenPipeError:
                pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    environment = dict(os.environ)
    environment['NO_PROXY'] = environment.get('NO_PROXY', environment.get('no_proxy', '')) + ',127.0.0.1,localhost,::1'
    environment['no_proxy'] = environment['NO_PROXY']
    url = f'http://127.0.0.1:{server.server_port}/?video={quote(args.video)}'
    command = [str(browser), url, '--disable-web-security']
    if os.name == 'nt':
        capture = output / 'framebuffer.bmp'
        command += [f'--screenshot={capture}', '--screenshot-frames=0',
                    '--window-size=1280x800', f'--timeout-ms={args.timeout * 1000}']
    else:
        if not os.environ.get('DISPLAY'):
            raise RuntimeError('Linux playback requires xvfb-run or an X11 display')
        capture = output / 'framebuffer.png'
        command += ['--width=1280', '--height=800', f'--timeout={args.timeout}']
    # A failed rerun must not reuse a framebuffer from an earlier run.
    capture.unlink(missing_ok=True)
    print('YOUTUBE_BROWSER ' + json.dumps(command), flush=True)
    process = None
    reader = None
    result = 1
    log = None
    try:
        log = (output / 'browser.log').open('w', encoding='utf-8')
        # CI trust stores differ from developer machines. Keep the preflight
        # bypass explicit; normal invocations still verify certificates.
        context = ssl.create_default_context()
        if args.ignore_certificate_errors:
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
        print('YOUTUBE_TLS_VERIFY ' + str(not args.ignore_certificate_errors), flush=True)
        with urlopen(Request('https://www.youtube.com/iframe_api', method='HEAD'),
                     timeout=15, context=context) as response:
            print('YOUTUBE_NETWORK_OK ' + str(response.status), flush=True)
        process = subprocess.Popen(command, env=environment, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True, errors='replace')

        def read_output():
            for line in process.stdout:
                log.write(line)
                log.flush()
                frame = re.search(r'FFMPEG_VIDEO_PRESENT width=(\d+) height=(\d+) hash=(\d+)', line)
                audio = re.search(r'FFMPEG_CI_AUDIO samples=(\d+) rms=([\d.]+) peak=(\d+)', line)
                with lock:
                    if frame and int(frame[1]) >= 200 and int(frame[2]) >= 200:
                        metrics['frames'] += 1
                        if len(signatures) < 512:
                            signatures.add(frame[3])
                    if audio:
                        metrics['audio_rms'] = max(metrics['audio_rms'], float(audio[2]))
                        metrics['audio_peak'] = max(metrics['audio_peak'], int(audio[3]))
                        if float(audio[2]) > 0.001:
                            metrics['audio_samples'] += int(audio[1])

        reader = threading.Thread(target=read_output, daemon=True)
        reader.start()
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            with lock:
                ready = (metrics['max_time'] >= 18 and metrics['frames'] >= 90 and
                         len(signatures) >= 30 and metrics['audio_samples'] >= 48000)
                failure = errors[0] if errors else None
            if failure:
                raise RuntimeError(failure)
            if ready:
                capture_ready.set()
                if os.name != 'nt':
                    ImageGrab.grab(xdisplay=os.environ['DISPLAY']).save(capture)
                if capture.exists() and (os.name != 'nt' or process.poll() is not None):
                    if os.name == 'nt' and process.returncode != 0:
                        raise RuntimeError(f'Browser exited with {process.returncode}')
                    metrics['framebuffer'] = check_framebuffer(capture)
                    with Image.open(capture) as image:
                        image.save(output / 'screenshot.png')
                    result = 0
                    break
            if process.poll() is not None:
                raise RuntimeError('Browser exited before video/audio/framebuffer checks passed')
            time.sleep(0.1)
        if result:
            raise RuntimeError('Timed out waiting for real video, audio and framebuffer')
    except (RuntimeError, OSError) as error:
        metrics['failure'] = str(error)
        print('YOUTUBE_PLAYBACK_FAIL ' + str(error), flush=True)
    finally:
        capture_ready.set()
        if process and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        if reader:
            reader.join(timeout=5)
        if log:
            log.close()
            if result:
                with (output / 'browser.log').open(encoding='utf-8', errors='replace') as failed_log:
                    print('YOUTUBE_BROWSER_LOG\n' + ''.join(deque(failed_log, maxlen=40)), flush=True)
        server.shutdown()
        server.server_close()
        metrics['unique_frames'] = len(signatures)
        (output / 'summary.json').write_text(json.dumps(metrics, indent=2) + '\n', encoding='utf-8')
    print(('YOUTUBE_PLAYBACK_PASS ' if not result else 'YOUTUBE_PLAYBACK_FAIL ') + json.dumps(metrics), flush=True)
    return result


if __name__ == '__main__':
    raise SystemExit(main())
