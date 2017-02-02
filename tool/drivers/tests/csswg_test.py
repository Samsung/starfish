#!/usr/bin/env python
import os

# TODO Make it one simple rule to generate expected image name
def font_dep_exp_img_namer(tc_file):
    file = os.path.splitext(tc_file)[0] + "-expected.png"
    return file.replace("_converted", "_result/font_dependent/x64", 1)

def font_indep_exp_img_namer(tc_file):
    file = os.path.splitext(tc_file)[0] + "_expected.png"
    return file.replace("_converted", "_result/font_independent", 1)

def get_exp_img_namer(font_dep):
    if font_dep:
        return font_dep_exp_img_namer
    return font_indep_exp_img_namer

def result_handler(tc_itr, result_itr):
    import logging
    logging.basicConfig(format="%(message)s")
    print "\nCheck Regression!!\n"
    ntc = 0
    npass = 0
    for result in result_itr:
        if result:
            npass += 1
        else:
            logging.error("[FAIL] " + tc_itr[ntc][1])
        ntc += 1
    return (npass, ntc - npass)