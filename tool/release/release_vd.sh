#!/bin/bash
ROOT=`pwd`

repo=$1

if [ "$repo" == "lwe_vd_rel" ]; then
    echo "Syncing with: lwe_vd_rel"
else
    echo "Usage: $0 lwe_vd_rel"
    exit 0
fi

if [ "$repo" == "lwe_vd_rel" ] && [ ! -d ../lwe_vd_rel ]; then
    echo "Error: $repo not exist"
    exit 0
fi

echo "Found: $repo"

./build_init.sh
cd $ROOT

rsync -av --delete --delete-excluded --filter="merge third_party/lwe/tool/release/filter.txt" --filter="merge third_party/lwe/tool/release/filter_vd.txt" . ../$repo

hash=`git log | head -1 | cut -f2 -d' ' | cut -c 1-6`
today=`date +%y%m%d`

cd ../$repo
mkdir -p .git

git add -A
echo "======================================="
echo git commit -m "LWE_VD_Release_$today""_$hash"
echo "======================================="
msg="LWE_VD_Release_$today""_$hash"
git commit -m "$msg"
cd $ROOT
