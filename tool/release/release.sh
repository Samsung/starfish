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

# Syncing with the current starfish repo

git submodule init
git submodule update binding_generator third_party/deviceapi third_party/escargot third_party/GCutil tool/gyp

cd third_party/escargot
git submodule init
git submodule update third_party/GCutil
mkdir -p include
make install_header_to_include
cd $ROOT

python binding_generator/scripts/starfish_code_generator.py src/ src/binding/
rsync -av --delete --delete-excluded --filter="merge tool/release/filter.txt" --filter="merge tool/release/filter_starfish.txt" . ../$repo

hash=`git log | head -1 | cut -f2 -d' ' | cut -c 1-6`
today=`date +%y%m%d`

cd ../$repo
mkdir -p .git

git add -A
echo "======================================="
echo git commit -m "LWE_Release_$today""_$hash"
echo "======================================="
msg="LWE_Release_$today""_$hash"
git commit -m "$msg"
cd $ROOT
