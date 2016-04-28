#!/usr/bin/python

import sys
import re
import os
import json

def readjson(fileName):
    fp = open(fileName, 'r')
    jsonobject = json.load(fp)
    fp.close()
    return jsonobject

def read_PATTERN(fileName):
    Anti_Pattern=[];
    fp = open(fileName, 'r')
    Anti_Pattern = fp.readlines()
    fp.close()

    #print str(Anti_Pattern)
    return Anti_Pattern

def read_json_PATTERN(key,jsonObject):
    Anti_Pattern=[];
    Anti_Pattern = jsonObject[key]

    #print str(Anti_Pattern)
    return Anti_Pattern

def write_result(fileName,result):
    fp = open(fileName, 'w')
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

    pattern_json = readjson('PATTERN.json')

    result = []
    result_fileName = []
    tmp_file_Pattern = ['TEMPLATE']

    if len(sys.argv) != 2:
        print 'Check Parameter!'

    write_result('PATTERN_FileName.txt', tmp_file_Pattern)

    Anti_Pattern_content = read_json_PATTERN('CONTENT_PATTERN',pattern_json)
    Anti_Pattern_dir = read_json_PATTERN('DIR_PATTERN',pattern_json)
    Anti_Pattern_fileName = read_json_PATTERN('FILENAME_PATTERN',pattern_json)
    tmp_file_Pattern = read_PATTERN('PATTERN_FileName.txt')

    for root, directories, filenames in os.walk(sys.argv[1]):
        #for directory in directories:
        #    os.path.join(root, directory)
        for filename in filenames:
            only_file_name = os.path.splitext(filename)[0]
            if filename.endswith('.html') or filename.endswith('.js'):
                if check_content(root,Anti_Pattern_dir):
                    if check_content(filename,Anti_Pattern_fileName):
                        current_file_path=os.path.join(root, filename)
                        #print os.path.join(root, filename)
                        current_file_str = open_File(current_file_path);
                        if check_content(current_file_str,Anti_Pattern_content):
                            result_fileName.append(filename)
                            result.append(current_file_path)
                        else:
                            tmp_file_Pattern.append(only_file_name)

    Anti_Pattern_fileName_str = str(tmp_file_Pattern);

    for index, filename in enumerate(result_fileName):
        only_file_name = os.path.splitext(filename)[0]

        if re.search(only_file_name, Anti_Pattern_fileName_str, re.X) is not None:
            result[index]=''
        if filename.endswith('.js'):
            result[index]=''

    real_result = []
    for filename in result:
        if filename is not '':
            real_result.append(filename)


    write_result('RESULT.txt',real_result)
    write_result('PATTERN_FileName.txt', tmp_file_Pattern)
    #print str(result)

main()
