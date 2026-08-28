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
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Inventory helpers for the pinned WPT MANIFEST.json."""

import json
import os

_HERE = os.path.dirname(os.path.abspath(__file__))

DEFAULT_TARGETS = os.path.join(_HERE, os.pardir, "wpt_status_targets.txt")
SERVER = "http://web-platform.test:8000"


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


def enumerate_tests(manifest_path, targets, test_type="testharness"):
    """Return ({category: [full_url, ...]}, [missing_category, ...]).

    test_type selects the MANIFEST.json items branch to walk ("testharness",
    "reftest", "crashtest", ...). Non-testharness branches use a 3-element
    variant ([url, references-or-extras, extras]) instead of testharness's
    2-element one, but _collect_urls only ever reads variant[0] (the url), so
    the same walker works unchanged across types.

    manifest_path may be a path to MANIFEST.json (existing callers:
    wpt_manifest_lists.py) or an already-parsed manifest dict -- wpt_status.py
    parses it once and passes the dict for each requested test type, instead
    of re-parsing the same (potentially tens-of-MB) file once per type.
    """
    if isinstance(manifest_path, dict):
        manifest = manifest_path
    else:
        with open(manifest_path) as fp:
            manifest = json.load(fp)
    url_base = manifest["url_base"]
    branch = manifest["items"][test_type]
    by_category = {}
    missing = []
    for target in targets:
        node = branch
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
