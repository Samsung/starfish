#!/bin/bash

path=$STARFISH_PATH

# 1. compile
cd $path
git checkout -- .
git fetch
git rebase origin master
cp .gitmodules_bot .gitmodules
git submodule init
git submodule update

cd third_party/escargot && make clean && TC=1 make x64.interpreter.debug.static -j
cd ../../
make clean && TC=1 make x64.exe.debug -j

# 2. run tests
cd tool/coverage && ./1_genDomSpecList.sh
cd ../../ && ./tool/coverage/1_genCssHtmlSpecList.sh

# 3. gen reports
cd $path/tool/coverage && ./2_genCoverage.sh && ./3_combineCsv.sh

# 4. gen xml and copy to the server
./4_genXmlAndCopy.sh
