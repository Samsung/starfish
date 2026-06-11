#!/bin/bash
ROOT=`pwd`
repo="lwe_rel_tmp"

if [ -d ../lwe_rel_tmp ]; then
    echo "clean lwe_rel_tmp.."
    rm -rf ../lwe_rel_tmp
fi

mkdir -p ../lwe_rel_tmp
git init ../lwe_rel_tmp
echo "Found: $repo"

# Syncing with the current Starfish repo

# WPT submodule is not needed for build, only for WPT test jobs
git config submodule.third_party/wpt.update none
git submodule update --init binding_generator third_party

# Note) do not use `recursive` update. it may contain other unnecessary submodules.
cd third_party/escargot
git submodule update --init third_party
cd $ROOT

rsync -av --delete --delete-excluded --filter="merge tool/release/filter_4_starfish_build.txt" . ../$repo
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

# Restore WPT submodule config
git config --unset submodule.third_party/wpt.update
