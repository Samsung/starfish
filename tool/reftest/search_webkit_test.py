#!/usr/bin/python

import sys
import re
import os

def read_PATTERN(fileName):
    Anti_Pattern=[];
    fp = open(fileName, 'r')
    Anti_Pattern = fp.readlines()
    fp.close()

    #print str(Anti_Pattern)
    return Anti_Pattern

def write_result(result):
    fp = open('RESULT.txt', 'w')
    for str in result:
        fp.write(str+"\n")
    fp.close()

def open_File(filepath):
    #print filepath
    fp = open(filepath, 'r')
    current_file_str = fp.read();
    fp.close()
    return current_file_str

def check_content(str,Anti_Pattern):
    #print "************************"
    #print str
    for pattern in Anti_Pattern:
        if re.search(pattern,str,re.X) is not None:
            #print pattern+"\n"
            return False

    return True

def main():
    result = []

    if len(sys.argv) != 2:
        print 'Check Parameter!'

    Anti_Pattern_content = read_PATTERN('PATTERN.txt')
    Anti_Pattern_dir = read_PATTERN('PATTERN_DIR.txt')

    for root, directories, filenames in os.walk(sys.argv[1]):
        #for directory in directories:
        #    os.path.join(root, directory)
        for filename in filenames:
            if filename != 'TEMPLATE.html':
                if filename.endswith('.html') or filename.endswith('.js'):
                    if check_content(root,Anti_Pattern_dir):
                        current_file_path=os.path.join(root, filename)
                        #print os.path.join(root, filename)
                        current_file_str = open_File(current_file_path);
                        if check_content(current_file_str,Anti_Pattern_content):
                            result.append(current_file_path)

    write_result(result)
    #print str(result)

main()