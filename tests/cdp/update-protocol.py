#!/usr/bin/env python3

"""Download the Chrome DevTools Protocol definition at a pinned revision.

Writes, all tracked by git:

    protocol/browser_protocol.json
    protocol/js_protocol.json
    protocol/revision.txt

Nothing else is touched; testlist-schema.res is yours to update after
running the tests.
"""

import argparse
import pathlib
import sys
import urllib.error
import urllib.request

RAW = ("https://raw.githubusercontent.com/ChromeDevTools/devtools-protocol/"
       "%s/json/%s")
FILES = ("browser_protocol.json", "js_protocol.json")


def download(revision, output_dir):
    """Fetch into a staging directory first.

    A half-finished download must not leave the checked-in definition in a
    mixed state, where the two files would describe different revisions.
    """
    staging = {}
    for name in FILES:
        with urllib.request.urlopen(RAW % (revision, name),
                                    timeout=60) as response:
            staging[name] = response.read()
    output_dir.mkdir(parents=True, exist_ok=True)
    for name, content in staging.items():
        (output_dir / name).write_bytes(content)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--revision", required=True,
                        help="a devtools-protocol commit sha or tag")
    args = parser.parse_args()

    output_dir = pathlib.Path(__file__).resolve().parent / "protocol"
    revision_file = output_dir / "revision.txt"
    previous = (revision_file.read_text().strip()
                if revision_file.exists() else "")

    # No overwrite guard: every output is tracked, so git diff shows what
    # changed and git checkout undoes it.
    try:
        download(args.revision, output_dir)
    except (urllib.error.URLError, OSError) as error:
        print("could not download %s: %s" % (args.revision, error),
              file=sys.stderr)
        return 1
    revision_file.write_text(args.revision + "\n")
    print("Updated the CDP protocol definition to %s" % args.revision)

    if previous and previous != args.revision:
        print()
        print("The protocol moved from %s to %s." % (previous, args.revision))
        print("Run 'run.py rebaseline schema' to rebuild testlist-schema.res.",
              file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
