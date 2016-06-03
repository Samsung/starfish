#!/bin/bash
cd test/reftest/web-platform-tests
./serve > /dev/null 2> /dev/null &
sleep 2
