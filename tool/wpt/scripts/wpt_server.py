#!/usr/bin/env python3
# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""On-demand Web Platform Tests server for local/CI test runs.

Wraps a pinned `wpt serve` checkout as a context manager so a WPT suite can
spin the server up only while it runs and tear it down cleanly afterwards --
replacing the legacy always-on external server.

Reliability notes (learned the hard way):
  * `wpt serve` forks many per-port subprocesses; if even one alt-port (e.g.
    8444) is still held by a previous run it aborts the WHOLE server. So we
    start it in its own session and kill the entire process group on exit,
    then wait for the ports to actually free before returning.
  * Startup is only considered healthy after several consecutive good probes;
    we also fail fast if the process dies during boot.

Prerequisite: WPT subdomains must resolve to loopback. Generate once with
    python3 <wpt_root>/wpt make-hosts-file | sudo tee -a /etc/hosts
"""

import os
import signal
import socket
import subprocess
import sys
import time
from contextlib import contextmanager

HOST = "web-platform.test"
HTTP_PORT = 8000
# Ports that must be free for `wpt serve` to come up fully.
REQUIRED_PORTS = (8000, 8001, 8443, 8444)

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(_HERE, os.pardir, os.pardir))  # tool/
from repo_paths import REPO_ROOT as _REPO_ROOT  # noqa: E402

DEFAULT_INJECT = os.path.join(_REPO_ROOT, "tool", "wpt", "inject_report.js")
# Pinned WPT checkout (the `third_party/wpt` submodule). Override with WPT_ROOT.
DEFAULT_WPT_ROOT = os.environ.get(
    "WPT_ROOT", os.path.join(_REPO_ROOT, "third_party", "wpt"))


def _port_open(port, host="127.0.0.1", timeout=0.5):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(timeout)
        return s.connect_ex((host, port)) == 0


def _http_ok(path="/", timeout=2.0):
    try:
        with socket.create_connection((HOST, HTTP_PORT), timeout=timeout) as s:
            req = "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n" % (path, HOST)
            s.sendall(req.encode())
            data = s.recv(64)
        return data.startswith(b"HTTP/")
    except OSError:
        return False


def _wait_ports_free(timeout=15):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if not any(_port_open(p) for p in REQUIRED_PORTS):
            return True
        time.sleep(0.5)
    return False


def _port_owner_pid(port):
    """Best-effort PID of the process listening on `port` (None if unknown)."""
    try:
        out = subprocess.check_output(
            ["ss", "-ltnp"], stderr=subprocess.DEVNULL).decode()
    except (OSError, subprocess.CalledProcessError):
        return None
    for line in out.splitlines():
        if (":%d " % port) in line and "pid=" in line:
            try:
                return int(line.split("pid=", 1)[1].split(",", 1)[0])
            except (ValueError, IndexError):
                return None
    return None


def _reclaim_ports(verbose=False):
    """Kill a stale `wpt serve` still holding our ports.

    A lingering server answers health probes, which would otherwise fool us
    into "succeeding" against the wrong server while our own boot silently
    aborts on a port collision. Kill the whole process group of the listener.
    """
    pid = _port_owner_pid(HTTP_PORT)
    if pid is None:
        return
    if verbose:
        print("[wpt] reclaiming port %d from stale pid %d" % (HTTP_PORT, pid),
              file=sys.stderr)
    try:
        os.killpg(os.getpgid(pid), signal.SIGKILL)
    except OSError:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    _wait_ports_free(timeout=15)


class WptServerError(RuntimeError):
    pass


@contextmanager
def wpt_serve(wpt_root, inject_script=DEFAULT_INJECT, http2=False,
              startup_timeout=60, verbose=False):
    """Run `wpt serve` for the duration of the with-block.

    Raises WptServerError if the server fails to come up healthy.
    """
    wpt_bin = os.path.join(wpt_root, "wpt")
    if not os.path.isfile(wpt_bin):
        raise WptServerError("no wpt checkout at %s (missing ./wpt)" % wpt_root)

    # A previous server (healthy-looking or not) holding our ports would make
    # our own boot abort on a collision while health probes hit the stale one.
    if _port_open(HTTP_PORT):
        _reclaim_ports(verbose=verbose)

    cmd = [sys.executable, wpt_bin, "serve"]
    if not http2:
        cmd.append("--no-h2")
    if inject_script:
        cmd += ["--inject-script", inject_script]

    # Ensure wpt serve connects directly to loopback, bypassing any proxy.
    wpt_domains = ".web-platform.test,.not-web-platform.test"
    env = os.environ.copy()
    for key in ("no_proxy", "NO_PROXY"):
        existing = env.get(key, "")
        env[key] = (existing + "," + wpt_domains) if existing else wpt_domains

    log = open("/tmp/wpt_serve.log", "w")
    proc = subprocess.Popen(cmd, cwd=wpt_root, stdout=log, stderr=subprocess.STDOUT,
                            stdin=subprocess.DEVNULL, start_new_session=True,
                            env=env)
    try:
        good = 0
        deadline = time.time() + startup_timeout
        while time.time() < deadline:
            if proc.poll() is not None:
                raise WptServerError(
                    "wpt serve exited during startup (code %s); see /tmp/wpt_serve.log"
                    % proc.returncode)
            if _http_ok("/"):
                good += 1
                if good >= 3:
                    break
            else:
                good = 0
            time.sleep(1)
        else:
            raise WptServerError(
                "wpt serve did not become healthy within %ss" % startup_timeout)
        if verbose:
            print("[wpt] serving on http://%s:%d (pid %d)"
                  % (HOST, HTTP_PORT, proc.pid), file=sys.stderr)
        yield "http://%s:%d" % (HOST, HTTP_PORT)
    finally:
        _terminate(proc, verbose=verbose)
        log.close()


def _terminate(proc, verbose=False):
    if proc.poll() is None:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        except OSError:
            pass
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except OSError:
                pass
    if not _wait_ports_free(timeout=15) and verbose:
        print("[wpt] warning: ports still busy after teardown", file=sys.stderr)


def _main(argv):
    import argparse
    p = argparse.ArgumentParser(description="Run wpt serve on demand.")
    p.add_argument("--wpt-root", default=DEFAULT_WPT_ROOT,
                   help="path to the wpt checkout (default: third_party/wpt)")
    p.add_argument("--inject", default=DEFAULT_INJECT)
    p.add_argument("--duration", type=int, default=0,
                   help="seconds to stay up (0 = until Ctrl-C)")
    args = p.parse_args(argv)
    if not args.wpt_root:
        p.error("--wpt-root or WPT_ROOT is required")

    with wpt_serve(args.wpt_root, inject_script=args.inject, verbose=True):
        if args.duration:
            time.sleep(args.duration)
        else:
            print("Serving; press Ctrl-C to stop.", file=sys.stderr)
            try:
                while True:
                    time.sleep(3600)
            except KeyboardInterrupt:
                pass
    return 0


if __name__ == "__main__":
    sys.exit(_main(sys.argv[1:]))
