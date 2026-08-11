#! /usr/bin/env python3

# Formatted by black.

import os
import subprocess
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)          # test_runner (same dir)
sys.path.insert(0, os.path.dirname(_HERE))  # tool/ for drivers.basics

from test_runner import __file__ as test_runner_path
from drivers.basics.constants import ERRORCODE


def update_file(pass_fname, tc_fname, dry_run=False):
    if not os.path.isfile(pass_fname) or not os.path.isfile(tc_fname):
        print(f"Error: {pass_fname} or {tc_fname} does not exist.")
        return

    with open(pass_fname, "r") as pass_file, open(tc_fname, "r") as tc_file:
        pass_list = [line.strip() for line in pass_file.readlines()]
        tc_list = tc_file.readlines()

    updated = False

    for i in range(len(tc_list)):
        if tc_list[i].startswith("#"):
            content = tc_list[i][1:].strip()
            # Handle empty line
            tokens = content.split()
            if len(tokens) == 0:
                continue
            # Use only the first token to consider inline comments
            if tokens[0] in pass_list:
                # Not update if an inline comment has @ignore annotation
                if "@ignore" in tokens:
                    print(f"= Ignored: {content}")
                    continue
                tc_list[i] = content + "\n"
                updated = True
                print(f"+ Updated: {content}")
        else:
            content = tc_list[i].strip()
            tokens = content.split()
            if tokens[0] not in pass_list:
                tc_list[i] = "# " + content + "\n"
                updated = True
                print(f"- Updated: {content}")

    if updated:
        if dry_run:
            print("Dry run mode. No actual update performed.")
        else:
            with open(tc_fname, "w") as file_b:
                file_b.writelines(tc_list)
            print("Updated.")
    else:
        print("No updates needed.")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Update TC lists based on passed TC lists"
    )
    parser.add_argument("LIST_FILE", help="Filename to be updated (.res)")
    parser.add_argument(
        "RESULT_FILE",
        nargs="?",
        default="test_result.txt",
        help="Filename including passed TCs [default: %(default)s]",
    )
    parser.add_argument(
        "-d", "--dry-run", action="store_true", help="Run the script in dry run mode"
    )
    parser.add_argument(
        "-r", "--run-tc", type=str, help="Run a test suite before updating TC lists"
    )
    args = parser.parse_args()

    if args.run_tc:
        try:
            command = [
                test_runner_path,
                "--force",
                "--out-pass-list",
                "--out-pass-list-filename",
                args.RESULT_FILE,
                args.run_tc,
            ]
            result = subprocess.run(" ".join(command), shell=True)
            if result.returncode not in [
                ERRORCODE.TEST_PASSED,
                ERRORCODE.TEST_FAILED,
            ]:
                print(f"\nStopped: '{command}' failed with code {result.returncode}")
                exit(1)
        except subprocess.CalledProcessError as e:
            print(f"Error: {e}")
            exit(1)

    update_file(args.RESULT_FILE, args.LIST_FILE, args.dry_run)
