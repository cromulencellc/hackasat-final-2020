#!/bin/bash

RESET="\033[0m"
BOLD="\033[1m"
YELLOW="\033[38;5;11m"
RED="\033[31m"

if [ ! -d "./kubos-linux" ]
then
    mkdir kubos-linux
fi

cd kubos-linux

if [ ! -d "./buildroot-2019.02.2" ]
then
    echo -e $YELLOW"Getting buildroot files..."$RESET
    wget https://buildroot.uclibc.org/downloads/buildroot-2019.02.2.tar.gz && tar xvzf buildroot-2019.02.2.tar.gz && rm buildroot-2019.02.2.tar.gz
    echo
fi

if [ ! -d "./kubos-linux-build" ]
then
    echo -e $YELLOW"Getting kubos linux files..."$RESET
    git clone --branch rasp-pi-zero https://github.com/kubos/kubos-linux-build.git
    mkdir -p kubos-linux-build/board/kubos/raspberrypi/overlay/home/microsd/mission-apps
    echo
fi

# if [ -f "./.empty" ]
# then
#     rm .empty
# fi

cd ..

if [ ! -d "./kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/home/microsd/mission-apps" ]
then
    mkdir -p kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/home/microsd/mission-apps
fi

diff -r ./kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/home/microsd/mission-apps/ ./overlay/microsd/mission-apps > /dev/null
app_diff=$?

if [ $app_diff -ne 0 ]
then
    read -r -n 1 -p "$(echo -e $BOLD$RED"Current mission-apps in build folder are different. Update (y/n)? "$RESET)" reply
    if [[ "$reply" =~ ^(y|Y)$ ]]
    then
        rm -rf ./kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/home/microsd/mission-apps
        cp -r ./overlay/microsd/* ./kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/home/microsd/
        echo -e $YELLOW"\nUpdated mission-apps\n"$RESET
    fi
fi

diff ./kubos-linux/kubos-linux-build/configs/raspberrypi0_defconfig ./configs/rpi0_defconfig_corrupted > /dev/null
app_diff=$?
if [ $app_diff -ne 0 ]
then
    read -r -n 1 -p "$(echo -e $BOLD$RED"Update defconfig to corrupted bootloader?. Update (y/n)? "$RESET)" reply
    if [[ "$reply" =~ ^(y|Y)$ ]]
    then
        rm ./kubos-linux/kubos-linux-build/configs/raspberrypi0_defconfig
        cp ./configs/rpi0_defconfig_corrupted ./kubos-linux/kubos-linux-build/configs/raspberrypi0_defconfig
        echo -e $YELLOW"\nUpdated to corrupted bootloader config\n"$RESET
    fi
fi

rm -f kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/etc/init.d/S50pigpio > /dev/null

if [ ! -f "./kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/etc/init.d/S99zinitpayload" ]
then
    cp ./overlay/init.d/S99zinitpayload ./kubos-linux/kubos-linux-build/board/kubos/raspberrypi/overlay/etc/init.d/
fi


echo -e "\nDone\n"