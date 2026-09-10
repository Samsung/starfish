# Common tools

Code shared by the test harnesses under `tool/`.

## Use it

Put `tool/` on `sys.path` first. That is also how `repo_paths` is reached.

```python
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # tool/

from common import storage
```

## Files

| File | What it holds |
| --- | --- |
| `storage.py` | Private test homes, and cleanup of what a run leaves |

## storage.py

Starfish keeps cookies, localStorage, IndexedDB and the code cache under
`$HOME`. One variable therefore isolates everything a test process persists.

| Call | What it does |
| --- | --- |
| `private_home(group)` | Returns a new home under `.tmp/<group>/<pid>/` |
| `owned_environment(base)` | Copies an environment and stamps this run's pid |
| `register_cleanup(group)` | Drops this run's directories at exit |
| `sweep_homes(group)` | Removes the directories of runs that are gone |
| `sweep_strays(marker)` | Kills owned processes whose owner is gone |
| `is_running(pid)` | Reports whether a pid is alive |

### Layout

`group` is any name the caller picks. It keeps one harness's directories
apart from another's.

```text
.tmp/
  test-cdp/                 a group
    1842884/                the pid of the run that made it
      tmpvvww9lzz/          one home, from private_home()
      tmp10h3jpg0/          another, for a second process
  test-cli/                 another group
    1842884/
      tmpykrw0d0r/
        home/               a caller may build anything inside its home
```

A run only ever removes its own pid directory. `sweep_homes` removes the pid
directories whose pid is gone, which is what a run stopped with Ctrl-C leaves.

### Ownership

Ownership does not follow the group. `OWNER_VARIABLE` is one name for every
caller, so whichever harness runs next also clears what an interrupted run of
another one left.

`sweep_strays` takes an optional command line marker. With one it only
considers processes whose command line contains it. Without one it considers
every process carrying `OWNER_VARIABLE`. Pass a marker when the processes are
recognizable that way, so the scan does not read the environment of every
process on the machine.

## Adding to this directory

Add something when two harnesses need the same behavior, not when two
harnesses look alike. One caller belongs in that caller's own directory.
