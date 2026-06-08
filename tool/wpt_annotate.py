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

"""Comment out currently-failing tests in the generated WPT `.res` lists.

This repo's convention is: active lines = expected-pass, `#`-commented lines =
known failures. After a measurement run (tool/wpt_runner.py --results FILE),
feed the results here to mark FAIL URLs as `# [auto-fail] ...`, so the lists
become a clean green regression gate while keeping failures visible/auditable.

Re-run after any engine fix or pin bump to refresh the gate.

    python3 tool/wpt_runner.py tool/wpt/lists --results res.txt
    python3 tool/wpt_annotate.py res.txt tool/wpt/lists
"""

import os
import sys

MARK = "# [auto-fail] "


def load_verdicts(results_path):
    """url -> True(pass)/False(fail); last verdict wins."""
    v = {}
    with open(results_path) as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) == 3:
                v[parts[2]] = (parts[0] == "PASS")
    return v


def annotate_file(path, verdicts):
    out = []
    commented = kept = unknown = 0
    with open(path) as f:
        for raw in f:
            line = raw.rstrip("\n")
            s = line.strip()
            # Leave blanks and already-commented lines untouched.
            if not s or s.startswith("#"):
                out.append(line)
                continue
            url = s.split()[0]
            passed = verdicts.get(url)
            if passed is False:
                out.append(MARK + line)
                commented += 1
            else:
                out.append(line)
                if passed is True:
                    kept += 1
                else:
                    unknown += 1
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    return kept, commented, unknown


def main(argv):
    if len(argv) != 2:
        print("usage: wpt_annotate.py RESULTS_FILE RES_DIR_OR_FILE", file=sys.stderr)
        return 2
    results_path, target = argv
    verdicts = load_verdicts(results_path)

    if os.path.isdir(target):
        files = [os.path.join(target, n) for n in sorted(os.listdir(target))
                 if n.endswith(".res")]
    else:
        files = [target]

    tk = tc = tu = 0
    for path in files:
        kept, commented, unknown = annotate_file(path, verdicts)
        tk += kept
        tc += commented
        tu += unknown
        if commented or unknown:
            print("%-46s kept %4d  commented %4d  unknown %4d"
                  % (os.path.basename(path), kept, commented, unknown))
    print("\nTOTAL active(pass) %d  commented(fail) %d  unknown(no result) %d"
          % (tk, tc, tu))
    if tu:
        print("note: 'unknown' lines had no entry in the results file "
              "(not run) -- left active.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
