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

"""Audit legacy WPT `.res` lists against a pinned `wpt serve` revision.

For every URL in the existing `tool/reftest/cairo/wpt/*.res` lists, ask the
running server whether it still serves that test (HTTP 200) or not (404/other).
This is the authoritative survival check: unlike a static `os.path.isfile`, the
server correctly accounts for WPT's GENERATED tests (`*.any.js` -> `*.any.html`,
`*.window.js` -> `*.window.html`, `.sub.` substitution, query variants), which
have no matching file on disk yet are served fine.

Use it to (1) pick a pin revision that maximises survival of the curated set,
and (2) generate the new `.res` base = old active URLs that still serve.

    python3 tool/wpt/scripts/wpt_audit.py --wpt-root /path/to/wpt
    python3 tool/wpt/scripts/wpt_audit.py --wpt-root /path/to/wpt \
        --out-dir tool/wpt/testharness_lists
"""

import os
import ssl
import sys
from argparse import ArgumentParser
from concurrent.futures import ThreadPoolExecutor
from http.client import HTTPConnection, HTTPSConnection
from urllib.parse import urlsplit

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)                                      # sibling wpt_*
sys.path.insert(0, os.path.join(_HERE, os.pardir, os.pardir))  # tool/
from repo_paths import REPO_ROOT  # noqa: E402
from wpt_server import wpt_serve, DEFAULT_WPT_ROOT  # noqa: E402

DEFAULT_RES_DIR = os.path.join(REPO_ROOT, "tool", "reftest", "cairo", "wpt")
_NOVERIFY = ssl._create_unverified_context()

GREEN, RED, YEL, RST = "\033[92m", "\033[91m", "\033[93m", "\033[0m"


def parse_res(path, include_commented):
    """Yield (url, was_commented) for each entry in a .res file."""
    with open(path) as fp:
        for line in fp:
            s = line.strip()
            if not s:
                continue
            commented = s.startswith("#")
            if commented:
                if not include_commented:
                    continue
                s = s[1:].strip()
                if not s or s.startswith("#"):
                    continue
            if not s.startswith("http"):
                continue
            yield s.split()[0], commented


def probe(url, timeout=10):
    """Return the HTTP status for a HEAD-ish GET, or None on connection error."""
    u = urlsplit(url)
    path = u.path + (("?" + u.query) if u.query else "")
    try:
        if u.scheme == "https":
            conn = HTTPSConnection(u.netloc, timeout=timeout, context=_NOVERIFY)
        else:
            conn = HTTPConnection(u.netloc, timeout=timeout)
        conn.request("GET", path)
        status = conn.getresponse().status
        conn.close()
        return status
    except OSError:
        return None


def audit(res_dir, include_commented, workers):
    res_files = sorted(f for f in os.listdir(res_dir) if f.endswith(".res"))
    # Collect entries grouped by file, de-duplicating identical URLs per file.
    per_file = {}
    for name in res_files:
        seen = set()
        entries = []
        for url, commented in parse_res(os.path.join(res_dir, name), include_commented):
            if url in seen:
                continue
            seen.add(url)
            entries.append((url, commented))
        if entries:
            per_file[name] = entries

    # Probe every unique URL once, concurrently.
    all_urls = sorted({u for entries in per_file.values() for u, _ in entries})
    print("Probing %d unique URLs across %d lists...\n" % (len(all_urls), len(per_file)))
    status = {}
    with ThreadPoolExecutor(max_workers=workers) as ex:
        for url, st in zip(all_urls, ex.map(probe, all_urls)):
            status[url] = st
    return per_file, status


def classify(st):
    if st == 200:
        return "served"
    if st == 404:
        return "missing"
    return "other"


# WPT serves these *.html test URLs generated from a *.js source on disk.
_GENERATED = [
    (".any.serviceworker.html", ".any.js"),
    (".any.sharedworker.html", ".any.js"),
    (".any.worker.html", ".any.js"),
    (".any.html", ".any.js"),
    (".window.html", ".window.js"),
    (".worker.html", ".worker.js"),
]
_DISPREFERRED = ("offscreen/", "/manual/", "tentative", "/support/")


def build_basename_index(wpt_root):
    """Map filename -> [posix relpaths] for every file under the wpt tree."""
    index = {}
    skip = {".git", "_venv3", "tools", "docs", ".github"}
    for root, dirs, files in os.walk(wpt_root):
        dirs[:] = [d for d in dirs if d not in skip]
        rel = os.path.relpath(root, wpt_root)
        for f in files:
            p = f if rel == "." else rel + "/" + f
            index.setdefault(f, []).append(p.replace(os.sep, "/"))
    return index


def _score(candidate, orig_path):
    """Higher is better: trailing path-segment overlap, minus dispreferred."""
    a = candidate.split("/")
    b = orig_path.lstrip("/").split("/")
    overlap = 0
    for x, y in zip(reversed(a[:-1]), reversed(b[:-1])):  # ignore basename
        if x == y:
            overlap += 1
        else:
            break
    penalty = sum(1 for d in _DISPREFERRED if d in ("/" + candidate))
    return (overlap, -penalty, -len(candidate))


_PLAIN_EXT = (".htm", ".html", ".xht", ".xhtml")
# A plain HTML test may have been rewritten as a generated *.js source.
_TO_GENERATED = ((".any.js", ".any.html"),
                 (".window.js", ".window.html"),
                 (".worker.js", ".worker.html"))


def _in(index, name):
    return index.get(name, [])


def _candidate_paths(base, index):
    """Candidate relpaths for a renamed test, across several rename patterns."""
    # 1. Same basename, moved directory (e.g. 2dcontext -> html/canvas).
    for p in _in(index, base):
        yield p

    # 2. A generated test URL (*.any.html) -> its *.js source's new dir.
    for html_suf, js_suf in _GENERATED:
        if base.endswith(html_suf):
            src = base[:-len(html_suf)] + js_suf
            for s in _in(index, src):
                d = os.path.dirname(s)
                yield (d + "/" + base) if d else base
            return  # generated name handled; plain patterns don't apply

    stem, ext = os.path.splitext(base)
    if ext.lower() not in _PLAIN_EXT:
        return

    # 3. Same stem, different extension (.htm <-> .html, .xht ...).
    for alt in _PLAIN_EXT:
        if alt != ext.lower():
            for p in _in(index, stem + alt):
                yield p

    # 4. Plain HTML converted to a generated *.js source.
    for js_suf, served_suf in _TO_GENERATED:
        for s in _in(index, stem + js_suf):
            d = os.path.dirname(s)
            served = stem + served_suf
            yield (d + "/" + served) if d else served


def remap_candidate(url, index):
    """Best-effort new URL for a renamed test, or None. Path only; probe to confirm."""
    sp = urlsplit(url)
    path = sp.path
    cands = list(dict.fromkeys(_candidate_paths(os.path.basename(path), index)))
    if not cands:
        return None
    best = max(cands, key=lambda c: _score(c, path))
    if best == path.lstrip("/"):
        return None
    return "%s://%s/%s%s%s" % (
        sp.scheme, sp.netloc, best,
        ("?" + sp.query) if sp.query else "",
        ("#" + sp.fragment) if sp.fragment else "")


def main(argv):
    p = ArgumentParser(description=__doc__)
    p.add_argument("--wpt-root", default=DEFAULT_WPT_ROOT,
                   help="path to the wpt checkout (default: third_party/wpt)")
    p.add_argument("--res-dir", default=DEFAULT_RES_DIR)
    p.add_argument("--include-commented", action="store_true",
                   help="also probe #-commented entries")
    p.add_argument("--out-dir", default=None,
                   help="write new .res (served + remapped URLs) + reports here")
    p.add_argument("--no-remap", action="store_true",
                   help="do not try to recover renamed tests by basename")
    p.add_argument("--workers", type=int, default=16)
    p.add_argument("--no-serve", action="store_true",
                   help="assume a server is already running")
    args = p.parse_args(argv)
    if not args.no_serve and not args.wpt_root:
        p.error("--wpt-root or WPT_ROOT is required (or pass --no-serve)")

    def do():
        per_file, status = audit(args.res_dir, args.include_commented, args.workers)
        remapped = {}
        if not args.no_remap and args.wpt_root:
            remapped = recover(per_file, status, args.wpt_root, args.workers)
        report(per_file, status, remapped, args.out_dir)

    if args.no_serve:
        do()
    else:
        with wpt_serve(args.wpt_root, verbose=True):
            do()
    return 0


def recover(per_file, status, wpt_root, workers):
    """For each missing URL, find a renamed candidate and probe it.

    Returns {old_url: new_url} for those confirmed served (HTTP 200).
    """
    missing = sorted({u for entries in per_file.values() for u, _ in entries
                      if classify(status.get(u)) != "served"})
    if not missing:
        return {}
    print("\nRemapping %d missing URLs by basename..." % len(missing))
    index = build_basename_index(wpt_root)
    candidates = {u: remap_candidate(u, index) for u in missing}
    to_probe = [(u, c) for u, c in candidates.items() if c]
    recovered = {}
    with ThreadPoolExecutor(max_workers=workers) as ex:
        for (old, new), st in zip(to_probe, ex.map(probe, [c for _, c in to_probe])):
            if st == 200:
                recovered[old] = new
    print("Recovered %d/%d missing via remap." % (len(recovered), len(missing)))
    return recovered


def report(per_file, status, remapped, out_dir):
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    tot = {"served": 0, "remapped": 0, "missing": 0, "other": 0}
    print("\n%-44s %7s %7s %7s %6s" % ("list", "served", "remap", "missing", "other"))
    print("-" * 76)
    for name in sorted(per_file):
        c = {"served": 0, "remapped": 0, "missing": 0, "other": 0}
        out_urls, missing_urls, remap_pairs = [], [], []
        for url, _ in per_file[name]:
            cls = classify(status.get(url))
            if cls == "served":
                c["served"] += 1
                out_urls.append(url)
            elif url in remapped:
                c["remapped"] += 1
                out_urls.append(remapped[url])
                remap_pairs.append((url, remapped[url]))
            else:
                c[cls] += 1
                missing_urls.append((url, status.get(url)))
        for k in tot:
            tot[k] += c[k]
        recov = c["served"] + c["remapped"]
        color = GREEN if not missing_urls else (RED if recov == 0 else YEL)
        print("%s%-44s %7d %7d %7d %6d%s"
              % (color, name, c["served"], c["remapped"], c["missing"], c["other"], RST))
        if out_dir:
            if out_urls:
                with open(os.path.join(out_dir, name), "w") as f:
                    f.write("\n".join(out_urls) + "\n")
            if remap_pairs:
                with open(os.path.join(out_dir, name + ".remap"), "w") as f:
                    for old, new in remap_pairs:
                        f.write("%s\t->\t%s\n" % (old, new))
            # Always record drops -- even when the whole list dropped to zero,
            # so nothing vanishes silently.
            if missing_urls:
                with open(os.path.join(out_dir, name + ".missing"), "w") as f:
                    for url, st in missing_urls:
                        f.write("%s\t%s\n" % (st, url))

    total = sum(tot.values())
    kept = tot["served"] + tot["remapped"]
    rate = 100.0 * kept / total if total else 0.0
    print("-" * 76)
    print("%-44s %7d %7d %7d %6d"
          % ("TOTAL", tot["served"], tot["remapped"], tot["missing"], tot["other"]))
    print("\n%sCoverage: %d/%d kept (%.1f%%)  [served %d + remapped %d; dropped %d]%s"
          % (YEL, kept, total, rate, tot["served"], tot["remapped"],
             tot["missing"] + tot["other"], RST))
    if out_dir:
        print("New .res -> %s/  (*.remap = renamed, *.missing = dropped)" % out_dir)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
