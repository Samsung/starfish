"""Writes the list of a browser-vendor test suite (webkit_test, blink_test,
gecko_test) from the tests found in its checkout.

A line already in the list stays as it is. A new line is active only when
every --pass-list file (the pass file `test_runner.py -f --out-pass-list`
writes) has it, and is added commented otherwise. Tests that no longer exist
upstream are dropped and printed.
"""
import argparse
import os
import subprocess
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TEST_EXTENSIONS = (".html", ".htm", ".xhtml", ".xht", ".svg", ".xml")
SUPPORT_DIRS = {"resources", "script-tests"}


def served_pages(checkout):
    """Yields every page in the directories the sparse checkout holds (the
    suite's tool/<vendor>/fetch.sh names them), leaving out support
    directories and -expected files."""
    served = subprocess.run(["git", "-C", checkout, "sparse-checkout", "list"],
                            capture_output=True, text=True, check=True).stdout.split()
    for top in sorted(served):
        if os.path.basename(top) in SUPPORT_DIRS:
            continue
        for d, dirs, files in os.walk(os.path.join(checkout, top)):
            dirs[:] = sorted(x for x in dirs if x not in SUPPORT_DIRS)
            for f in sorted(files):
                if f.endswith(TEST_EXTENSIONS) and "-expected" not in f:
                    yield os.path.join(d, f)


def read_list(path):
    """Maps each listed URL to its whole line, comment and known-failure reason
    included."""
    state = {}
    if os.path.isfile(path):
        for line in open(path):
            s = line.strip()
            if s:
                state[s.lstrip("#").strip().split()[0]] = s
    return state


def main(description, list_path, checkout, find_tests):
    """find_tests() returns the suite's test URLs in list order."""
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--pass-list", action="append", default=[],
                        help="pass file; repeat to require a pass in every run")
    args = parser.parse_args()
    passing = None
    for p in args.pass_list:
        runs = set(l.strip() for l in open(p) if l.strip())
        passing = runs if passing is None else passing & runs

    os.chdir(REPO_ROOT)  # the drivers' paths are relative to the repository
    if not os.path.isdir(checkout):
        sys.exit("no checkout at %s -- run the suite's fetch.sh" % checkout)

    old = read_list(list_path)
    tests = list(dict.fromkeys(find_tests()))
    for gone in sorted(set(old) - set(tests)):
        print("dropped " + gone)
    lines = []
    for url in tests:
        if url in old:
            lines.append(old[url])
        else:
            lines.append(url if passing and url in passing else "# " + url)
    with open(list_path, "w") as fp:
        fp.write("\n".join(lines) + "\n")
    print("%s: %d tests, %d active" % (os.path.relpath(list_path, REPO_ROOT), len(lines),
                                       sum(not l.startswith("#") for l in lines)))
