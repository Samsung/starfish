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

"""Generate an overall WPT status report for Starfish.

Unlike tool/wpt_runner.py (which runs the CI-gate `.res` lists, always ~100%
pass), this runs *un-curated* spec directories so the report reveals where
Starfish is strong or weak per spec area.

It enumerates testharness tests from third_party/wpt/MANIFEST.json for each
directory in tool/wpt_status_targets.txt, runs them in the Starfish shell under
an on-demand `wpt serve`, and writes a self-contained HTML report grouped by
spec category. No external reporting dependency (mozlog / wptrunner) is used.

The per-test result keeps each subtest's name and status, so a future
`render_wptreport()` (wpt.fyi format) can be added without re-running anything.

    xvfb-run -s '-screen 0 1920x1080x24' -a \
      python3 tool/wpt_status.py --only css/selectors --limit 30 -o report.html
    xvfb-run -s '-screen 0 1920x1080x24' -a \
      python3 tool/wpt_status.py -j8 -o report.html
"""

import json
import os
import subprocess
import sys
from argparse import ArgumentParser
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from html import escape

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wpt_server import wpt_serve, DEFAULT_WPT_ROOT  # noqa: E402
from wpt_runner import RE_PASS, RE_FAIL, RE_DONE, STARFISH  # noqa: E402

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_TARGETS = os.path.join(REPO_ROOT, "tool", "wpt_status_targets.txt")
SERVER = "http://web-platform.test:8000"

# WPTR DONE status codes emitted by tool/wpt/inject_report.js (testharness.js
# harness status: 0 OK, 1 ERROR, 2 TIMEOUT, 3 PRECONDITION_FAILED).
HARNESS_STATUS = {0: "OK", 1: "ERROR", 2: "TIMEOUT", 3: "PRECONDITION_FAILED"}


def read_targets(path):
    dirs = []
    with open(path) as fp:
        for line in fp:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            dirs.append(line.rstrip("/"))
    return dirs


def _collect_urls(node, prefix, out):
    """Walk a manifest testharness subtree, appending url_base-relative paths.

    A file entry is [hash, [url|null, extras], ...]; a null url means the test
    url is the file's own path, while a non-null url is an explicit variant
    (e.g. the .any.html / .any.worker.html expansions of a .any.js source).
    """
    for name, val in node.items():
        if isinstance(val, list):
            for variant in val[1:]:
                url = variant[0]
                if url is None:
                    url = prefix + "/" + name
                out.append(url)
        elif isinstance(val, dict):
            _collect_urls(val, prefix + "/" + name, out)


def ensure_manifest(wpt_root, manifest_path):
    """Build MANIFEST.json if absent.

    The manifest is .gitignored, so a fresh checkout / CI runner won't have it,
    and we read it before starting the server (so `wpt serve` can't build it for
    us). `wpt manifest` also bootstraps the wpt virtualenv on first run.
    --no-download builds locally instead of fetching, which is robust behind a
    proxy.
    """
    if os.path.isfile(manifest_path):
        return
    wpt_bin = os.path.join(wpt_root, "wpt")
    if not os.path.isfile(wpt_bin):
        raise SystemExit(
            "no wpt checkout at %s (missing ./wpt); run "
            "`git submodule update --init third_party/wpt`" % wpt_root)
    print("MANIFEST.json not found; building it (one-time, may take a "
          "while)...", flush=True)
    subprocess.run([sys.executable, wpt_bin, "manifest",
                    "-p", manifest_path, "--tests-root", wpt_root,
                    "--no-download"], check=True)


def enumerate_tests(manifest_path, targets):
    """Return ({category: [full_url, ...]}, [missing_category, ...])."""
    with open(manifest_path) as fp:
        manifest = json.load(fp)
    url_base = manifest["url_base"]
    testharness = manifest["items"]["testharness"]
    by_category = {}
    missing = []
    for target in targets:
        node = testharness
        found = True
        for part in target.split("/"):
            if isinstance(node, dict) and part in node:
                node = node[part]
            else:
                found = False
                break
        if not found or not isinstance(node, dict):
            missing.append(target)
            by_category[target] = []
            continue
        rel = []
        _collect_urls(node, target, rel)
        prefix = url_base.rstrip("/")
        by_category[target] = [SERVER + prefix + "/" + u.lstrip("/")
                               for u in rel]
    return by_category, missing


def run_test(url, timeout):
    """Run one test in the Starfish shell; return a result dict.

    Subtests are kept by name+status (not just counts) so the same data can
    later be rendered as a wpt.fyi-format wptreport.
    """
    cmd = [STARFISH, url, "--hide-window", "--width=800", "--height=600"]
    env = dict(os.environ)
    env["HIDE_WINDOW"] = "1"
    wpt_domains = ".web-platform.test,.not-web-platform.test"
    for key in ("no_proxy", "NO_PROXY"):
        existing = env.get(key, "")
        env[key] = (existing + "," + wpt_domains) if existing else wpt_domains
    try:
        out = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            env=env, timeout=timeout).stdout.decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return {"status": "TIMEOUT", "subtests": [], "message": "shell timeout"}
    except OSError as e:
        return {"status": "CRASH", "subtests": [], "message": str(e)}

    subtests = []
    for line in out.splitlines():
        m = RE_PASS.search(line)
        if m:
            subtests.append({"name": m.group(1), "status": "PASS"})
            continue
        m = RE_FAIL.search(line)
        if m:
            subtests.append({"name": m.group(1), "status": "FAIL"})

    done = RE_DONE.search(out)
    if done is None:
        return {"status": "ERROR", "subtests": subtests,
                "message": "no harness completion"}
    return {"status": HARNESS_STATUS.get(int(done.group(1)), "ERROR"),
            "subtests": subtests, "message": None}


def verdict(result):
    """Pass/fail of a whole test, matching wpt_runner's criteria."""
    if result["status"] != "OK":
        return False, result["status"]
    if not result["subtests"]:
        return False, "NO_SUBTESTS"
    if any(s["status"] == "FAIL" for s in result["subtests"]):
        return False, "SUBTESTS_FAILED"
    return True, "OK"


def run_all(tasks, jobs, timeout):
    """tasks: [(category, url)]; returns list of result dicts with verdicts."""
    results = []
    total = len(tasks)
    done_n = 0
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        futs = {ex.submit(run_test, url, timeout): (cat, url)
                for cat, url in tasks}
        for fut in as_completed(futs):
            cat, url = futs[fut]
            r = fut.result()
            ok, reason = verdict(r)
            r.update({"category": cat, "url": url, "ok": ok, "reason": reason})
            results.append(r)
            done_n += 1
            mark = "PASS" if ok else "FAIL"
            print("[%4d/%d] %s %s" % (done_n, total, mark, url))
    return results


HTML_HEAD = """<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<title>Starfish WPT status</title>
<style>
 body{font:14px/1.5 system-ui,sans-serif;margin:0;padding:1.5rem;color:#1a1a1a}
 h1{font-size:1.4rem;margin:0 0 .25rem}
 .meta{color:#666;margin-bottom:1rem}
 .bar{height:14px;border-radius:7px;background:#e33;overflow:hidden;margin:.5rem 0}
 .bar>span{display:block;height:100%;background:#2a2}
 details{border:1px solid #ddd;border-radius:6px;margin:.4rem 0}
 summary{cursor:pointer;padding:.5rem .75rem;font-weight:600;
         display:flex;justify-content:space-between;gap:1rem}
 summary .n{font-weight:400;color:#666}
 table{width:100%;border-collapse:collapse;font-size:13px}
 td{padding:.3rem .75rem;border-top:1px solid #eee;vertical-align:top}
 td.s{width:3.2rem;font-weight:700}
 tr.pass td.s{color:#2a2} tr.fail td.s{color:#e33}
 .u{font-family:ui-monospace,monospace;word-break:break-all}
 .sub{color:#888;white-space:nowrap}
 .toggle{margin-bottom:1rem}
 body.failonly tr.pass{display:none}
</style>
<script>
 function failOnly(cb){document.body.classList.toggle('failonly',cb.checked)}
</script>
</head><body>
"""


def render_html(results, generated_at, missing):
    by_cat = {}
    for r in results:
        by_cat.setdefault(r["category"], []).append(r)
    total = len(results)
    passed = sum(1 for r in results if r["ok"])
    pct = (100.0 * passed / total) if total else 0.0

    parts = [HTML_HEAD]
    parts.append("<h1>Starfish WPT status</h1>")
    parts.append('<div class="meta">Generated %s &middot; %d/%d passed '
                 "(%.1f%%)</div>" % (escape(generated_at), passed, total, pct))
    parts.append('<div class="bar"><span style="width:%.2f%%"></span></div>'
                 % pct)
    if missing:
        parts.append('<div class="meta">Not in manifest (skipped): %s</div>'
                     % escape(", ".join(missing)))
    parts.append('<label class="toggle"><input type="checkbox" '
                 'onchange="failOnly(this)"> Show failures only</label>')

    for cat in sorted(by_cat):
        rows = sorted(by_cat[cat], key=lambda r: (r["ok"], r["url"]))
        cp = sum(1 for r in rows if r["ok"])
        parts.append("<details%s><summary>%s"
                     '<span class="n">%d/%d</span></summary><table>'
                     % (" open" if cp < len(rows) else "",
                        escape(cat), cp, len(rows)))
        for r in rows:
            cls = "pass" if r["ok"] else "fail"
            np = sum(1 for s in r["subtests"] if s["status"] == "PASS")
            nf = sum(1 for s in r["subtests"] if s["status"] == "FAIL")
            detail = "PASS:%d FAIL:%d" % (np, nf)
            if not r["ok"] and r["reason"] not in ("SUBTESTS_FAILED", "OK"):
                detail = r["reason"]
            path = r["url"][len(SERVER):]
            parts.append('<tr class="%s"><td class="s">%s</td>'
                         '<td class="u">%s</td><td class="sub">%s</td></tr>'
                         % (cls, "PASS" if r["ok"] else "FAIL",
                            escape(path), escape(detail)))
        parts.append("</table></details>")
    parts.append("</body></html>")
    return "".join(parts)


def main(argv):
    p = ArgumentParser(description=__doc__)
    p.add_argument("--targets", default=DEFAULT_TARGETS,
                   help="spec-dir list file (default: wpt_status_targets.txt)")
    p.add_argument("--only", action="append", metavar="DIR",
                   help="run only this spec dir (repeatable); overrides file")
    p.add_argument("--wpt-root", default=DEFAULT_WPT_ROOT,
                   help="path to the wpt checkout (default: third_party/wpt)")
    p.add_argument("--manifest", default=None,
                   help="MANIFEST.json path (default: <wpt-root>/MANIFEST.json)")
    p.add_argument("-j", "--jobs", type=int, default=8, help="parallel shells")
    p.add_argument("--timeout", type=int, default=15, help="per-test seconds")
    p.add_argument("--limit", type=int, default=0,
                   help="cap tests per category (0 = no cap; quick trials)")
    p.add_argument("-o", "--output", default="report.html",
                   help="HTML report path (default: report.html)")
    p.add_argument("--no-serve", action="store_true",
                   help="assume a server is already running")
    args = p.parse_args(argv)

    targets = args.only if args.only else read_targets(args.targets)
    manifest = args.manifest or os.path.join(args.wpt_root, "MANIFEST.json")
    ensure_manifest(args.wpt_root, manifest)
    by_category, missing = enumerate_tests(manifest, targets)

    tasks = []
    for cat in targets:
        urls = by_category.get(cat, [])
        if args.limit:
            urls = urls[:args.limit]
        tasks.extend((cat, u) for u in urls)

    if missing:
        print("WARNING: not found in manifest: %s" % ", ".join(missing))
    print("Enumerated %d testharness tests across %d categories"
          % (len(tasks), len([c for c in targets if c not in missing])))
    if not tasks:
        print("No tests to run.")
        return 1

    def go():
        return run_all(tasks, args.jobs, args.timeout)

    if args.no_serve:
        results = go()
    else:
        with wpt_serve(args.wpt_root, verbose=True):
            results = go()

    generated_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    html = render_html(results, generated_at, missing)
    with open(args.output, "w") as fp:
        fp.write(html)

    passed = sum(1 for r in results if r["ok"])
    print("\nWrote %s  (%d/%d passed)" % (args.output, passed, len(results)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
