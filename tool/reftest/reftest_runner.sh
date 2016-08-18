#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

DEVICE=$1
VERSION=$2
TC=$3

TIMEOUT=10

if [ $DEVICE = "emulator" ]; then
    SDB="sdb -e"
else
    SDB="sdb -d"
fi
SDB_SHELL="$SDB shell su - -c"

if [ $VERSION = "2.3" ]; then
    VERSION="2.3.1"
elif [ $VERSION = "3.0" ]; then
    VERSION="3"
fi

# Prerequisite
echo -e "${BOLD}\n*** Check the commit numbers.${RESET}"
echo -e "${BOLD}*** Check the network.${RESET}"
echo -e "${BOLD}*** Check if you execute \`./build_third_party\`.${RESET}"
echo -ne "${BOLD}*** Are you ready? (y/n): ${RESET}"
read -t $TIMEOUT READ 
if [[ $READ == "n"* ]]; then
    echo -e "\n${BOLD}Please check it again.${RESET}\n"
    exit
fi
echo -e "${BOLD}*** Let's start!${RESET}"

# Initial setup
$SDB root on
export WEB_WIDGET_TEST_HOME=/opt/usr/starfish

function main {
    # Check the connected devices
    echo -e "\n${BOLD}1. Checking the connected device... ${RESET}"
    checkDevice
    echo -e "${BOLD}\tdone.${RESET}"

    # Build the binary
    echo -e "\n${BOLD}2. Building...${RESET}" 
    buildBinary
    echo -e "${BOLD}\tdone.${RESET}" 

    # Push the reftest to the devices
    echo -e "\n${BOLD}3. Pushing the testsuites to the device... ${RESET}" 
    pushTestSuite
    echo -e "${BOLD}\tdone.${RESET}" 

    # Running the tests
    echo -e "\n${BOLD}4. Running the tests...${RESET}\n"
    runTest

    # Remove unnecessary files
    rm version
    rm -rf test/test test/test.tgz
    $SDB_SHELL "rm $WEB_WIDGET_TEST_HOME/test.tgz"
    $SDB_SHELL "rm -rf $WEB_WIDGET_TEST_HOME"
}

function checkDevice {
    $SDB_SHELL "cat /etc/info.ini" &> version
    if [[ `grep -Eo "Tizen-${VERSION}" version` != "Tizen-"${VERSION} &&
          `grep -Eo "Tizen${VERSION}" version` != "Tizen"${VERSION} ]]; then
        echo -e "${RED}\tInvalid device! Need to check.${RESET}"
        echo -e "${BOLD}\tYour device information:${RESET}"
        echo -ne "\t"
        perl -i -pe "s/\\n/\\n\\t/g" version
        cat version
        echo -ne "${BOLD}But you want to test \`Tizen$VERSION\`. Keep going or not? (y/n) ${RESET}"
        read -t $TIMEOUT READ
        if [[ $READ == "n"* ]]; then
            echo -e "${BOLD}\tPlease check it again.${RESET}\n"
            exit
        fi
        rm version
    fi
}

function checkBinary {
    if [ $DEVICE = "emulator" ]; then
        if [[ `file StarFish` != *"ELF 32-bit"* || `file StarFish` = *"ARM"* ]]; then
            echo -e "${RED}Build Failed${RESET}"
            exit
        fi
    else
        if [[ `file StarFish` != *"ARM"* ]]; then
            echo -e "${RED}Build Failed${RESET}"
            exit
        fi
    fi
}

function buildBinary {
    # ./build_third_party.sh
    rm StarFish &> /dev/null 2>&1
    make clean &> /dev/null 2>&1
    if [ $DEVICE = "emulator" ]; then
        if [ $VERSION = "2.3.1" ]; then
            make tizen_wearable_emulator.exe.debug -j &> /dev/null 2>&1
            checkBinary 
            cp StarFish test/regression/bin/tizen-wearable-2.3-emulator
        else
            cp -rf deps/tizen/lib/tizen-wearable-2.3.1-i386 deps/tizen/lib/tizen-wearable-3.0-i386
            make tizen3_wearable_emulator.exe.debug -j &> /dev/null 2&>1
            checkBinary
            cp StarFish test/regression/bin/tizen-wearable-3.0-emulator
        fi
    else
        if [ $VERSION = "2.3.1" ]; then
            make tizen_wearable_arm.exe.debug -j &> /dev/null 2&>1
            checkBinary
            cp StarFish test/regression/bin/tizen-wearable-2.3-target-arm
        else
            make tizen3_wearable_arm.exe.debug -j &> /dev/null 2&>1
            checkBinary
            cp StarFish test/regression/bin/tizen-wearable-3.0-target-arm
        fi
    fi
}

function pushTestSuite {
    cd test
    echo -e "${BOLD}\tCompressing files...${RESET}"
    cp -rf regression test
    tar zcf test.tgz test &> /dev/null 2>&1
    $SDB_SHELL "mkdir -p $WEB_WIDGET_TEST_HOME" &> /dev/null 2>&1
    echo -e "${BOLD}\tPushing files...${RESET}"
    $SDB push test.tgz $WEB_WIDGET_TEST_HOME &> /dev/null 2>&1
    echo -e "${BOLD}\tExtracting files...${RESET}"
    $SDB_SHELL "tar zxf $WEB_WIDGET_TEST_HOME/test.tgz -C $WEB_WIDGET_TEST_HOME" &> /dev/null 2>&1
    $SDB_SHELL "mv test $WEB_WIDGET_TEST_HOME/test" &> /dev/null 2>&1
    cd - &> /dev/null 2>&1
}

function runTest {
    # XXX
    setEnv

    # Running the tests
    cd test
    ./test/tool/runner.sh $DEVICE $TC $VERSION

    # Print the result
    echo -e "${BOLD}You can find the result in result_${DEVICE}_${VERSION}.${RESET}\n"
    mv result ../result_${DEVICE}_${VERSION}
    cd - &> /dev/null 2>&1

    # XXX
    restoreEnv
}


function setEnv {
    rm test/result &> /dev/null 2>&1
    if [ $VERSION = "3" ]; then
        if [ $DEVICE = "emulator" ]; then
            $SDB push deps/tizen/lib/tizen-wearable-3.0-i386 $WEB_WIDGET_TEST_HOME/lib &> /dev/null 2>&1
        else
            $SDB push deps/tizen/lib/tizen-wearable-3.0-arm $WEB_WIDGET_TEST_HOME/lib &> /dev/null 2>&1
        fi
        $SDB_SHELL "ln -s $WEB_WIDGET_TEST_HOME/lib/libicui18n.so /usr/lib/libicui18n.so.51"
        $SDB_SHELL "ln -s $WEB_WIDGET_TEST_HOME/lib/libicudata.so /usr/lib/libicudata.so.51"
        $SDB_SHELL "ln -s $WEB_WIDGET_TEST_HOME/lib/libicuio.so /usr/lib/libicuio.so.51"
        $SDB_SHELL "ln -s $WEB_WIDGET_TEST_HOME/lib/libicuuc.so /usr/lib/libicuuc.so.51"
    fi
}

function restoreEnv {
    if [ $VERSION = "3" ]; then
        if [ $DEVICE = "emulator" ]; then
            rm -rf deps/tizen/lib/tizen-wearable-3.0-i386
        fi
        $SDB_SHELL "rm /usr/lib/libicui18n.so.51"
        $SDB_SHELL "rm /usr/lib/libicudata.so.51"
        $SDB_SHELL "rm /usr/lib/libicuio.so.51"
        $SDB_SHELL "rm /usr/lib/libicuuc.so.51"
    fi
}

main "$@"

