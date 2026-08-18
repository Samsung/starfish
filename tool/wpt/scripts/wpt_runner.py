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

"""Run WPT testharness tests against an on-demand `wpt serve`.

Reads a `.res` list (or a directory of them) of WPT URLs, runs each in the
Starfish shell in parallel, and judges the result from the `WPTR ` lines
emitted by tool/wpt/inject_report.js:

    WPTR PASS <name>
    WPTR FAIL <name>
    WPTR DONE status=<0|1|2|3> count=<n>

Verdict per test:
    PASS  -> harness completed cleanly (status=0), >=1 subtest, no FAIL subtests
    FAIL  -> any FAIL subtest, harness status!=0, no completion, or shell crash

Needs no stored expected `.txt` files, so it works without the internal
`test/` submodule.

    python3 tool/wpt/scripts/wpt_runner.py \
        tool/wpt/testharness_lists --wpt-root /path/to/wpt
    python3 tool/wpt/scripts/wpt_runner.py \
        tool/wpt/testharness_lists/dom_basic.res -j8
"""

import contextlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from argparse import ArgumentParser
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlsplit, urlunsplit

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)                                      # sibling wpt_*
sys.path.insert(0, os.path.join(_HERE, os.pardir, os.pardir))  # tool/
from repo_paths import REPO_ROOT  # noqa: E402
from wpt_server import wpt_serve, DEFAULT_WPT_ROOT  # noqa: E402
from wpt_reftest import (run_reftest, load_manifest, ensure_manifest,  # noqa: E402
                         ensure_imgdiff)

STARFISH = os.path.join(REPO_ROOT, "Starfish")
TMP_DIR = "/tmp"

# glibc's stdio is fully block-buffered (not line-buffered) whenever stdout
# isn't a tty -- true for every subprocess.PIPE capture here. On a hang, the
# tail of Starfish's own output (often the one line that would explain the
# hang, e.g. the exact curl error right before it stalls) can sit in that
# unflushed buffer and be lost entirely when the process is SIGKILLed on
# timeout, leaving a captured log that looks like it just stops for no
# reason. `stdbuf -oL -eL` forces line buffering from the outside with no
# Starfish source change needed.
STARFISH_CMD_PREFIX = ["stdbuf", "-oL", "-eL"]


@contextlib.contextmanager
def isolated_storage_dir():
    """Give one Starfish invocation its own private localStorage/cookies/
    HTTP-cache directory instead of the default $HOME/Starfish-storage.

    Without this, every parallel worker (this suite runs with jobs=8, see
    _wpt_serve_run's default) fights over the same on-disk cache dir lock
    ("HTTPCache.cpp: Failed to lock cache dir" / "Failed to create(or open)
    cache dir"). A worker that loses the race just falls back to running
    with caching disabled for that one page load -- usually harmless -- but
    under enough contention it can also leave a page stuck mid-navigation
    with no WPTR output at all, which surfaces here as an unrelated-looking
    flaky TIMEOUT (root-caused live: workers/interfaces/WorkerUtils/
    navigator/007.html). A fresh directory per invocation removes the lock
    contention entirely instead of chasing the exact wedge it causes.
    """
    d = tempfile.mkdtemp(prefix="starfish-storage-")
    try:
        yield d
    finally:
        shutil.rmtree(d, ignore_errors=True)


RE_PASS = re.compile(r"WPTR PASS (.*)")
RE_FAIL = re.compile(r"WPTR FAIL (.*)")
RE_DONE = re.compile(r"WPTR DONE status=(\d+) count=(\d+)")
RE_CRASHOK = re.compile(r"WPTR CRASHOK")

# Query marker inject_report.js checks for before entering the crashtest path,
# so a crashtest run never accidentally engages the testharness completion
# poll on a page that also happens to load testharness.js (or vice versa).
# NOTE: the matching literal lives in tool/wpt/inject_report.js's crashtest
# gate regex -- keep the two in sync (can't share a constant across Py/JS).
CRASHTEST_QUERY = "__starfish_crashtest=1"

GREEN, RED, YEL, RST = "\033[92m", "\033[91m", "\033[93m", "\033[0m"


def reason_category(reason):
    """Collapse a verdict reason to its leading category token.

    Reasons may carry a free-text suffix ("IMGDIFF_ERROR: <msg>") or a
    parenthesized sub-reason ("REF_LOAD_FAIL(TIMEOUT)"); the category is the
    bit before the first ':'. Shared by the failure-reason histogram here and
    by wpt_annotate.py's marker (`# [auto-fail:<category>]`) so the two never
    disagree on how a reason is bucketed.
    """
    return reason.split(":", 1)[0].strip()


def read_res(path, force=False):
    urls = []
    with open(path) as fp:
        for line in fp:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                if not force:
                    continue
                # --force re-includes commented entries. These may carry an
                # annotation marker before the URL (e.g. "# [auto-fail] http..."
                # from wpt_annotate.py), so pick the first http token rather
                # than assuming the URL follows the '#' directly.
                toks = [t for t in line.split() if t.startswith("http")]
                if toks:
                    urls.append(toks[0])
                continue
            if line.startswith("http"):
                urls.append(line.split()[0])
    return urls


def collect(path, force):
    """Return [(list_name, url), ...] from a .res file or a dir of them."""
    items = []
    if os.path.isdir(path):
        for name in sorted(os.listdir(path)):
            if name.endswith(".res"):
                for u in read_res(os.path.join(path, name), force):
                    items.append((name, u))
    else:
        name = os.path.basename(path)
        for u in read_res(path, force):
            items.append((name, u))
    return items


# Fingerprint of the local wpt-serve HTTP server's own connection-accept path
# starving under host CPU contention: Starfish's initial navigation itself
# fails with libcurl error 7 (CURLE_COULDNT_CONNECT) -- see
# src/platform/network/http -- so the page body never arrives, testharness.js
# never runs, and the shell just idles (no WPTR output at all) until this
# module's own external subprocess timeout kills it. Confirmed by direct
# reproduction: pinning the whole suite to 2 cores plus CPU-burner processes
# on those same cores reproduces this exact "failed to open[7] <url>" line,
# with no WPTR anywhere in the capture, on an otherwise-passing test's own
# page fetch -- indiscriminately, on whichever test's process happens to race
# a starved wpt-serve accept() that run. Not an engine hang (those don't
# print this), so retrying is safe: it can only mask this one specific,
# already-transient infra hiccup, never a real timeout/deadlock in Starfish.
RE_CONNECT_REFUSED = re.compile(r"failed to open\[7\] (\S+)")

# Small bounded retry (not infinite) for the hiccup above -- see
# RE_CONNECT_REFUSED and _is_connect_refused_on_navigation. Raised from the
# original 2 -- under heavier host contention than the original fix was
# tuned against, 2 retries wasn't always enough to outlast the wpt-serve
# accept() starvation window.
CONNECT_REFUSED_RETRIES = 5


def _is_connect_refused_on_navigation(log, url):
    """True if `log` shows the *top-level* navigation to `url` itself failed
    with CURLE_COULDNT_CONNECT and the page never got far enough to run
    testharness.js at all (no WPTR output whatsoever). See
    RE_CONNECT_REFUSED's comment for why this is safe to retry.
    """
    if "WPTR" in log:
        return False
    m = RE_CONNECT_REFUSED.search(log)
    return m is not None and m.group(1) == url


def run_one(url, timeout, _retries=CONNECT_REFUSED_RETRIES):
    """Return (ok, reason, npass_subtests, nfail_subtests, log).

    log carries the captured Starfish stdout+stderr (crash backtraces print
    to stdout, see src/shell/Shell.cpp) for the crash-ish reasons (NO_COMPLETION,
    TIMEOUT, SHELL_ERROR); it is None for the logical HARNESS_STATUS_*/
    NO_SUBTESTS/SUBTESTS_FAILED/OK outcomes, which have no crash to show.

    _retries: bounded retries left for the connect-refused-on-navigation
    infra hiccup (RE_CONNECT_REFUSED); 0 disables retrying.
    """
    env = dict(os.environ)
    env["HIDE_WINDOW"] = "1"
    wpt_domains = ".web-platform.test,.not-web-platform.test"
    for key in ("no_proxy", "NO_PROXY"):
        existing = env.get(key, "")
        env[key] = (existing + "," + wpt_domains) if existing else wpt_domains
    try:
        with isolated_storage_dir() as storage_dir:
            cmd = STARFISH_CMD_PREFIX + [STARFISH, url, "--hide-window", "--width=800", "--height=600",
                                         "--storage-dir=" + storage_dir]
            out = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 env=env, timeout=timeout).stdout.decode("utf-8", "replace")
    except subprocess.TimeoutExpired as e:
        log = (e.output or b"").decode("utf-8", "replace")
        if _retries > 0 and _is_connect_refused_on_navigation(log, url):
            print("  %s[RETRY]%s %s (connect refused on navigation, "
                  "%d retries left)" % (YEL, RST, url, _retries))
            return run_one(url, timeout, _retries - 1)
        return False, "TIMEOUT", 0, 0, log
    except OSError:
        return False, "SHELL_ERROR", 0, 0, None

    npass = len(RE_PASS.findall(out))
    nfail = len(RE_FAIL.findall(out))
    done = RE_DONE.search(out)
    if done is None:
        return False, "NO_COMPLETION", npass, nfail, out
    status, count = int(done.group(1)), int(done.group(2))
    if status != 0:
        return False, "HARNESS_STATUS_%d" % status, npass, nfail, None
    if count == 0:
        return False, "NO_SUBTESTS", npass, nfail, None
    if nfail:
        return False, "SUBTESTS_FAILED", npass, nfail, None
    return True, "OK", npass, nfail, None


def run_one_reftest(url, timeout, manifest):
    """Return (ok, reason, 0, 0, log).

    See tool/wpt/scripts/wpt_reftest.py for the mechanism.
    """
    ok, reason, log = run_reftest(url, manifest=manifest, timeout=timeout,
                                  tmp_dir=TMP_DIR)
    return ok, reason, 0, 0, log


def _with_crashtest_marker(url):
    """Insert CRASHTEST_QUERY into url's query component, fragment-safe.

    A plain string append (url + "?" + marker) would land the marker after a
    "#fragment" instead of in the query, so inject_report.js's
    location.search check would never see it. No current crashtest URL in the
    corpus has a fragment, but this is nearly free to get right.
    """
    parts = urlsplit(url)
    query = parts.query + ("&" if parts.query else "") + CRASHTEST_QUERY
    return urlunsplit((parts.scheme, parts.netloc, parts.path, query, parts.fragment))


def run_one_crashtest(url, timeout):
    """Return (ok, reason, 0, 0, log).

    A crashtest has no testharness.js, so completion is signaled by
    inject_report.js's crashtest path (see tool/wpt/inject_report.js) instead
    of the WPTR DONE contract: it waits for the WPT `test-wait` class to be
    gone from <html>, then prints WPTR CRASHOK. The query marker keeps that
    path from engaging on ordinary testharness runs.

    log carries the captured Starfish stdout+stderr (crash backtraces print
    to stdout, see src/shell/Shell.cpp) for the crash-ish reasons; None for OK.
    """
    gated_url = _with_crashtest_marker(url)
    env = dict(os.environ)
    env["HIDE_WINDOW"] = "1"
    wpt_domains = ".web-platform.test,.not-web-platform.test"
    for key in ("no_proxy", "NO_PROXY"):
        existing = env.get(key, "")
        env[key] = (existing + "," + wpt_domains) if existing else wpt_domains
    try:
        with isolated_storage_dir() as storage_dir:
            cmd = STARFISH_CMD_PREFIX + [STARFISH, gated_url, "--hide-window", "--width=800", "--height=600",
                                         "--storage-dir=" + storage_dir]
            r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               env=env, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        return False, "TIMEOUT", 0, 0, (e.output or b"").decode("utf-8", "replace")
    except OSError:
        return False, "SHELL_ERROR", 0, 0, None
    out = r.stdout.decode("utf-8", "replace")
    if r.returncode < 0:
        # Distinct from reftest's "TC_CRASH" (tool/wpt/scripts/wpt_reftest.py's
        # _screenshot, a broader "the render didn't come out right" bucket
        # inherited from the legacy wpt_test.py driver) -- this specifically
        # means the shell was killed by a signal, which is exactly the
        # condition a crashtest exists to detect.
        return False, "SIGNAL_CRASH", 0, 0, out
    if RE_CRASHOK.search(out):
        return True, "OK", 0, 0, None
    return False, "NO_COMPLETION", 0, 0, out


# Reason categories that mean "the Starfish process itself misbehaved"
# (crashed, hung, never signaled completion) as opposed to a logical
# pass/fail verdict (SUBTESTS_FAILED, IMG_MISMATCH, ...) -- only these carry
# a captured process log worth printing under --verbose. REF_LOAD_FAIL wraps
# one of these same tokens for the reference page (e.g. "REF_LOAD_FAIL(TC_CRASH)"),
# so the check strips both a "(...)" wrapper and a ": msg" suffix before
# matching, unlike reason_category() above which only strips the latter.
CRASH_REASONS = frozenset((
    "TC_CRASH", "SIGNAL_CRASH", "REF_LOAD_FAIL", "SHELL_ERROR",
    "NO_COMPLETION", "TIMEOUT", "INTERNAL_ERROR",
))


def _is_crash_reason(reason):
    return reason.split("(", 1)[0].split(":", 1)[0].strip() in CRASH_REASONS


def _tail_lines(text, n):
    """Return the last n lines of text, or all of it if n <= 0 or it fits."""
    if not text or n <= 0:
        return text
    lines = text.splitlines()
    if len(lines) <= n:
        return text
    return "\n".join(lines[-n:])


def load_done(results_path):
    """URLs already recorded in a results file (for --resume)."""
    done = set()
    if results_path and os.path.isfile(results_path):
        with open(results_path) as f:
            for line in f:
                parts = line.rstrip("\n").split("\t")
                if len(parts) == 3:
                    done.add(parts[2])
    return done


def run_all(items, jobs, timeout, results_path, append=False,
           mode="testharness", manifest=None, verbose=False, log_lines=100):
    """Run items and print verdicts.

    verbose: also print the captured Starfish output (tail-bound to
    log_lines, 0=unbounded) under a crash-ish FAIL (see CRASH_REASONS) --
    this is where a SIGSEGV/SIGABRT backtrace (src/shell/Shell.cpp) would
    otherwise be silently discarded. Off by default so a clean gating run
    stays as quiet as before; --results is unaffected either way (still the
    plain 3-column PASS/FAIL\treason\turl format wpt_annotate.py expects).
    """
    total = len(items)
    npass = 0
    reasons = Counter()
    per_list = {}  # name -> [pass, total]
    done_n = 0
    # Line-buffered + append so partial progress survives a hard kill / resume.
    results_fp = open(results_path, "a" if append else "w", buffering=1) \
        if results_path else None

    def task(item):
        name, url = item
        try:
            if mode == "reftest":
                ok, reason, np, nf, log = run_one_reftest(url, timeout, manifest)
            elif mode == "crashtest":
                ok, reason, np, nf, log = run_one_crashtest(url, timeout)
            else:
                ok, reason, np, nf, log = run_one(url, timeout)
        except Exception as e:
            # Backstop: ThreadPoolExecutor.map() re-raises a worker exception
            # when its result is consumed, which would abort this whole
            # `for ... in ex.map(...)` loop and lose every not-yet-flushed
            # result. One bad item (e.g. an unexpected crash deep in a mode's
            # subprocess handling) must not take down a multi-thousand-item
            # batch -- record it as this item's own failure instead.
            ok, reason, np, nf, log = False, "INTERNAL_ERROR: %s" % e, 0, 0, None
        return name, url, ok, reason, np, nf, log

    with ThreadPoolExecutor(max_workers=jobs) as ex:
        for name, url, ok, reason, np, nf, log in ex.map(task, items):
            done_n += 1
            pl = per_list.setdefault(name, [0, 0])
            pl[1] += 1
            if ok:
                npass += 1
                pl[0] += 1
            else:
                # Bucket by category (text before the first ":"), not the raw
                # reason: IMGDIFF_ERROR/INTERNAL_ERROR embed a per-failure
                # exception message, so without this every occurrence would
                # be its own one-off Counter key -- defeating the point of a
                # failure-reason histogram. The full message is still printed
                # per-test below and written in full to --results.
                reasons[reason_category(reason)] += 1
            # Per-test line in the legacy multi_basic format so failures are
            # identifiable: "[PASS] <url> (PASS: n)" / "[FAIL] <url> (...)".
            # reftest/crashtest have no subtests (np/nf are always 0 there),
            # so skip the "(PASS: n)" suffix for those modes.
            if mode == "testharness" and ok:
                print("%s[PASS] %s%s (%sPASS: %d%s)"
                      % (GREEN, RST, url, GREEN, np, RST))
            elif mode == "testharness" and reason == "SUBTESTS_FAILED":
                print("%s[FAIL] %s%s (%sPASS: %d%s, %sFAIL: %d%s)"
                      % (RED, RST, url, GREEN, np, RST, RED, nf, RST))
            elif ok:
                print("%s[PASS] %s%s" % (GREEN, RST, url))
            else:
                print("%s[FAIL] %s%s (%s%s%s)"
                      % (RED, RST, url, RED, reason, RST))
            if verbose and not ok and log and _is_crash_reason(reason):
                tail = _tail_lines(log, log_lines)
                total = len(log.splitlines())
                shown = len(tail.splitlines())
                # Report what was actually printed, not the configured cap:
                # a short-lived crash/timeout may capture fewer lines than
                # log_lines, in which case "(last N lines)" would falsely
                # imply N lines are always shown.
                if shown < total:
                    header = "  --- Starfish output (last %d of %d lines) ---" \
                        % (shown, total)
                else:
                    header = "  --- Starfish output (%d lines) ---" % total
                print(header)
                for line in tail.splitlines():
                    print("  | %s" % line)
            if results_fp:
                results_fp.write("%s\t%s\t%s\n"
                                 % ("PASS" if ok else "FAIL", reason, url))
    if results_fp:
        results_fp.close()
    return npass, reasons, per_list


def main(argv):
    p = ArgumentParser(description=__doc__)
    p.add_argument("res", help=".res file or directory of .res files")
    p.add_argument("--wpt-root", default=DEFAULT_WPT_ROOT,
                   help="path to the wpt checkout (default: third_party/wpt)")
    p.add_argument("-j", "--jobs", type=int, default=8, help="parallel shells")
    p.add_argument("--timeout", type=int, default=15, help="per-test seconds")
    p.add_argument("-f", "--force", action="store_true",
                   help="also run commented-out (#) entries")
    p.add_argument("--results", default=None, help="write per-test verdicts here")
    p.add_argument("--resume", action="store_true",
                   help="skip URLs already in --results and append the rest")
    p.add_argument("--no-serve", action="store_true",
                   help="assume a server is already running")
    p.add_argument("--mode", choices=("testharness", "reftest", "crashtest"),
                   default="testharness",
                   help="test kind the .res list(s) contain (default: "
                        "testharness)")
    p.add_argument("-v", "--verbose", action="store_true",
                   help="print captured Starfish output (crash backtrace etc.) "
                        "for crash-ish FAILs (TC_CRASH, SIGNAL_CRASH, "
                        "REF_LOAD_FAIL, TIMEOUT, NO_COMPLETION, SHELL_ERROR, "
                        "INTERNAL_ERROR)")
    p.add_argument("--log-lines", type=int, default=100,
                   help="tail this many lines of --verbose output (0=unbounded, "
                        "default: 100)")
    args = p.parse_args(argv)
    if not args.no_serve and not args.wpt_root:
        p.error("--wpt-root or WPT_ROOT is required (or pass --no-serve)")

    manifest = None
    if args.mode == "reftest":
        ensure_imgdiff()
        ensure_manifest(args.wpt_root, os.path.join(args.wpt_root, "MANIFEST.json"))
        manifest = load_manifest(args.wpt_root)

    items = collect(args.res, args.force)
    if args.resume:
        done = load_done(args.results)
        items = [it for it in items if it[1] not in done]
        print("Resuming: %d already done, %d remaining" % (len(done), len(items)))
    print("Running %d WPT %s tests (%d-way parallel) from %s"
          % (len(items), args.mode, args.jobs, args.res))

    def go():
        return run_all(items, args.jobs, args.timeout, args.results,
                       append=args.resume, mode=args.mode, manifest=manifest,
                       verbose=args.verbose, log_lines=args.log_lines)

    if args.no_serve:
        npass, reasons, per_list = go()
    else:
        with wpt_serve(args.wpt_root, verbose=True):
            npass, reasons, per_list = go()

    total = len(items)
    if len(per_list) > 1:
        print("\n%-46s %7s %7s %6s" % ("list", "pass", "total", "%"), )
        print("-" * 70)
        for name in sorted(per_list):
            pn, tn = per_list[name]
            rate = 100.0 * pn / tn if tn else 0.0
            color = GREEN if pn == tn else (RED if pn == 0 else YEL)
            print("%s%-46s %7d %7d %5.0f%%%s" % (color, name, pn, tn, rate, RST))
        print("-" * 70)

    rate = 100.0 * npass / total if total else 0.0
    print("\n%sPASS %d/%d (%.1f%%)   FAIL %d%s"
          % (YEL, npass, total, rate, total - npass, RST))
    if reasons:
        print("Failure reasons:")
        for reason, n in reasons.most_common():
            print("  %5d  %s" % (n, reason))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
