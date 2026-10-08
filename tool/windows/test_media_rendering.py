#!/usr/bin/env python3
"""Check progressive H264 and MSE H264/AV1, including Windows framebuffer pixels."""

import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import threading
from urllib.parse import parse_qs, urlsplit


def check_pixels(path):
    data = path.read_bytes()
    if data[:2] != b'BM':
        raise RuntimeError('Missing BMP framebuffer capture')
    offset = struct.unpack_from('<I', data, 10)[0]
    width, height = struct.unpack_from('<ii', data, 18)
    depth = struct.unpack_from('<H', data, 28)[0]
    if depth != 32 or width < 384 or abs(height) < 96:
        raise RuntimeError('Unexpected framebuffer dimensions or format')
    stride = width * 4
    for name, x, expected in [('progressive H264', 64, 2),
                              ('MSE H264', 224, 2), ('MSE AV1', 384, 0)]:
        for dx, dy in [(-16, -16), (0, 0), (16, 16)]:
            y = 48 + dy if height < 0 else height - 1 - (48 + dy)
            position = offset + y * stride + (x + dx) * 4
            blue, green, red, _ = data[position:position + 4]
            rgb = (red, green, blue)
            if rgb[expected] < 180 or any(rgb[i] > 70 for i in range(3) if i != expected):
                raise RuntimeError(f'{name} framebuffer pixel is {rgb}, expected solid color')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--browser', default=os.environ.get('STARFISH_BIN'))
    parser.add_argument('--capture', help='Keep the Windows BMP for inspection')
    args = parser.parse_args()
    if not args.browser:
        parser.error('--browser or STARFISH_BIN is required')
    root = Path(__file__).resolve().parent
    assets = {'/': (root / 'media_rendering.html').read_bytes()}
    for url, file in [('clip.mp4', 'progressive.mp4'),
                      ('mse_h264.mp4', 'mse_h264.mp4'), ('mse_av1.mp4', 'mse_av1.mp4')]:
        assets['/' + url] = (root / 'fixtures' / file).read_bytes()
    result = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def do_GET(self):
            request = urlsplit(self.path)
            if request.path == '/result':
                result.append(parse_qs(request.query).get('status', ['FAIL'])[0])
                data = b''
            else:
                data = assets.get(request.path)
                if data is None:
                    self.send_error(404)
                    return
            self.send_response(200)
            self.send_header('Content-Type', 'text/html' if request.path == '/' else 'video/mp4')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    environment = os.environ.copy()
    environment['NO_PROXY'] = environment.get('NO_PROXY', environment.get('no_proxy', '')) + ',127.0.0.1,localhost,::1'
    environment['no_proxy'] = environment['NO_PROXY']
    try:
        with tempfile.TemporaryDirectory(prefix='starfish-video-') as directory:
            capture = Path(args.capture).resolve() if args.capture else Path(directory) / 'video.bmp'
            command = [str(Path(args.browser).resolve()), f'http://127.0.0.1:{server.server_port}/']
            if os.name == 'nt':
                command += [f'--screenshot={capture}', '--screenshot-frames=1000',
                            '--window-size=640x240', '--timeout-ms=20000']
            else:
                command += ['--timeout=15']
            run = subprocess.run(command, env=environment, capture_output=True, timeout=30)
            output = (run.stdout + run.stderr).decode('utf-8', errors='replace')
            if args.capture:
                Path(str(capture) + '.log').write_text(output, encoding='utf-8')
            if run.returncode or result != ['PASS']:
                print(output[-12000:])
                raise RuntimeError('Media playback regression failed')
            if os.name == 'nt':
                try:
                    check_pixels(capture)
                except RuntimeError:
                    print('\n'.join(line for line in output.splitlines()
                                    if 'FFmpeg' in line or 'DemuxerMP4' in line or
                                    'MEDIA_RENDERING' in line))
                    raise
                print('MEDIA_RENDERING_PASS: progressive H264, MSE H264/AV1 framebuffer pixels')
            else:
                if output.count('FFMPEG_VIDEO_PRESENT width=64 height=48') < 2:
                    raise RuntimeError('MSE frames missing; Linux frame inspection requires STARFISH_MEDIA_PLAYBACK_TEST=ON')
                print('MEDIA_RENDERING_PASS: progressive H264, MSE H264/AV1 decoded frames')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
