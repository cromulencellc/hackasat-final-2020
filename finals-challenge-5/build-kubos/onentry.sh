#!/bin/bash

RESET="\033[0m"
BOLD="\033[1m"
YELLOW="\033[38;5;11m"
RED="\033[31m"
GREEN="\033[32m"

cd /kubos-linux/buildroot-2019.02.2/ &> /dev/null

if [ ! -f ".config" ]
then
    echo -e $YELLOW"Config file does not exist. Making..."$RESET
    make BR2_EXTERNAL=../kubos-linux-build raspberrypi0_defconfig &> /dev/null
    if [ $? -eq 0 ]
    then
        echo -e $GREEN"Made BR2_EXTERNAL for raspberry pi zero."$RESET
    else
        echo -e $RED$BOLD"Making BR_EXTERNAL failed. Make sure you run startscript from the repo directory first."$RESET
    fi
fi
