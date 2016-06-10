#!/bin/sh

d=`date +"%y%m%d"`

ssh webtf@10.113.64.195 "mkdir -p share/Daily_Tests/$d"

python csv2xml.py dom.demo.csv && mv dom.xml dom.demo.xml
python csv2xml.py html.demo.csv && mv html.xml html.demo.xml
python csv2xml.py css.demo.csv && mv css.xml css.demo.xml
python csv2xml.py coverage.demo.csv && mv coverage.xml coverage.demo.xml

python csv2xml.py dom.csv
python csv2xml.py html.csv
python csv2xml.py css.csv
python csv2xml.py coverage.csv

scp *.xml webtf@10.113.64.195:share/Daily_Tests/$d
