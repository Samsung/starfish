#!/bin/bash

rm tool/reftest/internal_part* &> /dev/null 2>&1

DIV=$1
IDX=1
LIST=`cat tool/reftest/internal_slow.res`
for i in $LIST; do
    echo $i >> tool/reftest/internal_part$IDX.res
    IDX=$((IDX+1))
    if [[ $IDX -gt $DIV ]]; then
        IDX=1
    fi
done

