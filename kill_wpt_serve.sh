#!/bin/bash
ps -ef | grep serve | awk '{print "kill -9 " $2}' | sh
