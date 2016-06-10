#!/bin/sh

mkdir -p out

# FIX an extra tab in html.csv
#head -1 html.csv | sed 's/\t$//' > out/t.txt
#tail -n +2 html.csv > out/t1.txt
#cat out/t.txt out/t1.txt > html.csv

run() {
    printf "\t\t$1" > out/label.txt
    c=`head -1 $2 | tr '\t' '\n' | wc -l`
    for i in `seq 4 $c`; do
        printf "\t" >> out/label.txt
    done;
    printf "\n" >> out/label.txt
    cat out/label.txt $2 > out/$3
}

run "DOM" dom.csv domLabel.txt
run "HTML" html.csv htmlLabel.txt
run "CSS" css.csv cssLabel.txt

cut -f3- out/htmlLabel.txt > out/htmlLabel1.txt
cut -f3- out/cssLabel.txt > out/cssLabel1.txt

paste out/domLabel.txt out/htmlLabel1.txt > out/t.txt
paste out/t.txt out/cssLabel1.txt > coverage.csv

run "DOM" dom.demo.csv domDemoLabel.txt
run "HTML" html.demo.csv htmlDemoLabel.txt
run "CSS" css.demo.csv cssDemoLabel.txt

cut -f3- out/htmlDemoLabel.txt > out/htmlDemoLabel1.txt
cut -f3- out/cssDemoLabel.txt > out/cssDemoLabel1.txt

paste out/domDemoLabel.txt out/htmlDemoLabel1.txt > out/tt.txt
paste out/tt.txt out/cssDemoLabel1.txt > coverage.demo.csv

rm -fr out
