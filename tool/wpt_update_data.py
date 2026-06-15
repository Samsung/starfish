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

"""Update cumulative WPT status metrics for the dashboard.

Loads a metrics JSON file (from wpt_status.py --output-json), appends it to
a cumulative data.json file, and writes back. Used by the nightly CI workflow
to maintain time-series data for the dashboard.

    python3 tool/wpt_update_data.py \\
      --metrics /tmp/metrics.json \\
      --data-file data.json
"""

import json
import os
import sys
from argparse import ArgumentParser


def load_data(path):
    """Load cumulative data; return list of metrics dicts."""
    if not os.path.isfile(path):
        return []
    try:
        with open(path) as fp:
            data = json.load(fp)
        if isinstance(data, dict):
            return data.get("history", [])
        return data if isinstance(data, list) else []
    except (json.JSONDecodeError, IOError) as e:
        print("ERROR: %s is corrupted: %s" % (path, e), file=sys.stderr)
        sys.exit(1)


def load_metrics(path):
    """Load a single metrics JSON file."""
    try:
        with open(path) as fp:
            return json.load(fp)
    except (json.JSONDecodeError, IOError) as e:
        print("ERROR: failed to load metrics from %s: %s" % (path, e),
              file=sys.stderr)
        sys.exit(1)


def append_metric(data, metric):
    """Append a metric to the data list, avoiding duplicates by date.

    If a metric with the same date already exists, replace it (for re-runs).
    """
    date = metric.get("date")
    if date:
        # Remove any existing entry for the same date
        data = [d for d in data if d.get("date") != date]
    data.append(metric)
    return data


def save_data(data, path):
    """Write cumulative data to a JSON file."""
    with open(path, "w") as fp:
        json.dump(data, fp, indent=2)


def main(argv):
    p = ArgumentParser(description=__doc__)
    p.add_argument("--metrics", required=True,
                   help="metrics JSON file from wpt_status.py --output-json")
    p.add_argument("--data-file", default="data.json",
                   help="cumulative data.json file (default: data.json)")
    args = p.parse_args(argv)

    metric = load_metrics(args.metrics)
    data = load_data(args.data_file)
    data = append_metric(data, metric)
    save_data(data, args.data_file)

    print("Updated %s: added %s, total %d records" %
          (args.data_file, metric.get("date"), len(data)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
