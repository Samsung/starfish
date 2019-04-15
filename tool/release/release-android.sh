#!/bin/bash

pushd `pwd`
repo=$1

if [ ! -d ../"$repo" ]; then
    echo "Error: $repo does not exist"
    exit 0
fi

echo "Found: $repo"

# Syncing with the current Starfish repo
git submodule init
git submodule sync
git submodule update

cd third_party/escargot

git submodule init
git submodule sync
git submodule update
mkdir -p include
make install_header_to_include

popd

make clean
python binding_generator/scripts/starfish_code_generator.py src/ src/binding/

rsync -av --delete --delete-excluded --filter="merge tool/release/filter-android.txt" build/android/apk_root/ ../$repo
rsync -av --delete --delete-excluded --filter="merge tool/release/filter-android.txt" . ../$repo/src/starfish
cp -f .gitignore ../$repo

hash=`git log | head -1 | cut -f2 -d' ' | cut -c 1-7`
today=`date +%y%m%d`

pushd ../$repo
mkdir -p .git

sed -i "s/\(VERSION \".*\"\)/\1 \"$today\_$hash\"/g" src/starfish/src/StarfishInfo.h

git add -A
echo "======================================="
echo git commit -m "LWE_Release_$today""_$hash"
echo "======================================="
msg="LWE_Release_$today""_$hash"
git commit -m "$msg"
popd
