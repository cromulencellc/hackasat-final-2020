SRC 		= main.c radio_process.c serial_loader.c conflash.c radio_hal.c xtea_encrypt.c team_codes.c util.c team_keys.c rand.c uart.c circbuf.c oobm.c

# makfile configuration
TEAM0_OBJ	= $(SRC:%.c=team_0/%.o)
TEAM1_OBJ	= $(SRC:%.c=team_1/%.o)
TEAM2_OBJ	= $(SRC:%.c=team_2/%.o)
TEAM3_OBJ	= $(SRC:%.c=team_3/%.o)
TEAM4_OBJ	= $(SRC:%.c=team_4/%.o)
TEAM5_OBJ	= $(SRC:%.c=team_5/%.o)
TEAM6_OBJ	= $(SRC:%.c=team_6/%.o)
TEAM7_OBJ	= $(SRC:%.c=team_7/%.o)
TEAM8_OBJ	= $(SRC:%.c=team_8/%.o)
TEAM9_OBJ	= $(SRC:%.c=team_9/%.o)
TEAM10_OBJ	= $(SRC:%.c=team_10/%.o)
TEAM11_OBJ	= $(SRC:%.c=team_11/%.o)
TEAM12_OBJ	= $(SRC:%.c=team_12/%.o)
TEAM13_OBJ	= $(SRC:%.c=team_13/%.o)
TEAM14_OBJ	= $(SRC:%.c=team_14/%.o)
TEAM15_OBJ	= $(SRC:%.c=team_15/%.o)
TEAM16_OBJ	= $(SRC:%.c=team_16/%.o)
TEAM17_OBJ	= $(SRC:%.c=team_17/%.o)
TEAM18_OBJ	= $(SRC:%.c=team_18/%.o)
TEAM19_OBJ	= $(SRC:%.c=team_19/%.o)

INCLUDE_DIR      = ../../common
CFLAGS          = -I${INCLUDE_DIR} -Os -Wall  -mcpu=430 -mivcnt=16 -mmpy=16    # Uniarch flags

.PHONY: all team_0 team_1 team_2 team_3 team_4 team_5 team_6 team_7 team_8 team_9 team_10 team_11 team_12 team_13 team_14 team_15 team_16 team_17 team_18 team_19

#switch the compiler (for the internal make rules)
CC              = msp430-gcc

team_0: CFLAGS += -DMY_TEAM_ID=0
team_1: CFLAGS += -DMY_TEAM_ID=1
team_2: CFLAGS += -DMY_TEAM_ID=2
team_3: CFLAGS += -DMY_TEAM_ID=3
team_4: CFLAGS += -DMY_TEAM_ID=4
team_5: CFLAGS += -DMY_TEAM_ID=5
team_6: CFLAGS += -DMY_TEAM_ID=6
team_7: CFLAGS += -DMY_TEAM_ID=7
team_8: CFLAGS += -DMY_TEAM_ID=8
team_9: CFLAGS += -DMY_TEAM_ID=9
team_10: CFLAGS += -DMY_TEAM_ID=10
team_11: CFLAGS += -DMY_TEAM_ID=11
team_12: CFLAGS += -DMY_TEAM_ID=12
team_13: CFLAGS += -DMY_TEAM_ID=13
team_14: CFLAGS += -DMY_TEAM_ID=14
team_15: CFLAGS += -DMY_TEAM_ID=15
team_16: CFLAGS += -DMY_TEAM_ID=16
team_17: CFLAGS += -DMY_TEAM_ID=17
team_18: CFLAGS += -DMY_TEAM_ID=18
team_19: CFLAGS += -DMY_TEAM_ID=19

targets = team_0 team_1 team_2 team_3 team_4 team_5 team_6 team_7 team_8

all: $(targets)

team_0: $(TEAM0_OBJ)
team_1: $(TEAM1_OBJ)
team_2: $(TEAM2_OBJ)
team_3: $(TEAM3_OBJ)
team_4: $(TEAM4_OBJ)
team_5: $(TEAM5_OBJ)
team_6: $(TEAM6_OBJ)
team_7: $(TEAM7_OBJ)
team_8: $(TEAM8_OBJ)
team_9: $(TEAM9_OBJ)
team_10: $(TEAM10_OBJ)
team_11: $(TEAM11_OBJ)
team_12: $(TEAM12_OBJ)
team_13: $(TEAM13_OBJ)
team_14: $(TEAM14_OBJ)
team_15: $(TEAM15_OBJ)
team_16: $(TEAM16_OBJ)
team_17: $(TEAM17_OBJ)
team_18: $(TEAM18_OBJ)
team_19: $(TEAM19_OBJ)

team_0/%.o team_1/%.o team_2/%.o team_3/%.o team_4/%.o team_5/%.o team_6/%.o team_7/%.o team_8/%.o team_9/%.o team_10/%.o team_11/%.o team_12/%.o team_13/%.o team_14/%.o team_15/%.o team_16/%.o team_17/%.o team_18/%.o team_19/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

team_0:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768


team_1:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_2:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_3:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_4:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_5:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_6:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_7:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_8:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_9:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_10:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_11:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_12:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_13:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_14:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_15:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_16:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_17:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_18:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

team_19:
	$(CC) $^ -Tlinker.x -o $@/$@ $(CFLAGS)
	msp430-objcopy -O ihex $@/$@ $@/$@.ihex
	./ihex2coe.tcl -ihex $@/$@.ihex -out $@/$@.coe -mem_size 32768

clean:
	rm -rf team_0 team_1 team_2 team_3 team_4 team_5 team_6 team_7 team_8 team_9 team_10 team_11 team_12 team_13 team_14 team_15 team_16 team_17 team_18 team_19
