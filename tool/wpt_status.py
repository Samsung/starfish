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


def wpt_revision(wpt_root):
    """Return a human-readable WPT version string for the checkout.

    WPT publishes daily/weekly epoch tags (e.g. epochs/daily/2026-06-08_05H),
    so `git describe` gives a readable version and the short hash pins it
    exactly. Recording this per run means the dashboard can show which WPT
    revision produced each number -- so a count that shifts after a submodule
    bump is not mistaken for a regression. Returns "unknown" if git is
    unavailable (e.g. a tarball checkout).
    """
    def git(*args):
        return subprocess.run(
            ["git", "-C", wpt_root] + list(args),
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
        ).stdout.decode("utf-8", "replace").strip()
    try:
        tag = git("describe", "--tags", "--always")
        sha = git("rev-parse", "--short", "HEAD")
    except OSError:
        return "unknown"
    if tag and sha and tag != sha:
        return "%s (%s)" % (tag, sha)
    return tag or sha or "unknown"


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


def score(result):
    """Return (passing, total) subtest counts using wpt.fyi's summary rule.

    wpt.fyi (results-processor) scores each test at the *subtest* level so its
    numbers are comparable across browsers:
      - with subtests: total = #subtests, passing = #PASS subtests
        (the harness status itself is not counted separately);
      - without subtests (single-page test, or harness ERROR/TIMEOUT that
        produced none): total = 1, passing = 1 if the harness status is OK,
        else 0.
    """
    subs = result["subtests"]
    if subs:
        return sum(1 for s in subs if s["status"] == "PASS"), len(subs)
    return (1, 1) if result["status"] in ("OK", "PASS") else (0, 1)


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
 details details{margin:.4rem .4rem .4rem 1rem;border-left:3px solid #eee}
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
 body.failonly details.allpass{display:none}
</style>
<script>
 function failOnly(cb){document.body.classList.toggle('failonly',cb.checked)}
 // Reorder directory <details> and test <tr> siblings at every tree level by
 // the chosen key/direction. Preserves the tree (dirs first, table last) and
 // each <details> open state, since we only re-append existing nodes.
 function sortAll(){
   var key=document.getElementById('sortKey').value;
   var sign=document.getElementById('sortDir').value==='asc'?1:-1;
   function val(el){return key==='name'?(el.getAttribute('data-name')||''):
     parseFloat(el.getAttribute('data-rate'))||0}
   function cmp(a,b){var x=val(a),y=val(b);
     if(key==='name')return sign*String(x).localeCompare(String(y));
     return sign*(x-y)}
   function sortIn(container){
     var kids=Array.prototype.slice.call(container.children);
     var dets=kids.filter(function(e){return e.tagName==='DETAILS'&&
       e.classList.contains('dir')});
     var tables=kids.filter(function(e){return e.tagName==='TABLE'});
     dets.sort(cmp);
     dets.forEach(function(d){container.appendChild(d)});
     tables.forEach(function(tb){
       container.appendChild(tb);
       // Browsers wrap bare <tr> in an implicit <tbody>, so match both and
       // re-append to each row's actual parent.
       var rows=Array.prototype.slice.call(
         tb.querySelectorAll(':scope>tr, :scope>tbody>tr'));
       rows.sort(cmp);
       rows.forEach(function(r){r.parentNode.appendChild(r)})});
     dets.forEach(sortIn);
   }
   sortIn(document.body);
 }
</script>
</head><body>
"""


def build_tree(results):
    """Group results into a nested directory tree keyed on the test URL path.

    Mirrors wpt.fyi's drill-down: /css/selectors/foo.html becomes
    css -> selectors -> foo.html, with each directory aggregating the subtest
    counts of everything beneath it. A node is {"dirs": {name: node},
    "tests": [result], "pass": int, "total": int}; a directory may hold both
    subdirectories and its own direct test files.
    """
    root = {"dirs": {}, "tests": [], "pass": 0, "total": 0}
    for r in results:
        parts = r["url"][len(SERVER):].strip("/").split("/")
        node = root
        for part in parts[:-1]:
            node = node["dirs"].setdefault(
                part, {"dirs": {}, "tests": [], "pass": 0, "total": 0})
        node["tests"].append(r)
    _accumulate(root)
    return root


def _accumulate(node):
    """Sum subtest (pass, total) bottom-up into each node; return its totals."""
    p = sum(score(r)[0] for r in node["tests"])
    t = sum(score(r)[1] for r in node["tests"])
    for child in node["dirs"].values():
        cp, ct = _accumulate(child)
        p += cp
        t += ct
    node["pass"], node["total"] = p, t
    return p, t


def render_node(name, node, parts):
    """Render a directory node as a nested <details>, recursing into subdirs.

    Each node and test row carries data-name / data-pass / data-total /
    data-rate so the client-side sort (sortAll() in HTML_HEAD) can reorder
    siblings at every tree level without re-running anything.
    """
    p, t = node["pass"], node["total"]
    allpass = " allpass" if p == t else ""
    rate = (p / t) if t else 0.0
    # Collapsed by default (like wpt.fyi); the reader expands what they want.
    parts.append('<details class="dir%s" data-name="%s" data-pass="%d" '
                 'data-total="%d" data-rate="%.6f"><summary>%s/'
                 '<span class="n">%d/%d</span></summary>'
                 % (allpass, escape(name), p, t, rate, escape(name), p, t))
    for sub in sorted(node["dirs"]):
        render_node(sub, node["dirs"][sub], parts)
    tests = sorted(node["tests"], key=lambda r: r["url"])
    if tests:
        parts.append("<table>")
        for r in tests:
            cls = "pass" if r["ok"] else "fail"
            np, nt = score(r)
            rrate = (np / nt) if nt else 0.0
            detail = "%d/%d subtests" % (np, nt)
            if not r["ok"] and r["reason"] not in ("SUBTESTS_FAILED", "OK"):
                detail = r["reason"]
            leaf = r["url"][len(SERVER):].rstrip("/").rsplit("/", 1)[-1]
            parts.append('<tr class="%s" data-name="%s" data-pass="%d" '
                         'data-total="%d" data-rate="%.6f">'
                         '<td class="s">%s</td>'
                         '<td class="u">%s</td><td class="sub">%s</td></tr>'
                         % (cls, escape(leaf), np, nt, rrate,
                            "PASS" if r["ok"] else "FAIL",
                            escape(leaf), escape(detail)))
        parts.append("</table>")
    parts.append("</details>")


def render_html(results, generated_at, missing, revision=None):
    # Subtest-level totals, matching wpt.fyi's classification (see score()).
    passed = sum(score(r)[0] for r in results)
    total = sum(score(r)[1] for r in results)
    pct = (100.0 * passed / total) if total else 0.0

    parts = [HTML_HEAD]
    parts.append("<h1>Starfish WPT status</h1>")
    # wpt.fyi-style "N tests (M subtests)" so the file count and subtest count
    # are both visible (a smaller total here is scope, not a regression).
    parts.append('<div class="meta">Generated {date} &middot; Showing '
                 "{files:,} tests ({total:,} subtests) &middot; "
                 "{passed:,}/{total:,} subtests passed ({pct:.1f}%) "
                 "&middot; comparable to wpt.fyi</div>"
                 .format(date=escape(generated_at), files=len(results),
                         total=total, passed=passed, pct=pct))
    parts.append('<div class="meta">WPT revision: <strong>%s</strong></div>'
                 % escape(revision or "unknown"))
    parts.append('<div class="bar"><span style="width:%.2f%%"></span></div>'
                 % pct)
    parts.append('<div class="meta">Counts <strong>testharness</strong> '
                 "subtests only (reftest / crashtest / wdspec excluded), so "
                 "the total test count looks smaller than wpt.fyi's full set; "
                 "read the comparison at the subtest level.</div>")
    if missing:
        parts.append('<div class="meta">Not in manifest (skipped): %s</div>'
                     % escape(", ".join(missing)))
    parts.append('<label class="toggle"><input type="checkbox" '
                 'onchange="failOnly(this)"> Show failures only</label>')
    parts.append('<div class="toggle">Sort: '
                 '<select id="sortKey" onchange="sortAll()">'
                 '<option value="name">Directory name</option>'
                 '<option value="rate">Pass rate</option></select> '
                 '<select id="sortDir" onchange="sortAll()">'
                 '<option value="asc">Ascending</option>'
                 '<option value="desc">Descending</option></select></div>')

    root = build_tree(results)
    for name in sorted(root["dirs"]):
        render_node(name, root["dirs"][name], parts)
    parts.append("</body></html>")
    return "".join(parts)


def extract_metrics(results, now=None, revision=None):
    """Extract metrics from test results for dashboard JSON.

    passed/failed/total/rate are at the *subtest* level (see score()), so the
    dashboard numbers are comparable to wpt.fyi. files_passed/files_total keep
    the per-test-file view for our own diagnostics, and categories holds the
    per-spec-dir [passed, total] subtest breakdown (used by the browser
    comparison in Phase 2). wpt_revision records which WPT checkout produced
    these numbers so historical entries stay interpretable across submodule
    bumps.

    now: datetime to use (defaults to datetime.now()); pass the same value
    used for the HTML report so timestamps are consistent.
    revision: WPT version string (see wpt_revision()).
    """
    if now is None:
        now = datetime.now()
    passed = sum(score(r)[0] for r in results)
    total = sum(score(r)[1] for r in results)
    categories = {}
    for r in results:
        p, t = score(r)
        agg = categories.setdefault(r["category"], [0, 0])
        agg[0] += p
        agg[1] += t
    return {
        "date": now.strftime("%Y-%m-%d"),
        "time": now.strftime("%H:%M:%S"),
        "timestamp": int(now.timestamp()),
        "passed": passed,
        "failed": total - passed,
        "total": total,
        "rate": round(100.0 * passed / total, 1) if total else 0.0,
        "files_passed": sum(1 for r in results if r["ok"]),
        "files_total": len(results),
        "categories": categories,
        "wpt_revision": revision or "unknown",
    }


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
    p.add_argument("--output-json", default=None,
                   help="JSON metrics file (for dashboard)")
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

    revision = wpt_revision(args.wpt_root)
    generated_at_dt = datetime.now()
    generated_at = generated_at_dt.strftime("%Y-%m-%d %H:%M:%S")
    html = render_html(results, generated_at, missing, revision)
    with open(args.output, "w") as fp:
        fp.write(html)

    sub_pass = sum(score(r)[0] for r in results)
    sub_total = sum(score(r)[1] for r in results)
    files_pass = sum(1 for r in results if r["ok"])
    print("\nWrote %s  (%d/%d subtests passed; %d/%d files all-pass)"
          % (args.output, sub_pass, sub_total, files_pass, len(results)))

    if args.output_json:
        metrics = extract_metrics(results, generated_at_dt, revision)
        with open(args.output_json, "w") as fp:
            json.dump(metrics, fp, indent=2)
        print("Wrote %s" % args.output_json)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
