#!/usr/bin/env python
import os
import sys
import re
from basics.utils import PColors

def tc_handler(tc_file, output, show_progress=True):
    is_pass = False
    result = ""
    word_pass = len(re.findall(r"PASS", output))
    word_fail = len(re.findall(r"FAIL", output))
    word_all = word_pass + word_fail

    if word_all == 0:
        result = PColors.red("[FAIL] ") + tc_file + " (" + PColors.red("Invalid result") + ")"
        word_fail += 1
        result += "Starfish output => " + output
    elif word_fail == 0:
        result = PColors.green("[PASS] ") + tc_file
        result += " (" + PColors.green("PASS: " + str(word_pass)) + ")"
        is_pass = True
    else:
        result = PColors.red("[FAIL] ") + tc_file
        result += " (" + PColors.green("PASS: " + str(word_pass)) + ", "
        result += PColors.red("FAIL: " + str(word_fail)) + ")"
        result += "Starfish output => " + output

    if show_progress:
        print result
    return (word_pass, word_fail)

def result_handler(tc_itr, result_itr):
    ntotal = 0
    npass = 0
    nsubpass = 0
    nsubfail = 0
    for result in result_itr:
        (word_pass, word_fail) = result
        ntotal += 1
        npass += (1 if word_fail is 0 else 0)
        nsubpass += word_pass
        nsubfail += word_fail
    return ((npass, ntotal - npass), (nsubpass, nsubfail))

def result_summarizer(result):
    ((unit_pass, unit_fail), (subunit_pass, subunit_fail)) = result
    summary = "Total: " + str(unit_pass + unit_fail)
    summary += ", Pass: " + str(unit_pass)
    if unit_fail > 0:
        summary += ", Fail: " + str(unit_fail)
    summary += " (Detail total: " + str(subunit_pass + subunit_fail)
    summary += ", pass: " + str(subunit_pass)
    if subunit_fail > 0:
        summary += ", fail: " + str(subunit_fail) + ")"
    else:
        summary += ")"
    print PColors.yellow(summary + "\n")
    return (unit_fail == 0)
