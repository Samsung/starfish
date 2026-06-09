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

    python3 tool/wpt_runner.py tool/wpt/lists --wpt-root /path/to/wpt
    python3 tool/wpt_runner.py tool/wpt/lists/dom_basic.res -j8
"""

import os
import re
import subprocess
import sys
from argparse import ArgumentParser
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wpt_server import wpt_serve, DEFAULT_WPT_ROOT  # noqa: E402

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STARFISH = os.path.join(REPO_ROOT, "Starfish")

RE_PASS = re.compile(r"WPTR PASS (.*)")
RE_FAIL = re.compile(r"WPTR FAIL (.*)")
RE_DONE = re.compile(r"WPTR DONE status=(\d+) count=(\d+)")

GREEN, RED, YEL, RST = "\033[92m", "\033[91m", "\033[93m", "\033[0m"


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


def run_one(url, timeout):
    """Return (ok, reason, npass_subtests, nfail_subtests)."""
    cmd = [STARFISH, url, "--hide-window", "--width=800", "--height=600"]
    env = dict(os.environ)
    env["HIDE_WINDOW"] = "1"
    try:
        out = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             env=env, timeout=timeout).stdout.decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT", 0, 0
    except OSError:
        return False, "SHELL_ERROR", 0, 0

    npass = len(RE_PASS.findall(out))
    nfail = len(RE_FAIL.findall(out))
    done = RE_DONE.search(out)
    if done is None:
        return False, "NO_COMPLETION", npass, nfail
    status, count = int(done.group(1)), int(done.group(2))
    if status != 0:
        return False, "HARNESS_STATUS_%d" % status, npass, nfail
    if count == 0:
        return False, "NO_SUBTESTS", npass, nfail
    if nfail:
        return False, "SUBTESTS_FAILED", npass, nfail
    return True, "OK", npass, nfail


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


def run_all(items, jobs, timeout, results_path, append=False):
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
        ok, reason, np, nf = run_one(url, timeout)
        return name, url, ok, reason, np, nf

    with ThreadPoolExecutor(max_workers=jobs) as ex:
        for name, url, ok, reason, np, nf in ex.map(task, items):
            done_n += 1
            pl = per_list.setdefault(name, [0, 0])
            pl[1] += 1
            if ok:
                npass += 1
                pl[0] += 1
            else:
                reasons[reason] += 1
            # Per-test line in the legacy multi_basic format so failures are
            # identifiable: "[PASS] <url> (PASS: n)" / "[FAIL] <url> (...)".
            if ok:
                print("%s[PASS] %s%s (%sPASS: %d%s)"
                      % (GREEN, RST, url, GREEN, np, RST))
            elif reason == "SUBTESTS_FAILED":
                print("%s[FAIL] %s%s (%sPASS: %d%s, %sFAIL: %d%s)"
                      % (RED, RST, url, GREEN, np, RST, RED, nf, RST))
            else:
                print("%s[FAIL] %s%s (%s%s%s)"
                      % (RED, RST, url, RED, reason, RST))
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
    args = p.parse_args(argv)
    if not args.no_serve and not args.wpt_root:
        p.error("--wpt-root or WPT_ROOT is required (or pass --no-serve)")

    items = collect(args.res, args.force)
    if args.resume:
        done = load_done(args.results)
        items = [it for it in items if it[1] not in done]
        print("Resuming: %d already done, %d remaining" % (len(done), len(items)))
    print("Running %d WPT tests (%d-way parallel) from %s"
          % (len(items), args.jobs, args.res))

    def go():
        return run_all(items, args.jobs, args.timeout, args.results,
                       append=args.resume)

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
