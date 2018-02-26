#!/bin/bash

SORTED_LIST_PATH=tool/reftest/cairo/internal.res

rm tool/reftest/cairo/internal_part* &> /dev/null 2>&1

DIV=$1
IDX=1
LIST=`cat $SORTED_LIST_PATH`

for i in $LIST; do
    echo $i >> tool/reftest/cairo/internal_part$IDX.res
    IDX=$((IDX+1))
    if [[ $IDX -gt $DIV ]]; then
        IDX=1
    fi
done


