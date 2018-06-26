#!/bin/bash

DIV_NUM=30
cd ./test_new/tc_list
./split_part1_reftest.sh $DIV_NUM
cd ../..

mkdir -p out/wpt_result
for i in `seq 1 $DIV_NUM`
do
    # echo [$SECONDS"s"] WPT reference test \("$i"/"$DIV_NUM"\)
    ./tool/drivers/run_test.py wpt_ref test_new/tc_list/wpt.part1.reftest."$i".tmp common -p4 --out-file=out/wpt_result/wpt.part1.reftest."$i".result
    sleep 3
done
echo Elapsed $SECONDS"s"



