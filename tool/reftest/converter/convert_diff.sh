#!/bin/bash

tc=$1".res"
origtc=$(cat tool/pixel_test/${1}.res)
newtc=$(cat tool/reftest/converter/${1}_converted.res)

array=( $origtc )
newarray=( $newtc )
len=${#array[*]}
len=$((len-1))

for i in $(seq 0 $len); do
   echo ${array[$i]}
   echo ${newarray[$i]}
   colordiff ${array[$i]} ${newarray[$i]}
   echo $i
done
