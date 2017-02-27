#!/usr/bin/env python

# Copyright 2015 Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import fileinput
import argparse

#from check_license import CheckLicenser
import os.path as fs
import subprocess

TERM_RED = "\033[1;31m"
TERM_YELLOW = "\033[1;33m"
TERM_GREEN = "\033[1;32m"
TERM_BLUE = "\033[1;34m"
TERM_EMPTY = "\033[0m"


count_err = 0
count_lines = 0
count_empty_lines = 0

interesting_exts = ['.cpp', '.h', '.js', '.py', '.sh', '.cmake']
clang_format_exts = ['.cpp', '.h']
skip_dirs = ['deps', 'build', 'third_party', 'out', 'tools', '.git', 'test']
skip_files = []


def report_error_name_line(name, line, msg):
    global count_err
    if line is None:
        print("%s: %s" % (name, msg))
    else:
        print("%s:%d: %s" % (name, line, msg))
    count_err += 1


def report_error(msg):
    report_error_name_line(fileinput.filename(), fileinput.filelineno(), msg)


def is_checked_by_clang(file):
    _, ext = fs.splitext(file)
    return ext in clang_format_exts and file not in skip_files

def check_tidy_at_file(file, update):
    if update:
        formatted = subprocess.check_output(['clang-format-3.8',
            '-style=file', '-i', file])
    else:
        formatted = subprocess.check_output(['clang-format-3.8',
            '-style=file', file])
        f = open(file + '.formatted', 'w')
        f.write(formatted)
        f.close()
        if subprocess.call(['diff'] + [file, file + '.formatted']) != 0:
            print(file + '\n')
        os.remove(file + '.formatted')

def check_whitespace_error(files):
    global count_lines
    global count_empty_lines
    for line in fileinput.input(files):
        if '\t' in line:
            report_error('TAB character')
        if '\r' in line:
            report_error('CR character')
        if line.endswith(' \n') or line.endswith('\t\n'):
            report_error('trailing whitespace')
        if not line.endswith('\n'):
            report_error('line ends without NEW LINE character')

#            if fileinput.isfirstline():
#                if not CheckLicenser.check(fileinput.filename()):
#                    report_error_name_line(fileinput.filename(),
#                                           None,
#                                       'incorrect license')

        count_lines += 1
        if not line.strip():
            count_empty_lines += 1

def check_tidy(args):
    print args.path

    if args.update:
        print("Files will be fomatted. Check the change: git diff")

    if os.path.isfile(args.path):
        if is_checked_by_clang(args.path):
            check_tidy_at_file(args.path, args.update)

        check_whitespace_error([args.path])

    for (dirpath, _, filenames) in os.walk(args.path):
        if any(d in fs.relpath(dirpath, args.path) for d in skip_dirs):
            continue

        files = [fs.join(dirpath, name) for name in filenames
                 if is_checked_by_clang(name)]
        if not files:
            continue

        for file in files:
            if is_checked_by_clang(file):
                check_tidy_at_file(file, args.update)

        check_whitespace_error(files)

    print "* total lines of code: %d" % count_lines
    print ("* total non-blank lines of code: %d"
           % (count_lines - count_empty_lines))
    print "%s* total errors: %d%s" % (TERM_RED if count_err > 0 else TERM_GREEN,
                                      count_err,
                                      TERM_EMPTY)
    print

    return count_err == 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Tidy Checker')
    parser.add_argument('--update', '-u', dest='update', action='store_true',
                        help='flag to update')
    parser.add_argument('--path', '-p', dest='path', default='.', type=str,
                        help='path to check tidy')
    args = parser.parse_args()
    check_tidy(args)
