#!/usr/bin/env python3
"""Serve a YouTube iframe over HTTP and open it in the Starfish shell."""

import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import threading
from urllib.parse import parse_qs, quote, urlsplit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--browser', required=True)
    parser.add_argument('--video', default='M7lc1UVf-VE')
    parser.add_argument('--port', type=int, default=8777)
    parser.add_argument('--timeout-ms', type=int, default=0)
    parser.add_argument('--screenshot')
    parser.add_argument('--screenshot-frames', type=int, default=500)
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def do_GET(self):
            request = urlsplit(self.path)
            if request.path == '/yt-status':
                message = parse_qs(request.query).get('event', [''])[0]
                print(message, flush=True)
                self.send_response(200)
                self.send_header('Content-Length', '0')
                self.end_headers()
            else:
                super().do_GET()

    server = ThreadingHTTPServer(('127.0.0.1', args.port),
                                partial(Handler, directory=str(Path(__file__).resolve().parent)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    url = f'http://127.0.0.1:{server.server_port}/youtube_iframe.html?video={quote(args.video)}'
    environment = os.environ.copy()
    environment['NO_PROXY'] = environment.get('NO_PROXY', environment.get('no_proxy', '')) + ',127.0.0.1,localhost,::1'
    environment['no_proxy'] = environment['NO_PROXY']
    command = [str(Path(args.browser).resolve()), url]
    if args.timeout_ms and os.name == 'nt':
        command.append(f'--timeout-ms={args.timeout_ms}')
    if args.screenshot:
        command += [f'--screenshot={args.screenshot}',
                    f'--screenshot-frames={args.screenshot_frames}']
    print('Opening ' + url, flush=True)
    print('YouTube state 1 means playing; confirm audible sound in the window.', flush=True)
    try:
        return subprocess.call(command, env=environment)
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    raise SystemExit(main())
