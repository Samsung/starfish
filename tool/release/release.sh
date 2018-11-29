#!/bin/bash
ROOT=`pwd`

repo=$1

if [ "$repo" == "lwe_rel" ]; then
    echo "Syncing with: lwe_rel"
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
elif [ "$repo" == "lightweight-web-engine" ] && [ ! -d ../lightweight-web-engine ]; then
    # git clone ssh://id@review.tizen.org:29418/platform/upstream/lightweight-web-engine
    echo "Error: $repo not exist"
    exit 0
fi

echo "Found: $repo"

# Syncing with the current Starfish repo

git submodule init
git submodule update

cd third_party/escargot
git submodule init
git submodule update
mkdir -p include
make install_header_to_include
cd $ROOT

python binding_generator/scripts/starfish_code_generator.py src/ src/binding/
rsync -av --delete --delete-excluded --filter="merge tool/release/filter.txt" . ../$repo

hash=`git log | head -1 | cut -f2 -d' ' | cut -c 1-7`
today=`date +%y%m%d`

cd ../$repo
mkdir -p .git

sed -i "s/\(VERSION \".*\"\)/\1 \"$today\_$hash\"/g" src/StarfishInfo.h

git add -A
echo "======================================="
echo git commit -m "LWE_Release_$today""_$hash"
echo "======================================="
msg="LWE_Release_$today""_$hash"
git commit -m "$msg"
cd $ROOT
