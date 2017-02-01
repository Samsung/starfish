#!/usr/bin/env python
import os
import re
if __name__ == "__main__":
    import sys
    sys.path.append(os.path.dirname(os.path.dirname(sys.argv[0])))
from basics.utils import Strings

RE_KEYWORDS = re.compile(r"Success|failure|Skipped")

def tc_handler(tc_file, output, show_progress=True):
    is_pass = False
    result = ""
    tc_expected_file = os.path.splitext(tc_file)[0] + "-expected.txt"
    if not os.path.isfile(tc_expected_file):
        result = Strings.FAIL_SIGN + tc_file + ": Expected file does not exist."
    else:
        expected_out = None
        starfish_out = None
        with open(tc_expected_file) as fp:
            expected_out = RE_KEYWORDS.findall(fp.read())
        starfish_out = RE_KEYWORDS.findall(output)

        if expected_out == starfish_out:
            result = Strings.PASS_SIGN
            is_pass = True
        else:
            result = Strings.FAIL_SIGN
        result += tc_file
    if show_progress:
        print result
    return is_pass


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("list")
    args = parser.parse_args()

    import basics.starfish_basic_test as basictest
    cpass, cfail = basictest.run_parallel(args.list, tc_handler=tc_handler)

    from basics.utils import PColors
    print PColors.yellow("PASS: " + str(cpass) + ", FAIL: " + str(cfail))
    

