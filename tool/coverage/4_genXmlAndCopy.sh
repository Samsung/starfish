#!/bin/sh

d=`date +"%y%m%d"`

ssh webtf@10.113.64.195 "mkdir -p share/Daily_Tests/$d"
python csv2xml.py dom.csv
python csv2xml.py html.csv
python csv2xml.py css.csv
python csv2xml.py coverage.csv
scp *.xml webtf@10.113.64.195:share/Daily_Tests/$d
