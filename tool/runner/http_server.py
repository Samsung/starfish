#! /usr/bin/env python3

# Formatted by black.

from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from contextlib import contextmanager
from functools import partial
import signal
import errno
import sys
import os


class RequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        if options.no_cache:
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
            self.send_header("Pragma", "no-cache")
            self.send_header("Expires", "0")
        super().end_headers()

    def log_message(self, format, *args):
        if options.silent:
            return
        super().log_message(format, *args)


def start_server(options):
    try:
        httpd = ThreadingHTTPServer(
            (options.bind, options.port),
            partial(RequestHandler, directory=options.directory),
        )
    except OSError as e:
        if e.errno == errno.EADDRINUSE:
            print(f"{options.port} Port in use.")
            exit(1)
        else:
            raise e

    options.port = options.port or httpd.server_address[1]

    def signal_handler(signal_number, frame):
        httpd.server_close()
        print()
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    print(f"Running HTTPServer for {options} ..")

    httpd.serve_forever()


@contextmanager
def popen_server(directory, cwd, address=None, port=0, silent=False):
    from subprocess import Popen, PIPE

    process = None
    command = [sys.executable, __file__, "-d", directory, str(port)]
    command += ["--bind", address] if address else []
    command += ["--silent"] if silent == True else []

    try:
        process = Popen(
            command,
            close_fds=True,
            cwd=cwd,
            stdout=PIPE if silent else None,
            stderr=PIPE if silent else None,
        )
        yield process
    finally:
        if not process:
            return
        process.terminate()
        process.wait()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bind",
        "-b",
        default="0.0.0.0",
        metavar="ADDRESS",
        help="Set the bind address [default: %(default)s]",
    )
    parser.add_argument(
        "--directory",
        "-d",
        default=os.getcwd(),
        help="Set the directory. [Default: %(default)s]",
    )
    parser.add_argument(
        "port",
        action="store",
        default=0,
        type=int,
        nargs="?",
        help="Set the port [default: %(default)s (auto)]",
    )
    parser.add_argument(
        "--no-cache",
        "-nc",
        action="store_true",
        default=True,
        help="Set to add no-cache control headers [default: %(default)s]",
    )
    parser.add_argument(
        "--silent",
        "-s",
        action="store_true",
        default=False,
        help="Run in silent mode [default: %(default)s]",
    )

    global options
    options = parser.parse_args()

    start_server(options)
