#!/usr/bin/env python
import os
import re
if __name__ == "__main__":
    import sys
    sys.path.append(os.path.dirname(os.path.dirname(sys.argv[0])))
from basics.utils import Strings

RE_KEYWORDS = re.compile(r"PASS|FAIL")

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


def exp_img_namer(tc_file, backend):
    if tc_file.startswith("http"):
        if backend == "efl":
            backend = "test/efl/reftest/vendor/"
        elif backend == "cairo":
            backend = "test/cairo/reftest/vendor/"
        pre = "webkit/fast/xmlhttprequest_result/"
        post = os.path.splitext(os.path.basename(tc_file))[0] + "_expected.png"
        return backend + pre + post
    else:
        file = os.path.splitext(tc_file)[0] + "_expected.png"
        if not "_original/" in file and "fast/" in file:
            # Support legacy TCs
            pre, post = file.split("fast/", 1)
            mid, post = post.split("/", 1)
            return pre + "fast/" + mid + "_result/" + post
        return file


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("list")
    parser.add_argument("--pixeltest", dest="pixel_test", action="store_true")
    parser.add_argument("--ahem-font", dest="ahem_font", action="store_true")
    args = parser.parse_args()

    if args.pixel_test:
        import basics.starfish_pixel_test as pixeltest
        if args.ahem_font:
            cpass, cfail = pixeltest.run_parallel(args.list, ahem_font=True, expected_namer=exp_img_namer)
        else:
            cpass, cfail = pixeltest.run_parallel(args.list, ahem_font=False, expected_namer=exp_img_namer)
    else:
        import basics.starfish_basic_test as basictest
        cpass, cfail = basictest.run_parallel(args.list, tc_handler=tc_handler)

    from basics.utils import PColors
    print PColors.yellow("PASS: " + str(cpass) + ", FAIL: " + str(cfail))
