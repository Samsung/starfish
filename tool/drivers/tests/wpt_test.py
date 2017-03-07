#!/usr/bin/env python
import os
import sys
import re
if __name__ == "__main__":
    import sys
    sys.path.append(os.path.dirname(os.path.dirname(sys.argv[0])))
from basics.utils import PColors

def tc_handler(tc_file, output, show_progress=True):
    is_pass = False
    result = ""
    word_pass = len(re.findall(r"PASS", output))
    word_fail = len(re.findall(r"FAIL", output))
    word_all = word_pass + word_fail

    if word_all == 0:
        result = PColors.red("[FAIL] ") + tc_file + " - Wrong result"
    elif word_fail == 0:
        result = PColors.green("[PASS] ") + tc_file
        result += " (" + PColors.green("PASS: " + str(word_pass)) + ")"
        is_pass = True
    else:
        result = PColors.red("[FAIL] ") + tc_file
        result += " (" + PColors.green("PASS: " + str(word_pass)) + ", "
        result += PColors.red("FAIL: " + str(word_fail)) + ")"

    if show_progress:
        print result
    return is_pass


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("list")
    parser.add_argument("--ahem-font", dest="ahem_font", action="store_true")
    args = parser.parse_args()

    import basics.starfish_basic_test as basictest
    if args.ahem_font:
        cpass, cfail = basictest.run_parallel(args.list, regression=True, tc_handler=tc_handler)
    else:
        cpass, cfail = basictest.run_parallel(args.list, tc_handler=tc_handler)
    print PColors.yellow("PASS: " + str(cpass) + ", FAIL: " + str(cfail))
