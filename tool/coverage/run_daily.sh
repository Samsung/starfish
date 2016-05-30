#!/bin/bash

path=`pwd`

# 1. compile
cd $path
git checkout .
git fetch
git rebase origin master

cd third_party/escargot && make clean && TC=1 make x64.interpreter.debug.static -j
cd ../../
make clean && TC=1 make x64.exe.debug -j

# 2. run tests
cd tool/coverage && ./genDomSpecList.sh
cd ../../ && ./tool/coverage/genCssHtmlSpecList.sh

# 3. gen reports
cd $path/tool/coverage && ./genCoverage.sh && ./combineCsv.sh
d=`date +"%y%m%d"`

ssh webtf@10.113.64.195 "mkdir -p share/Daily_Tests/$d"
scp coverage.csv webtf@10.113.64.195:share/Daily_Tests/$d

python csv2xml.py dom.csv
python csv2xml.py html.csv
python csv2xml.py css.csv
scp *.xml webtf@10.113.64.195:share/Daily_Tests/$d
