#/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
time=3
tmpfile="tmp"
PASSTC=0
FAILTC=0
TCFILE=0

if [ "$1" = "" ]; then
    tc=$(find test/reftest/web-platform-tests -name "*htm*")
else
    tc=$1
fi

if [[ "$tc" == *"/" ]]; then
    tc=$(find $tc -name "*.htm*" | sort)
fi

if [[ "$tc" == *".res" ]]; then
    tc=$(cat $tc | sort)
fi

echo -e "${BOLD}###### Web Platform Tests ######${RESET}\n"

for i in $tc ; do
    # Running the tests
    ./run.sh $i > $tmpfile & 
    sleep $time 
    killall StarFish

    # Collect the result
    replace1='s/\\n"//g'
    replace2='s/"//g'
    replace3='s/\n//g'
    replace4='s/spawnSTDOUT:\s//g'
    perl -i -pe $replace1 $tmpfile
    perl -i -pe $replace2 $tmpfile
    perl -i -pe $replace3 $tmpfile
    perl -i -pe $replace4 $tmpfile

    PASS=`grep -o Pass $tmpfile | wc -l`
    FAIL=`grep -o Fail $tmpfile | wc -l`
    SUM=`expr $PASS + $FAIL`
    PASSTC=`expr $PASSTC + $PASS`
    FAILTC=`expr $FAILTC + $FAIL`
    TCFILE=`expr $TCFILE + 1`

    if [ $SUM -eq 0 ]; then
        echo -e "${YELLOW}[CHECK]${RESET}" $i "(${BOLD}No results${RESET})"
    elif [ $FAIL -eq 0 ]; then
        echo -e "${GREEN}[PASS]${RESET}" $i "(${GREEN}PASS:" $PASS"${RESET})"
    elif [ $PASS -eq 0 ]; then
        echo -e "${RED}[FAIL]${RESET}" $i "(${RED}FAIL:" $FAIL"${RESET})"
    else
        echo -e "${RED}[FAIL]${RESET}" $i "(${GREEN}PASS:" $PASS"${RESET}," "${RED}FAIL:" $FAIL"${RESET})"
    fi
done

# Remove temporary file
rm $tmpfile

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
echo -e "${YELLOW}Run" `expr $PASSTC + $FAILTC` "test cases ("$TCFILE" files):" $PASSTC "passed," $FAILTC "failed.${RESET}\n"
