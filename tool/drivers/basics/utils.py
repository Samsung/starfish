#!/usr/bin/env python
import types

class PColors:
    ENDC = '\033[0m'
    BLUE = '\033[94m'
    GREEN = '\033[32m'
    YELLOW = '\033[93m'
    RED = '\033[91m'

    @staticmethod
    def blue(text):
        return PColors.BLUE + text + PColors.ENDC

    @staticmethod
    def green(text):
        return PColors.GREEN + text + PColors.ENDC

    @staticmethod
    def yellow(text):
        return PColors.YELLOW + text + PColors.ENDC

    @staticmethod
    def red(text):
        return PColors.RED + text + PColors.ENDC

def isInt(val):
    return type(val) is types.IntType

def isBool(val):
    return type(val) is types.BooleanType

def isFunction(val):
    return type(val) is types.FunctionType

class Strings:
    PASS_SIGN = PColors.green("[PASS] ")
    FAIL_SIGN = PColors.red("[FAIL] ")
    CHECK_SIGN = PColors.yellow("[CHECK] ")
    

