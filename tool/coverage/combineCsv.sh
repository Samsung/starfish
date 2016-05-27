#!/bin/sh

mkdir -p out

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

cut -f3- out/htmlLabel.txt >  out/htmlLabel1.txt
cut -f3- out/cssLabel.txt > out/cssLabel1.txt

# FIX htmlLabel1.txt extra tab
head -1 out/htmlLabel1.txt | sed 's/\t$//' > out/t.txt
tail -n +2 out/htmlLabel1.txt > out/t1.txt
cat out/t.txt out/t1.txt > out/htmlLabel1.txt

paste out/domLabel.txt out/htmlLabel1.txt > out/t.txt
paste out/t.txt out/cssLabel1.txt > coverage.csv

rm -fr out
