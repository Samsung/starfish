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

"""Generate per-spec `.res` URL lists for a non-testharness WPT test type.

tool/wpt/lists/ (testharness) is curated from the legacy tool/reftest/cairo/wpt/
lists via wpt_audit.py. reftest and crashtest have no such legacy corpus, so
this instead enumerates MANIFEST.json directly -- the same mechanism
tool/wpt_status.py already uses for its un-curated testharness coverage report
-- against the spec directories in tool/wpt_status_targets.txt.

The generated lists intentionally carry only test URLs, no reference/relation/
fuzzy metadata: tool/wpt_reftest.py resolves that from MANIFEST.json at run
time (so a submodule bump that changes a reference or adds fuzzy just works,
the way wptrunner also resolves it live rather than baking it into a list).

    python3 tool/wpt_manifest_lists.py --mode reftest \
        --out-dir tool/wpt/reftest_lists
    python3 tool/wpt_manifest_lists.py --mode crashtest \
        --out-dir tool/wpt/crashtest_lists
"""

import os
import sys
from argparse import ArgumentParser

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from wpt_server import DEFAULT_WPT_ROOT  # noqa: E402
from wpt_status import (DEFAULT_TARGETS, ensure_manifest,  # noqa: E402
                        enumerate_tests, read_targets)

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def list_name(target):
    """Turn a spec directory ("css/css-flexbox") into a list filename."""
    return target.replace("/", "_") + ".res"


def write_list(path, urls):
    with open(path, "w") as fp:
        for url in urls:
            fp.write(url + "\n")


def main(argv):
    p = ArgumentParser(description=__doc__)
    p.add_argument("--mode", required=True, choices=("reftest", "crashtest"),
                   help="MANIFEST.json items branch to enumerate -- matches "
                        "wpt_runner.py --mode for the same test kind")
    p.add_argument("--wpt-root", default=DEFAULT_WPT_ROOT,
                   help="path to the wpt checkout (default: third_party/wpt)")
    p.add_argument("--targets", default=DEFAULT_TARGETS,
                   help="spec directory list (default: wpt_status_targets.txt)")
    p.add_argument("--out-dir", required=True,
                   help="directory to write <spec>.res files into")
    args = p.parse_args(argv)

    manifest_path = os.path.join(args.wpt_root, "MANIFEST.json")
    ensure_manifest(args.wpt_root, manifest_path)
    targets = read_targets(args.targets)
    by_category, missing = enumerate_tests(manifest_path, targets,
                                           test_type=args.mode)

    os.makedirs(args.out_dir, exist_ok=True)
    total = 0
    written = 0
    for target in targets:
        urls = by_category.get(target, [])
        if not urls:
            continue
        write_list(os.path.join(args.out_dir, list_name(target)), urls)
        written += 1
        total += len(urls)

    print("Wrote %d lists (%d URLs total) to %s" % (written, total, args.out_dir))
    if missing:
        print("%d target dirs not found in manifest (skipped):" % len(missing))
        for m in missing:
            print("  %s" % m)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
