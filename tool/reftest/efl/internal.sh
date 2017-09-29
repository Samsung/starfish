#!/bin/bash

SORTED_LIST_PATH=_internal_sorted.res

rm tool/reftest/efl/internal_part* &> /dev/null 2>&1
cat tool/reftest/efl/internal_unsorted.res | sort -nr | cut -d";" -f2 > $SORTED_LIST_PATH

DIV=$1
IDX=1
LIST=`cat $SORTED_LIST_PATH`

for i in $LIST; do
    echo $i >> tool/reftest/efl/internal_part$IDX.res
    IDX=$((IDX+1))
    if [[ $IDX -gt $DIV ]]; then
        IDX=1
    fi
done
rm $SORTED_LIST_PATH &> /dev/null 2>&1

