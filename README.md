STARFISH
========

# Building

    git clone git@10.113.64.203:StarFish/starfish.git
    cd starfish
    ./build_third_party.sh
    make [x86|x64|tizen_arm|tizen_wearable_arm].[exe|lib].[debug|release] -j
e.g. $ make x64.exe.debug -j

# Test

## Run regressiontest (compare with pre-version)

    make regression_test                         // all xml file within test/demo/20160115
    make regression_test [tc=unittest.xml]       // only one file

## Run pixeltest (compare with webkit)

    make pixel_test                         // all xml file within test/
    make pixel_test [tc=unittest.xml]       // only one file

### Capture StarFish screenshot

    ELM_ENGINE="shot:" ./StarFish [filepath=*.xml] --pixel-test

### Capture Webkit screenshot

    phantomjs capture.js [category=css|dom|xhr]
    phantomjs capture.js -f [filepath=*.html]

# Contributing

[Wiki](http://10.113.64.203/StarFish/starfish/wikis/home)


