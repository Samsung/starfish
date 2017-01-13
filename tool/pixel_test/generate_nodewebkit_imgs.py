#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil

DEFAULT_EXPECTED_PATH = "out/x64/exe/debug/reftest/pixel_test"
LIST_PATH = sys.argv[1]
INPUT_DIR = ""
EXPECTED_DIR = ""
TMP_LIST_PATH = "__tmp_list__.res"
SPECIFIED_EXPECTED_DIR = False

if len(sys.argv) > 2:
    for idx in range(2, len(sys.argv)):
        arg = sys.argv[idx]
        if "--input-dir=" in arg:
            INPUT_DIR = arg.replace("--input-dir=", "")
        elif "--expected-dir=" in arg:
            EXPECTED_DIR = arg.replace("--expected-dir=", "")
            SPECIFIED_EXPECTED_DIR = True

if SPECIFIED_EXPECTED_DIR and os.path.isdir(EXPECTED_DIR):
    print("Error : Specified expected directory [" + EXPECTED_DIR + "] already exists. Please delete it before run this script.")
    sys.exit(0)

if os.path.isfile(TMP_LIST_PATH):
    os.remove(TMP_LIST_PATH)

with open(LIST_PATH, 'r') as old:
    with open(TMP_LIST_PATH, 'w') as new:
        for line in old:
            new.write(os.path.join(INPUT_DIR, line))

subprocess.call(["test/tool/nwjs-no-AA/nw", "tool/pixel_test/nw_capture/", "-l", TMP_LIST_PATH, "pc"])
if SPECIFIED_EXPECTED_DIR:
    shutil.move(DEFAULT_EXPECTED_PATH, EXPECTED_DIR)
    print("Move generated images to " + EXPECTED_DIR)
os.remove(TMP_LIST_PATH)