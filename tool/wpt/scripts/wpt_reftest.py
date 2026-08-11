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

"""Run a single WPT reftest: capture + compare, harness-side.

WPT reftests carry no stored expected image. Instead a test page names one or
more reference pages in MANIFEST.json, each with a comparison relation ("=="
must-match, "!=" must-mismatch); the verdict comes from rendering both and
diffing the pixels at run time.

The engine's own two-phase `--ref-test` state machine (WebView.cpp: rtDoTest)
does this internally, but currently crashes navigating to the reference
(WebView.cpp:816, referrerURL null assert) and only understands rel=match.
Rather than fix and extend engine-internal test orchestration, this instead
composes two already-CI-proven primitives from the harness side, the same way
tool/wpt/scripts/wpt_runner.py already judges testharness tests externally:

  - `--screen-shot=<file>`: render one page, dump one PNG, exit. Used as-is
    by tool/drivers/basics/starfish_pixel_test.py in the existing golden-image
    pixel suites.
  - `tool/imgdiff/imgdiff a b`: pixel-compare two PNGs. It always prints a
    `diff: X.XX%` line; we read that percentage ourselves rather than trusting
    imgdiff's own "[imgdiff-fail]" verdict, because that verdict folds in a
    4-neighbor noise-cluster filter meant for golden-image antialiasing
    tolerance -- inappropriate for WPT reftest, which (absent a fuzzy
    annotation) wants an exact match for "==" and any difference at all for
    "!=". A nonzero exit code still means imgdiff couldn't even read a file.

MANIFEST.json reftest entries look like:
    "path/to/test.html": ["<hash>", [null, [["/path/to/ref.html", "=="]], {"fuzzy": ...}]]
A single manifest key can have multiple variants (query-string-parameterized
generated tests, e.g. "foo.html?width=10..."); resolve_references() matches
the exact variant the URL under test names, not the union of all of them.
References are resolved relative to the manifest's own url_base, not to the
running test's URL.
"""

import json
import os
import re
import subprocess
import sys
import threading

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)                                      # sibling wpt_*
sys.path.insert(0, os.path.join(_HERE, os.pardir, os.pardir))  # tool/
from repo_paths import REPO_ROOT  # noqa: E402
from wpt_server import DEFAULT_WPT_ROOT  # noqa: E402

STARFISH = os.path.join(REPO_ROOT, "Starfish")
IMGDIFF = os.path.join(REPO_ROOT, "tool", "imgdiff", "imgdiff")
SERVER = "http://web-platform.test:8000"


def _manifest_path(wpt_root):
    return os.path.join(wpt_root, "MANIFEST.json")


def ensure_manifest(wpt_root, manifest_path):
    """Build MANIFEST.json if absent.

    The manifest is .gitignored, so a fresh checkout / CI runner won't have it,
    and we read it before starting the server (so `wpt serve` can't build it
    for us). `wpt manifest` also bootstraps the wpt virtualenv on first run.
    --no-download builds locally instead of fetching, which is robust behind a
    proxy.

    Lives here (rather than wpt_status.py, which first defined it) because
    this is the lowest-level module that needs it: wpt_status.py imports from
    wpt_runner.py, which imports from this module, so defining it in
    wpt_status.py forced every other caller into a deferred, function-scoped
    import to dodge the resulting cycle. wpt_status.py re-exports this name
    for backward compatibility with existing callers.
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


def load_manifest(wpt_root=DEFAULT_WPT_ROOT):
    with open(_manifest_path(wpt_root)) as fp:
        return json.load(fp)


def ensure_imgdiff():
    """Fail fast (once, before the batch) if the imgdiff binary is missing.

    Without this, a run started before `ninja ... install_pixel_test_dep` has
    built imgdiff would instead have every single reftest raise its own
    FileNotFoundError deep in _images_differ, surfacing as thousands of
    identical INTERNAL_ERROR reasons -- looking like a tooling meltdown rather
    than one missing binary. Mirrors ensure_manifest()'s upfront-check pattern.
    """
    if not os.access(IMGDIFF, os.X_OK):
        raise SystemExit(
            "imgdiff not found/executable at %s; build it once with "
            "`ninja -C <build> install_pixel_test_dep`" % IMGDIFF)


def _find_node(items, test_path):
    """Walk items["reftest"] (or ["crashtest"]) by test_path components."""
    node = items
    parts = [p for p in test_path.split("/") if p]
    for part in parts[:-1]:
        if not isinstance(node, dict) or part not in node:
            return None
        node = node[part]
    if not isinstance(node, dict) or parts[-1] not in node:
        return None
    return node[parts[-1]]


def resolve_references(manifest, test_path):
    """Return [(ref_full_url, relation, fuzzy), ...] for a reftest test_path.

    test_path is manifest-relative (e.g. "css/css-flexbox/foo.html", no
    leading slash, no server origin) and may carry a query string for a
    parameterized/generated test (e.g. "foo.html?width=10&height=10"; the
    manifest dict key is always the bare path without the query, so lookup is
    done on the query-stripped path, then the query-bearing test_path is used
    to pick the ONE variant among possibly-several that matches the URL
    actually being tested. Returns [] if the manifest has no reftest entry for
    this path (e.g. a stale .res list entry).
    """
    bare_path = test_path.split("?", 1)[0]
    entry = _find_node(manifest["items"]["reftest"], bare_path)
    if not entry:
        return []
    url_base = manifest["url_base"].rstrip("/")
    out = []
    # entry = ["<hash>", variant, variant, ...]; each variant is
    # [url|null, [[ref_path, relation], ...], {extras}]. A manifest key can
    # have multiple variants (query-string-parameterized generated tests);
    # a null variant url means "this entry's own bare path" (the common,
    # single-variant case, which always matches); a non-null url is a
    # specific query-variant and must match test_path exactly, so a
    # different variant's references never leak into this test's verdict.
    for variant in entry[1:]:
        variant_path = variant[0]
        if variant_path is not None and variant_path != test_path:
            continue
        references = variant[1]
        extras = variant[2] if len(variant) > 2 else {}
        fuzzy = extras.get("fuzzy") if isinstance(extras, dict) else None
        for ref_path, relation in references:
            out.append((_ref_to_url(ref_path, url_base), relation, fuzzy))
    return out


# A reference can be a server-relative path ("/css/.../ref.html") or an
# absolute URI with its own scheme -- most commonly "about:blank" (14 such
# refs in the current pin). Only the former gets the server origin + url_base
# prepended; blindly concatenating an absolute-URI ref would produce a
# corrupt URL like "http://web-platform.test:8000about:blank" that renders
# nothing meaningful (silently yielding a false PASS for "!=" refs and a
# bogus IMG_MISMATCH for "==" refs).
_SCHEME_RE = re.compile(r"^[a-z][a-z0-9+.-]*:", re.IGNORECASE)


def _ref_to_url(ref_path, url_base):
    if _SCHEME_RE.match(ref_path):
        return ref_path
    return SERVER + url_base + ref_path


def url_to_test_path(url):
    """Strip server origin from a full test URL to get a manifest-relative path."""
    for prefix in (SERVER,):
        if url.startswith(prefix):
            return url[len(prefix):].lstrip("/")
    return url.lstrip("/")


def _screenshot(url, out_path, timeout, width=800, height=600):
    """Render one page and dump a PNG. Returns (ok, reason, log).

    log carries the captured Starfish stdout+stderr (crash backtraces print
    to stdout, see src/shell/Shell.cpp) on any failure path, so a caller can
    surface it (e.g. wpt_runner.py's --verbose); it is None on success.
    """
    cmd = [STARFISH, url, "--hide-window", "--screen-shot=" + out_path,
          "--width=%d" % width, "--height=%d" % height]
    env = dict(os.environ)
    env["HIDE_WINDOW"] = "1"
    wpt_domains = ".web-platform.test,.not-web-platform.test"
    for key in ("no_proxy", "NO_PROXY"):
        existing = env.get(key, "")
        env[key] = (existing + "," + wpt_domains) if existing else wpt_domains
    try:
        r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        # e.output holds whatever the process wrote before being killed.
        return False, "TIMEOUT", (e.output or b"").decode("utf-8", "replace")
    except OSError:
        return False, "SHELL_ERROR", None
    if r.returncode != 0 or not os.path.isfile(out_path):
        return False, "TC_CRASH", r.stdout.decode("utf-8", "replace")
    return True, None, None


_DIFF_RE = re.compile(r"diff: ([\d.]+)%")


def _images_differ(png_a, png_b, timeout):
    """Return True if imgdiff measures any nonzero pixel difference.

    Deliberately does NOT use imgdiff's own "[imgdiff-fail]" marker: that
    verdict folds in a 4-neighbor noise-cluster filter built for golden-image
    antialiasing tolerance, so a small-but-real difference (exactly what a
    "!=" reftest is checking for) can print unmarked as "passed (not exactly
    same)". WPT reftest semantics (no fuzzy) want an exact comparison, so we
    read the percentage imgdiff always prints and decide ourselves: >0% means
    the images differ, regardless of how imgdiff itself classified it.

    Known residual limits (see docs/wpt.md): the printed percentage is
    rounded to 2 decimals, so a handful of differing pixels on a large image
    can round to "0.00%"; and imgdiff's own per-pixel comparison already
    tolerates a difference of a few tolerance-per-channel before ANY pixel
    counts as differing at all. Both would need a tool/imgdiff/imgdiff.cpp
    change (a raw-diffCount / no-tolerance mode) to close completely -- out of
    scope here since imgdiff is shared with the golden-image pixel suites.

    Raises RuntimeError if imgdiff couldn't read a file at all (distinct from
    "the images differ"), and lets subprocess.TimeoutExpired propagate on a
    hang -- both are caught by run_reftest.
    """
    r = subprocess.run([IMGDIFF, png_a, png_b], stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=timeout)
    out = r.stdout.decode("utf-8", "replace")
    # Checked before the percentage parse: a read error (missing/corrupt PNG)
    # prints "[imgdiff-fail]" via abort() with no "diff:" line at all, so
    # parsing first would misreport this as an unparseable-output error
    # instead of the clearer "couldn't read a file" one.
    if r.returncode != 0:
        raise RuntimeError("imgdiff failed to read an input file: %s" % out.strip())
    m = _DIFF_RE.search(out)
    if not m:
        raise RuntimeError("could not parse imgdiff output: %s" % out.strip())
    return float(m.group(1)) > 0.0


def run_reftest(url, wpt_root=DEFAULT_WPT_ROOT, manifest=None, timeout=15,
                tmp_dir=None, width=800, height=600):
    """Run one WPT reftest. Return (ok, reason, log).

    ok=True means the test's own pass condition (relation vs the actual pixel
    comparison) was satisfied for every listed reference -- WPT reftests with
    multiple references require ALL of them to hold.

    log carries the captured Starfish output when a page failed to render
    (crash/timeout/shell-error); it is None for a purely logical mismatch
    (IMG_MISMATCH etc., where there is no process output to show).
    """
    if manifest is None:
        manifest = load_manifest(wpt_root)
    test_path = url_to_test_path(url)
    references = resolve_references(manifest, test_path)
    if not references:
        return False, "NO_REFERENCE", None

    # wpt_runner.py fans out via ThreadPoolExecutor, so many reftests run
    # concurrently inside one process -- pid alone is not unique per-call the
    # way it is for the engine's own getpid()-based reftest PNGs. Mix in the
    # thread id so concurrent calls never collide on the same tmp filename.
    tmp_dir = tmp_dir or "/tmp"
    tag = "%d_%d" % (os.getpid(), threading.get_ident())
    test_png = os.path.join(tmp_dir, "reftest_%s_test.png" % tag)
    # The whole body runs under this try/finally (not just the loop) so
    # test_png is cleaned up on every exit path, including the test page's
    # own screenshot failing (e.g. TIMEOUT after partially writing the file).
    try:
        ok, reason, log = _screenshot(url, test_png, timeout, width, height)
        if not ok:
            return False, reason, log

        for ref_url, relation, fuzzy in references:
            # MVP: fuzzy tolerance is not implemented (exact-pixel only), so a
            # fuzzy-annotated reftest is judged same as an exact one and may
            # false-FAIL on legitimate sub-threshold antialiasing noise --
            # expected to be filtered out by wpt_annotate.py until a follow-up
            # adds tolerance support.
            ref_png = os.path.join(tmp_dir, "reftest_%s_ref.png" % tag)
            try:
                ok, reason, log = _screenshot(ref_url, ref_png, timeout, width, height)
                if not ok:
                    return False, "REF_LOAD_FAIL(%s)" % reason, log
                # A single corrupt/unreadable PNG or a hung imgdiff must not
                # take down the whole batch run (ThreadPoolExecutor.map()
                # re-raises on the consuming side, which would abort the
                # entire wpt_runner.py invocation) -- convert both into a
                # normal per-test FAIL instead of letting them propagate.
                try:
                    differ = _images_differ(test_png, ref_png, timeout)
                except subprocess.TimeoutExpired:
                    return False, "IMGDIFF_TIMEOUT", None
                except RuntimeError as e:
                    return False, "IMGDIFF_ERROR: %s" % e, None
            finally:
                os.path.exists(ref_png) and os.remove(ref_png)
            wants_match = (relation == "==")
            passed = (not differ) if wants_match else differ
            if not passed:
                return False, ("IMG_MISMATCH" if wants_match
                               else "IMG_UNEXPECTED_MATCH"), None
        return True, "OK", None
    finally:
        os.path.exists(test_png) and os.remove(test_png)


if __name__ == "__main__":
    import argparse

    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("url", help="full test URL, e.g. "
                   "http://web-platform.test:8000/css/css-flexbox/foo.html")
    p.add_argument("--wpt-root", default=DEFAULT_WPT_ROOT)
    p.add_argument("--timeout", type=int, default=15)
    args = p.parse_args()
    ensure_manifest(args.wpt_root, _manifest_path(args.wpt_root))
    ok, reason, log = run_reftest(args.url, wpt_root=args.wpt_root, timeout=args.timeout)
    print("PASS" if ok else "FAIL", reason)
    if log:
        print(log)
    sys.exit(0 if ok else 1)
