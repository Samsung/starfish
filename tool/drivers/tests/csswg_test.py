#!/usr/bin/env python
import os

def fd_exp_img_namer(tc_file):
    file = os.path.splitext(tc_file)[0] + "_expected.png"
    return file.replace("_converted", "_result/font_independent", 1)


def result_handler(tc_itr, result_itr):
    import logging
    logging.basicConfig(format="%(message)s")
    print "\n============= Check Regression ============="
    ntc = 0
    npass = 0
    for result in result_itr:
        if result:
            npass += 1
        else:
            logging.error("[FAIL] " + tc_itr[ntc][1])
        ntc += 1
    print "\n================= Result ==================="
    return (npass, ntc - npass)