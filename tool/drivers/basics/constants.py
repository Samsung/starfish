#! /usr/bin/env python3

# Formatted by black.

import os


class ENVOPTS:
    TEST_RESULT_FILE = "TC_TEST_RESULT_FILE"
    FORCE_ENABLE = "TC_FORCE_ENABLE"
    TIMEOUT = "TC_TIMEOUT"
    REPLACE_STR = "TC_REPLACE_STR"

class ERRORCODE:
    TEST_PASSED = 0
    TEST_FAILED = 1
    TEST_STOPPED = 2

# Per-test-case subprocess timeout applied when TC_TIMEOUT is not set at all
# (e.g. every CI job driving tool/runner/test_runner.py today). Without this,
# a single Starfish test process that never exits (hang/deadlock, or host
# resource starvation right at the seam between two parallel test batches)
# blocks its worker forever with no recourse, which -- combined with
# multiprocessing.Pool.map_async(...).get() waiting for *every* worker to
# finish -- stalls the *entire* parallel batch until the outer CI job timeout
# kills it, producing zero diagnostic output. See parallel.py.
DEFAULT_TC_TIMEOUT_SEC = 60


def resolve_tc_timeout(default=DEFAULT_TC_TIMEOUT_SEC):
    """Resolve the effective per-test-case timeout (seconds), or None to
    disable it entirely.

    - TC_TIMEOUT unset -> `default` (fail-safe: CI must never run with an
      unbounded per-test timeout).
    - TC_TIMEOUT set to a positive number -> that value.
    - TC_TIMEOUT explicitly set to "0" (or negative) -> None, i.e. no
      timeout at all. This preserves an escape hatch for interactive
      debugging (e.g. stepping through a hung Starfish process under gdb).
    """
    raw = os.environ.get(ENVOPTS.TIMEOUT)
    if raw is None:
        return default
    value = float(raw)
    return value if value > 0 else None
