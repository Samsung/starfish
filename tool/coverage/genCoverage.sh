#!/bin/sh

# coverage
node genTable.js in/specDOM.txt in/dom.raw.csv > out/dom.csv
node genTable.js in/specHTML.txt in/html.res > out/html.csv
node genTable.js in/specCSS.txt in/css.res > out/css.csv
