#!/bin/bash
ROOT=`pwd`

repo=$1

if [ "$repo" == "lwe_rel" ]; then
    echo "Syncing with: lwe_rel"
elif [ "$repo" == "lwe_vd" ]; then
    echo "Syncing with: lwe_vd"
elif [ "$repo" == "lightweight-web-engine" ]; then
    echo "Syncing with: lightweight-web-engine"
else
    echo "Usage: $0 [ lwe_rel | lightweight-web-engine ]"
    exit 0
fi


if [ "$repo" == "lwe_rel" ] && [ ! -d ../lwe_rel ]; then
    # echo "cloning..."
    # git clone git@github.sec.samsung.net:RS7-webtf/lwe_rel.git ../lwe_rel
    echo "Error: $repo not exist"
    exit 0
elif [ "$repo" == "lwe_vd" ] && [ ! -d ../lwe_vd ]; then
    echo "Error: $repo not exist"
    exit 0
elif [ "$repo" == "lightweight-web-engine" ] && [ ! -d ../lightweight-web-engine ]; then
    # git clone ssh://id@review.tizen.org:29418/platform/upstream/lightweight-web-engine
    echo "Error: $repo not exist"
    exit 0
else
    echo "Found: $repo"
fi

# Syncing with the current starfish repo

git submodule init
git submodule update

cd third_party/escargot
git submodule init
git submodule update third_party/GCutil
cd $ROOT

python binding_generator/scripts/starfish_code_generator.py src/ src/binding/

if [ "$repo" == "lwe_vd" ]; then
    rsync -av --delete --exclude-from "tool/release/release_ignore.txt" . ../$repo/third_party/lwe
else
    rsync -av --delete --exclude-from "tool/release/release_ignore.txt" . ../$repo
fi

hash=`git log | head -1 | cut -f2 -d' ' | cut -c 1-6`
today=`date +%y%m%d`

cd ../$repo
find ./binding_generator ./tool ./third_party -name ".git*" -exec rm -f {} \;
git add -A
echo "======================================="
echo git commit -m "LWE_Release_$today""_$hash"
echo "======================================="
msg="LWE_Release_$today""_$hash"
git commit -m "$msg"
cd $ROOT
