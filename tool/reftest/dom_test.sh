#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
RESET='\033[0m'

# Variables
PASSFILE="test/regression/reftest/dom/dom_regression.res"
PASSFILENEW="dom_regression.res"
PASSTC=0
SKIPTC=0
FAILTC=0
TCFILE=0

if [ "$1" = "" ]; then
    tc=$(find test/reftest/web-platform-tests -name "*htm*")
else
    tc=$1
fi

if [[ "$tc" == *"/" ]]; then
    tc=$(find $tc -name "*.htm*" | sort)
elif [[ "$tc" == *".res" ]]; then
    tc=$(cat $tc)
fi

if [ "$2" = true ]; then
    REGRESSION=true
fi

echo -e "${BOLD}###### DOM Tests ######${RESET}\n"

cnt=-1
ROTATE="$(nproc)"
filenames=()
array=( $tc )
pos=$(( ${#array[*]} - 1 ))
last=${array[$pos]}
for i in $tc ; do
    cnt=$((cnt+1))
    if [[ $cnt == $ROTATE ]]; then
        cnt=0
    fi
    RESFILE="tmp"$cnt

    # Running the tests
    filenames[$cnt]=$i
    ./run.sh $i --result-folder="out/$i" --hide-window > $RESFILE &
    if [[ $cnt != $((ROTATE-1)) ]] && [[ $i != $last ]]; then
        continue;
    fi
    wait;

    for c in $(seq 0 $cnt); do
        TMPFILE="tmp"$c

        # Expected result
        EXPECTED_FILE=${i%.*}"-expected.txt"
        EXPECTED=$(cat $EXPECTED_FILE)
        EXPECTED=${EXPECTED##*Status:}

        # Collect the result
        replace1='s/\\n"//g'
        replace2='s/"//g'
        replace3='s/\n//g'
        replace4='s/spawnSTDOUT:\s//g'
        perl -i -pe $replace1 $TMPFILE
        perl -i -pe $replace2 $TMPFILE
        perl -i -pe $replace3 $TMPFILE
        perl -i -pe $replace4 $TMPFILE

        SKIP=`grep -o Skipped $TMPFILE | wc -l`
        RESULT=`grep -o $EXPECTED $TMPFILE | wc -l`
        TCFILE=`expr $TCFILE + 1`

        if [ $SKIP -eq 1 ]; then
            SKIPTC=`expr $SKIPTC + 1`
            echo -e "${YELLOW}[SKIP]${RESET}" ${filenames[$c]}
        elif [ $RESULT -eq 1 ]; then
            PASSTC=`expr $PASSTC + 1`
            echo -e "${GREEN}[PASS]${RESET}" ${filenames[$c]}
            if [ "$REGRESSION" = true ]; then
                echo -e ${filenames[$c]} >> $PASSFILENEW
            fi

            # Copy the passed files to converted dir
            #tc=${filenames[$c]}
            #dest=${tc%%/html*}"/html_converted"${tc#*html}
            #dir=${dest%/*}
            #mkdir -p $dir
            #cp $tc $dest
            #tc=${tc%.*}
            #dest=${dest%.*}
            #cp $tc".js" $dest".js"
            #cp $tc"-expected.txt" $dest"-expected.txt"
        else 
            FAILTC=`expr $FAILTC + 1`
            echo -e "${RED}[FAIL]${RESET}" ${filenames[$c]}
        fi

        rm $TMPFILE
    done
done

wait;

# Print the summary
echo -e "\n${BOLD}###### Summary ######${RESET}\n"
echo -e "${YELLOW}Run" `expr $TCFILE` "test cases:" $PASSTC "passed," $FAILTC "failed," $SKIPTC "skipped.${RESET}\n"

# Remove temporary file
# rm $TMPFILE

# Regression test
if [ "$REGRESSION" = true ]; then
    echo -e "${BOLD}###### Regression Test ######${RESET}\n"

    diff=`diff -u $PASSFILE $PASSFILENEW`
    if [ "$diff" = "" ] ; then
        echo -e "${GREEN}[PASSED] no change${RESET}\n"
    else
        echo -e "${RED}[CHECKED] some changes${RESET}\n"
        diff -u $PASSFILE $PASSFILENEW
        echo

        # Update the regression lists
        cp $PASSFILENEW test/regression/reftest/dom

        # Copy the converted files
        file=$(cat $PASSFILENEW)
        for i in $file ; do
            dest=test/regression/${i#*/}
            destdir=${dest%/*}
            mkdir -p $destdir
            tc=${i%.*}
            dest=${dest%.*}
            cp $tc".html" $dest".html"
            cp $tc".js" $dest".js"
            cp $tc"-expected.txt" $dest"-expected.txt"
        done

        # Copy the original files
        replace='s/html_converted/html/g'
        perl -i -pe $replace $PASSFILENEW
        file=$(cat $PASSFILENEW)
        for i in $file ; do
            dest=test/regression/${i#*/}
            destdir=${dest%/*}
            mkdir -p $destdir
            tc=${i%.*}
            dest=${dest%.*}
            cp $tc".html" $dest".html"
            cp $tc".js" $dest".js"
            cp $tc"-expected.txt" $dest"-expected.txt"
        done

        # Copy the resource files
        #cp -rf test/reftest/web-platform-tests/resources test/regression/reftest/web-platform-tests/
    fi

    # Remove temporary file
    rm $PASSFILENEW
fi

