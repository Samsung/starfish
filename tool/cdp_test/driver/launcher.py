#!/usr/bin/env python3

"""Find a CDP-enabled Starfish build and run one process per case.

Every case gets a fresh process on a fresh port. That is slower than reusing
one browser, but it keeps a case that wedges or crashes Starfish from
corrupting the next case's result.
"""

import atexit
import contextlib
import ctypes
import os
import pathlib
import re
import shutil
import signal
import socket
import subprocess
import time

from common import storage

from . import client

# Stamped into the fixture page so an orphan can be recognised in /proc. The
# page also has to keep the process alive, hence the timer.
MARKER = "starfish-cdp-test"
# Names this suite's directories under .tmp. Ownership is not per suite: see
# OWNER_VARIABLE in common/storage.py.
STORAGE_GROUP = "test-cdp"
TEST_PAGE = ("data:text/html,<!--%s-->"
             "<script>setInterval(function(){}, 300)</script>" % MARKER)

# The sweep finds leftovers by this string on the command line. Losing it
# would disable cleanup silently, so fail loudly at import instead.
if MARKER not in TEST_PAGE:
    raise RuntimeError("TEST_PAGE must carry MARKER; orphan cleanup finds "
                       "leftover browsers by it")
XVFB_SCREEN = "-screen 0 1920x1080x24"
# setpriv --pdeathsig asks the kernel to kill the browser when this run dies,
# which is the only defence that survives a SIGKILL of the runner. It sets the
# signal from outside, so nothing of ours has to run between fork and exec.
# Looked up once: which() is a PATH scan, and a sweep starts a browser per
# entry. None means no setpriv, and the fork-time fallback is used instead.
SETPRIV = shutil.which("setpriv")

# Whether to give the browser a virtual display of its own. Off by default: a
# headless build needs none, and running Starfish as our direct child is what
# lets the kernel kill it together with us. An x11 build needs this unless
# the environment already has a working DISPLAY. There is no fallback, so a
# build that needs a display and was not given one fails instead of quietly
# behaving differently. Set once at start-up.
virtual_display = False
PR_SET_PDEATHSIG = 1

# Browsers started by this process and not yet stopped.
_live = set()
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent


def _repository_dir():
    """Walk up to the repository root, identified by its CMakeLists.txt.

    Searching beats counting parent directories: this file has already moved
    twice, and a wrong depth would silently look for builds in the wrong out/.
    """
    for parent in SCRIPT_DIR.parents:
        if (parent / "CMakeLists.txt").is_file():
            return parent
    raise RuntimeError("could not locate the repository root above %s" %
                       SCRIPT_DIR)


REPOSITORY_DIR = _repository_dir()


def find_free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


def find_starfish():
    """Pick the one out/ build configured with STARFISH_ENABLE_CDP=1.

    Refuses to guess: zero or several equally good matches raise, so the
    caller must pass --starfish instead of silently testing the wrong build.

    TODO: ask the binary for its build information instead of reading the
    cache. CMakeCache.txt records what was configured, not what was built, so
    a tree that was configured but never built, or configured again without a
    rebuild, is classified wrong. A --version that prints the backend, the
    shell and whether CDP is compiled in would also let a run state which
    build it tested.
    """
    candidates = []
    headless_candidates = []
    for cache_path in (REPOSITORY_DIR / "out").glob("*/CMakeCache.txt"):
        cache = cache_path.read_text(errors="replace")
        if not re.search(r"STARFISH_ENABLE_CDP(?::[^=]+)?=(1|ON|TRUE)$",
                         cache, re.MULTILINE):
            continue
        # The executable is named after the cached TARGETNAME, which differs
        # per configuration: "Starfish" locally, "lightweight-web-engine" in
        # the Docker CI build.
        target_name = re.search(r"^TARGETNAME(?::[^=]+)?=(.+)$", cache,
                                re.MULTILINE)
        if not target_name:
            continue
        binary = cache_path.parent / "bin" / target_name.group(1)
        if binary.is_file():
            candidates.append(binary)
            backend = re.search(r"^BACKEND(?::[^=]+)?=(.+)$", cache,
                                re.MULTILINE)
            shell = re.search(r"^SHELL(?::[^=]+)?=(.+)$", cache,
                              re.MULTILINE)
            if (backend and shell and
                    backend.group(1).endswith("_headless") and
                    shell.group(1).endswith("_headless")):
                headless_candidates.append(binary)
    if len(headless_candidates) == 1:
        return headless_candidates[0]
    if len(candidates) == 1:
        return candidates[0]
    # Neither case is a bug in this tool, so report it the way require() does:
    # a plain message and a non-zero exit, never a traceback.
    if not candidates:
        raise SystemExit(
            "No CDP-enabled build was found under %s.\n"
            "Build one with STARFISH_ENABLE_CDP=1, or name a binary:\n"
            "  tool/cdp_test/run.py <command> --starfish <path>"
            % (REPOSITORY_DIR / "out"))
    raise SystemExit(
        "Several CDP-enabled builds were found:\n%s\n"
        "Name the one to test:\n"
        "  tool/cdp_test/run.py <command> --starfish <path>"
        % "\n".join("  %s" % binary for binary in sorted(candidates)))


def stop(process):
    if process.poll() is not None:
        return
    # Starfish is started in its own session, so signal the whole group.
    # Under --virtual-display the xvfb-run wrapper is the direct child, and
    # killing only that would leave the browser behind.
    #
    # The group can exit between poll() and killpg. This runs in a finally
    # block, so an unguarded ProcessLookupError would replace the failure the
    # caller is actually reporting.
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        process.wait()
        return
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait()


def wait_for_transport(endpoint, timeout):
    """Poll a freshly spawned browser until its CDP endpoint answers."""
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
        try:
            return client.probe_transport(endpoint, timeout)
        except (OSError, ValueError) as error:
            last_error = error
            time.sleep(0.1)
    raise RuntimeError(
        "CDP endpoint did not become ready: %s%s"
        % (last_error,
           "" if virtual_display
           else "; an x11 build needs a DISPLAY or --virtual-display"))


def _set_parent_death_signal():
    """Ask the kernel to kill this child when the runner dies.

    The fallback for a machine without setpriv. It runs in the child between
    fork and exec, which Python documents as unsafe in a program that has
    threads: a lock another thread held at fork time is held forever in the
    child, so an allocation here can hang before exec is reached. Used anyway
    when there is no setpriv, because losing this defence entirely is worse.
    """
    try:
        ctypes.CDLL("libc.so.6", use_errno=True).prctl(
            PR_SET_PDEATHSIG, signal.SIGKILL)
    except OSError:
        pass                    # Not Linux, or no libc; the other layers hold.


def _stop_all():
    for process in list(_live):
        stop(process)


def _on_signal(number, _frame):
    """Stop browsers, then die the way we were asked to.

    Without this a SIGTERM to the runner skips every finally block and leaves
    the browser it had open behind.
    """
    _stop_all()
    signal.signal(number, signal.SIG_DFL)
    os.kill(os.getpid(), number)


def install_cleanup():
    """Arm the cleanup paths. Call once, before the first browser starts."""
    # Registered first so it runs last: no browser may still be writing to the
    # storage when it goes. See register_cleanup for the ordering rule.
    storage.register_cleanup(STORAGE_GROUP)
    atexit.register(_stop_all)
    for number in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(number, _on_signal)


def sweep_storage():
    """Remove the storage of runs that are gone, and report how many."""
    return storage.sweep_homes(STORAGE_GROUP)


def sweep_strays():
    """Kill every marked browser whose owning run is gone."""
    return storage.sweep_strays(MARKER)


@contextlib.contextmanager
def running(starfish, timeout):
    """Start Starfish on the fixture page and yield its CDP endpoint.

    The process is always stopped, including on assertion failure or timeout,
    so a failing case cannot leak a browser into the next one.
    """
    command = [str(starfish), TEST_PAGE, "--disable-console"]
    if virtual_display:
        # xvfb-run gives the browser a throwaway X server and picks a free
        # display number, which is what the rest of this repository uses.
        xvfb_run = shutil.which("xvfb-run")
        if not xvfb_run:
            raise RuntimeError("--virtual-display was given but xvfb-run "
                               "is not installed")
        command = [xvfb_run, "-s", XVFB_SCREEN, "-a"] + command
    # Outermost, so the parent-death signal lands on our direct child whether
    # that is the browser or the xvfb-run wrapper around it.
    if SETPRIV:
        command = [SETPRIV, "--pdeathsig", "SIGKILL"] + command

    port = find_free_port()
    home = storage.private_home(STORAGE_GROUP)
    environment = storage.owned_environment()
    environment["STARFISH_ENABLE_CDP"] = "1"
    environment["STARFISH_CDP_PORT"] = str(port)
    # Everything Starfish persists hangs off HOME, so one variable isolates
    # the lot. Set for the browser only: node keeps the real environment.
    environment["HOME"] = str(home)
    process = subprocess.Popen(
        command,
        env=environment,
        start_new_session=True,
        # Only when setpriv is missing, because preexec_fn runs between fork
        # and exec: Python documents it as unsafe once the program has
        # threads, and --workers gives it threads.
        preexec_fn=None if SETPRIV else _set_parent_death_signal,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    _live.add(process)
    try:
        endpoint = "http://127.0.0.1:%d" % port
        # Discovery and Schema.getDomains must answer before any case runs.
        # Every case therefore covers the transport path implicitly.
        wait_for_transport(endpoint, timeout)
        yield endpoint
    finally:
        stop(process)
        _live.discard(process)
        # After the browser is gone, so nothing is still writing to it.
        shutil.rmtree(home, ignore_errors=True)
