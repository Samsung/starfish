#!/usr/bin/env python3

"""Read and write the list of what runs.

The file is both the run list and the result of the last run, so nothing has
to be joined against a second report to explain a skip.

    {
      "layer": "schema",
      "revision": "ea39a11d80de9a08ce2af03f52125ed2e462cf84",
      "counts": { "total": 661, "pass": 43, "fail": 42 },
      "entries": {
        "Browser.getVersion": { "expected": "pass" },
        "Animation.disable": {
          "expected": "fail",
          "category": "not-implemented",
          "detail": "Animation.disable returned {'code': -32601, ...}"
        }
      }
    }

An entry is a name: a CDP method for the schema layer, a test file path for
the behavior layer. It carries no expectation, so a list cannot weaken one.
Unknown names are rejected, so a list left stale by an update fails the run.

Keys are written sorted and indented one field per line, so a promotion or a
demotion shows up in a diff as the few lines that actually changed.
"""

import atexit
import json
import os

# A first differing line can be a base64 screenshot, which is worth neither
# the file size nor the diff. Cut it, and say so in the file.
DETAIL_LIMIT = 1000
CUT_MARKER = " ...(cut)"


def load(list_path, known):
    """Return (active, skipped) for a list file.

    active is a sorted list of names. skipped maps a name to its
    {"category", "detail"} record.
    """
    document = json.loads(list_path.read_text())
    entries = document.get("entries", {})
    unknown = [name for name in entries if name not in known]
    if unknown:
        raise ValueError("the list names %d entries that do not exist, for "
                         "example %s" % (len(unknown), ", ".join(unknown[:3])))
    active = sorted(name for name, record in entries.items()
                    if record.get("expected") == "pass")
    skipped = {name: {"category": record.get("category", ""),
                      "detail": record.get("detail", "")}
               for name, record in entries.items()
               if record.get("expected") == "fail"}
    return active, skipped


def names(list_path, known):
    """Every entry a file names, whatever status it gives them.

    Used for a subset run, where the file says what to run and nothing else.
    """
    entries = json.loads(list_path.read_text()).get("entries", {})
    unknown = [name for name in entries if name not in known]
    if unknown:
        raise ValueError("the list names %d entries that do not exist, for "
                         "example %s" % (len(unknown), ", ".join(unknown[:3])))
    return list(entries)


def revision_of(list_path):
    """The upstream revision the list was last written against."""
    return json.loads(list_path.read_text()).get("revision")


def save(list_path, layer_name, revision, active, skipped, counts):
    document = {
        "layer": layer_name,
        "revision": revision,
        "counts": counts,
        "entries": dict(
            sorted(
                [(name, {"expected": "pass"}) for name in active] +
                [(name, {"expected": "fail",
                         "category": record["category"],
                         "detail": _fit(record["detail"])})
                 for name, record in skipped.items()])),
    }
    # Written through a temporary file in the same directory, because a run
    # saves after every entry and may be killed mid-write. os.replace is
    # atomic, so the list is either the previous save or this one, never half
    # of both.
    # The name carries the pid, so two runs never share one temporary file:
    # the first os.replace would otherwise move it out from under the second,
    # which then fails on a file that no longer exists.
    temporary = list_path.with_name("%s.%d.partial"
                                    % (list_path.name, os.getpid()))
    try:
        temporary.write_text(json.dumps(document, indent=2, sort_keys=False,
                                        ensure_ascii=False) + "\n")
        os.replace(temporary, list_path)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise


def hold(list_path):
    """Claim a list for this run, or refuse to start.

    Two runs against one list each carry over what the file held when they
    read it, so the one that finishes last would write back the other's
    entries as they were before it started. Refusing the second run is the
    only way to keep both results.

    The claim is a file naming the owning pid. It is dropped at exit, and a
    claim whose owner is gone is taken over: Ctrl-C kills the runner without
    running atexit, so a stale claim is normal, not a sign of trouble.
    """
    lock_path = list_path.with_name(list_path.name + ".lock")
    for attempt in (1, 2):
        try:
            handle = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        except FileExistsError:
            owner = _owner_of(lock_path)
            if owner is not None and attempt == 1:
                raise SystemExit(
                    "Another run is already updating %s (pid %d).\n"
                    "Wait for it to finish, or stop it, and try again."
                    % (list_path.name, owner))
            # No live owner: drop the stale claim and take it on the retry.
            try:
                lock_path.unlink()
            except FileNotFoundError:
                pass
            continue
        os.write(handle, str(os.getpid()).encode())
        os.close(handle)
        atexit.register(_release, lock_path)
        return


def _owner_of(lock_path):
    """The live pid named by a claim, or None if it is stale or unreadable."""
    try:
        owner = int(lock_path.read_text().strip())
    except (OSError, ValueError):
        return None
    try:
        os.kill(owner, 0)            # Signal 0 only tests that the pid exists.
    except OSError:
        return None
    return owner


def _release(lock_path):
    try:
        lock_path.unlink()
    except FileNotFoundError:
        pass


def _fit(detail):
    """One line, bounded, so one huge failure cannot dominate the file."""
    flattened = " ".join(str(detail).split())
    if len(flattened) <= DETAIL_LIMIT:
        return flattened
    return flattened[:DETAIL_LIMIT] + CUT_MARKER
