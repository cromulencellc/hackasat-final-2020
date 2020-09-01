# makfile configuration
NAME            = vip_badge
OBJECTS         = main.o pong.o delay.o rain.o scoreboard.o announcement.o message.o  color_picker.o lcd/lcd_hal.o lcd/lcd_gfx.o lcd/glcdfont.o
INCLUDE_DIR      = ../common
LDFLAGS					= -Wl --gc-sections
VIP_NAME = \"VIP\"
CFLAGS          = -I${INCLUDE_DIR} -Os -Wall -s -g0 -mcpu=430 -mivcnt=16 -mmpy=16 -DVIP_NAME=${VIP_NAME}

#switch the compiler (for the internal make rules)
CC              = msp430-gcc

.PHONY: all FORCE clean download download-jtag download-bsl dist

#all should be the first target. it's built when make is runwithout args
all: ${NAME}.elf ${NAME}.a43 ${NAME}.lst 

#confgigure the next line if you want to use the serial download
#download: download-uart
#download: download-jtag
#download: download-bsl


${NAME}.elf: ${OBJECTS} 
	mkdir -p bin
	mkdir -p build
	${CC} -T../common/linker.x -o bin/$@ ${OBJECTS} 
	msp430-objcopy -O ihex bin/${NAME}.elf build/${NAME}.ihex
	tclsh ../common/tools/ihex2mem.tcl -ihex build/${NAME}.ihex -out bin/${NAME}.mem -mem_size 16384
    
${NAME}.a43: ${NAME}.elf
	msp430-objcopy -O ihex bin/$^ build/$@

${NAME}.lst: ${NAME}.elf
	msp430-objdump -dSt bin/$^ >build/$@

download-jtag: all
	msp430-jtag -e bin/${NAME}.elf

download-bsl: all
	msp430-bsl -e ${NAME}.elf

download-uart: all
	openmsp430-loader.tcl -device /dev/ttyUSB0 -baudrate 115200 ${NAME}.elf

clean:
	rm -rf *.o lcd/*.o bin build

#backup archive
dist:
	tar czf dist.tgz *.c *.h *.txt makefile

#dummy target as dependecy if something has to be build everytime
FORCE:

#project dependencies
main.o: main.c ../common/hardware.h vip.h ${OBJECTS} 
