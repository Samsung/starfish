"""Private HOME directories for test processes, and cleanup of leftovers.

Starfish keeps cookies, localStorage, IndexedDB and the code cache under
$HOME, in one directory shared by every instance. Giving a process its own
HOME therefore isolates everything it persists. That is what lets two tests
run at the same time without reading each other's state, and what keeps a
test from seeing what an earlier test left behind.

Both test suites need the same three things, so they share this module:

    private_home(group)   a directory for one process to keep its profile in
    sweep_homes(group)    remove the directories of runs that are gone
    sweep_strays(marker)  kill test processes whose owning run is gone

The directories live under .tmp in the repository rather than /tmp, so what
a failing test wrote is easy to find. .tmp is git-ignored.

Groups keep the suites apart under .tmp. Ownership does not: OWNER_VARIABLE
is one name for every suite, so a sweep also clears what an interrupted run
of the other suite left behind.

The caller puts tool/ on sys.path before importing this, which is also how it
reaches repo_paths.
"""

import atexit
import os
import pathlib
import shutil
import signal
import tempfile

from repo_paths import REPO_ROOT

# Stamped into each test process's environment so a sweep can tell a leftover
# from a process that a concurrently running suite still owns.
OWNER_VARIABLE = "STARFISH_TEST_OWNER"

_TMP_ROOT = pathlib.Path(REPO_ROOT) / ".tmp"


def storage_root(group):
    return _TMP_ROOT / group


def is_running(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def private_home(group):
    """A directory for one process to keep its profile in.

    Grouped under the pid that made it, so a leftover can be matched to a run
    the same way a leftover process is, and callers cannot collide: mkdtemp
    picks the name.
    """
    owned = storage_root(group) / str(os.getpid())
    owned.mkdir(parents=True, exist_ok=True)
    return pathlib.Path(tempfile.mkdtemp(dir=owned))


def owned_environment(base=None):
    """Copy an environment and stamp this run as the owner."""
    environment = dict(os.environ if base is None else base)
    environment[OWNER_VARIABLE] = str(os.getpid())
    return environment


def register_cleanup(group):
    """Drop this run's directories when it exits.

    Call before arming anything that stops processes. atexit runs handlers in
    reverse order of registration, so registering this first removes the
    storage last, when nothing can still be writing to it.
    """
    atexit.register(drop_homes, group)


def drop_homes(group):
    """Give up this run's storage."""
    shutil.rmtree(storage_root(group) / str(os.getpid()), ignore_errors=True)
    _drop_empty_roots(group)


def _drop_empty_roots(group):
    """Remove the shared parents, but only while they are empty.

    rmdir fails on a directory another run is using, which is the point: two
    runs share these parents, and private_home recreates them, so losing the
    race costs nothing.
    """
    for path in (storage_root(group), _TMP_ROOT):
        try:
            path.rmdir()
        except OSError:
            break


def sweep_homes(group):
    """Remove the storage of runs that are gone, and report how many.

    Ctrl-C kills a runner without running atexit, so a directory left behind
    is normal. Only a dead owner's is taken: a concurrent run is still using
    its own.
    """
    removed = 0
    root = storage_root(group)
    if root.is_dir():
        for entry in root.iterdir():
            if not entry.name.isdigit() or is_running(int(entry.name)):
                continue
            shutil.rmtree(entry, ignore_errors=True)
            removed += 1
    _drop_empty_roots(group)
    return removed


def sweep_strays(marker=None):
    """Kill every marked test process whose owning run is gone.

    Ownership comes from OWNER_VARIABLE in the process's own environment, not
    from its parent. A parent check would misfire twice: a subreaper such as
    a container started with --init adopts orphans so they never look
    parentless, and at start-up a run owns nothing yet, so a concurrent
    suite's process would look like a leftover and be killed.

    marker narrows the search to command lines that contain it. Without one,
    every process carrying OWNER_VARIABLE is a candidate, which is what a
    suite whose processes have no recognizable command line needs.
    """
    killed = 0
    for entry in pathlib.Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            if marker is not None:
                if marker.encode() not in (entry / "cmdline").read_bytes():
                    continue
            owner = None
            for variable in (entry / "environ").read_bytes().split(b"\0"):
                name, _, value = variable.partition(b"=")
                if name.decode(errors="replace") == OWNER_VARIABLE:
                    owner = int(value)
                    break
            if owner is None:
                continue        # not started by any suite
            if is_running(owner):
                continue        # ours, or another suite still running
            os.kill(int(entry.name), signal.SIGKILL)
            killed += 1
        except (OSError, IndexError, ValueError):
            continue
    return killed
