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

"""Inventory helpers for WPT `.res` lists."""

import os
from urllib.parse import urlsplit


def compare_inventory(manifest_items, ci_entries):
    """Compare candidate keys with active CI entries without selecting tests.

    manifest_items contains (type, URL) keys; ci_entries maps each requested
    type to (list name, URL) pairs from collect(..., False). Callers supply
    the actual CI lists, including separately scheduled suites. Missing CI
    keys are never silently added to the MANIFEST or treated as executable.
    URL queries and origins remain significant. List provenance is retained
    for duplicate entries and diagnostics.
    """
    candidates = set(manifest_items)
    provenance = {}
    for kind, entries in ci_entries.items():
        for name, url in entries:
            provenance.setdefault((kind, url), set()).add(name)
    ci = set(provenance)
    same_type_path = set()
    other_types = {}
    for kind, url in candidates:
        parsed = urlsplit(url)
        path = (parsed.path, parsed.query, parsed.fragment)
        same_type_path.add((kind, path))
        other_types.setdefault(path, set()).add(kind)
    missing = {}
    for kind, url in ci - candidates:
        parsed = urlsplit(url)
        path = (parsed.path, parsed.query, parsed.fragment)
        if (kind, path) in same_type_path:
            reason = "origin_mismatch"
        elif path in other_types:
            reason = "type_mismatch"
        else:
            reason = "absent"
        missing[(kind, url)] = reason
    return {"covered": ci & candidates, "missing": missing,
            "additional": candidates - ci, "ci_sources": provenance}


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
