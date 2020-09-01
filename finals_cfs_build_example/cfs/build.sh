#!/bin/bash

make distclean
make prep
make install

pushd build/exe/cpu3
tar cf eeprom-tar eeprom/
rtems-bin2c eeprom-tar eeprom-tar
cp eeprom-tar.h /src/psp/fsw/leon3-rtems/inc/eeprom-tar.h
cp eeprom-tar.c /src/psp/fsw/leon3-rtems/src/eeprom-tar.c
popd

make install
