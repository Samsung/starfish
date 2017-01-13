#!/usr/bin/env python
import os
import sys
import subprocess
import multiprocessing

LIST_PATH = sys.argv[1]
INPUT_DIR = ""                    # for TCs (html files)
EXPECTED_DIR = ""                 # for expected images (node-webkit)
OUTPUT_FILE = "__starfish_result.png" # for Starfish screen-shots
NPROC = multiprocessing.cpu_count()
TC_LIST = []

# Parse arguments
# "argparse" can be used here - wy.kim commented
if len(sys.argv) > 2:
    for idx in range(2, len(sys.argv)):
        arg = sys.argv[idx]
        if "--input-dir=" in arg:
            INPUT_DIR = arg.replace("--input-dir=", "")
        elif "--expected-dir=" in arg:
            EXPECTED_DIR = arg.replace("--expected-dir=", "")

FNULL = open(os.devnull, 'w')


def run():
    # TODO : Consider too long tc list
    with open(LIST_PATH) as fp:
        for idx, line in enumerate(fp):
            content = line.strip()
            if (len(content) > 0) and content[0] != '#':
                TC_LIST.append([idx, content])
    print "======================================================="
    print " - Process       : ", NPROC
    print " - TC            :", len(TC_LIST)
    print " - TC dir        :", INPUT_DIR
    print " - Expected dir  :", EXPECTED_DIR
    print "======================================================="

    print "Processing..."
    p = multiprocessing.Pool(NPROC)
    final_results = p.map(singleTask, TC_LIST)
    p.terminate()

    print "==================== test result ======================"
    pass_count = 0
    for idx, line in enumerate(final_results):
        show = line
        if "passed" in line:
            pass_count = pass_count + 1
        elif "Error" in line:
            show = "diff: 100.0% failed"
        print TC_LIST[idx][1], show
    print "===== total :", len(TC_LIST), " passed :", pass_count, " failed :", (len(TC_LIST) - pass_count)


def singleTask(data):
    tc_idx = data[0]
    tc_name = os.path.splitext(data[1])[0]
    tc_file = os.path.join(INPUT_DIR, data[1])
    tc_expected_png = os.path.join(EXPECTED_DIR, tc_name + "_expected.png")
    tc_expected_dir = os.path.dirname(tc_expected_png)
    tc_result_png = str(tc_idx) + OUTPUT_FILE
    # Assure TC exist
    if not os.path.isfile(tc_file):
        print tc_idx, "\t", "ERROR : No such TC - " + tc_file
        return "Error"
    # Assure expected image
    if not os.path.isfile(tc_expected_png):
        print tc_idx, "\t", "ERROR : No expected file - " + tc_expected_png
        return "Error"
    # Create screen-shot image using Starfish
    subprocess.call(["./StarFish", tc_file, "--pixel-test", "--width=800", "--height=600", "--screen-shot=" + tc_result_png], stdout=FNULL, stderr=subprocess.STDOUT)
    if not os.path.isfile(tc_result_png):
        print tc_idx, "\t", "ERROR : Failed to generate Srarfish screenshot image - " + tc_file
        return "Error"
    # Diff
    diff_result = subprocess.check_output(["tool/imgdiff/imgdiff", tc_result_png, tc_expected_png]).decode('UTF-8').strip()
    os.remove(tc_result_png)
    if len(diff_result) == 0:
        return "Error"
    # print tc_idx, "\t", data[1], diff_result
    return diff_result

run()