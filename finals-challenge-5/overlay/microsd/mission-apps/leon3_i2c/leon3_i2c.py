#!/usr/bin/env python3

####################################################################
# Takes an image, compresses the image, tests its integrety, then 
# sends the image over I2C using pigpio on address 6.
####################################################################

from PIL import Image
import binascii
from ctypes import *
import argparse
import os
import time
import subprocess
import sys

os.nice(-20)

pigpio = cdll.LoadLibrary("./libpigpio.so.1")

CharArr512 = c_char * 512
ByteArr512 = c_ubyte * 512

class bsc_xfer_t(Structure):
	_fields_ = [("control", c_uint32),("rxCnt", c_int),("rxBuf",CharArr512),("txCnt",c_int),("txBuf",ByteArr512)]

### There was a refactored version without a bunch of globals but was not tested thoroughly. Sry

global SCL     
global SDA     
global I2C_ADDR
global txBuf_state

global TAKE     
global REBOOT    
global STOP     

global WAIT     
global IMG_RDY  
global IMG_SIZE 
global BAD_ADDR 

global CTRL_REG          # {0x0, TAKE}, {0x1, REBOOT}, {0x2, STOP}
global STATUS_REG        # {0x3, WAIT}, {0x4, IMG_RDY}, {0x5 --> 0x8, IMG_SIZE}, {0x9, BAD_ADDR}
global IMG_REG           # {0x10 --> 0x493E or 18750}
global blank_buf

global xfer     
global running  
global img_size 
global count    

SCL      = 19
SDA      = 18
I2C_ADDR = 9

TAKE     = 0
REBOOT   = 1 
STOP     = 2

WAIT     = 0
IMG_RDY  = 1
IMG_SIZE = 2
BAD_ADDR = 6

CTRL_REG   = [bytearray([0])] * 3       # {0x0, TAKE}, {0x1, REBOOT}, {0x2, STOP}
STATUS_REG = [bytearray([0])] * 7       # {0x3, WAIT}, {0x4, IMG_RDY}, {0x5 --> 0x8, IMG_SIZE}, {0x9, BAD_ADDR}
IMG_REG    = [bytearray(16)]  * 18750   # {0x10 --> 0x493E or 18750}
blank_buf  = bytearray(16)

xfer = None
running = False
img_size = 0
count = 0

def initialize_pigpio():

    global running
    global xfer
    global img_size

    # bsc_xfer struct{int control #RW, txCnt #RO, char rxBuf[512] #RO, char txCnt[512] #RW, char txBuf[512] #RW}
    xfer = bsc_xfer_t()

    # Use I2C address 9 with control word I2C slave enable.
    xfer.control = (0x09<<16) | 0x305

    if pigpio.gpioInitialise() < 0:
        return None

    pigpio.gpioSetPullUpDown(18, 2)
    pigpio.gpioSetPullUpDown(19, 2)

    running = True

    # Take an image on start up just to make sure that the file exists.

    img_size = 0

    take_image()

    return True

def delete():
    global running
    global xfer
    
    # Set control word so that I2C is closed
    # Control: EnableBsc=0, EnableI2C=0, AbortAndClearFifo=1
    xfer.control = (0x09<<16) | 0x80
    
    pigpio.bscXfer(byref(xfer))
    pigpio.gpioTerminate()
    
    running = False

def isRunning():
    global running
    return running

def wait_for_master(event, tick):      # (i2c_address, data)

    # start = time.time()

    global txBuf_state
    global count
    global xfer
    global running

    global CTRL_REG
    global STATUS_REG
    global IMG_REG

    global TAKE     
    global REBOOT    
    global STOP     
    global WAIT     
    global IMG_RDY  
    global IMG_SIZE 
    global BAD_ADDR
    global blank_buf

    status = pigpio.bscXfer(byref(xfer))
    count += 1

    if status and xfer.rxCnt > 0:
        
        # parse_status(status)
        ## print(f"Got rxCnt: {xfer.rxCnt} | Data: {xfer.rxBuf[0:xfer.rxCnt]}")

        try:
            addr = int(xfer.rxBuf[0:xfer.rxCnt].decode())
        except ValueError:
            addr = -1

        ## Debug
        # print(f"Address: {addr}")

        if addr > 15 and addr < 18751:  # IMAGE_REG range
            xfer.txBuf[0:16] = IMG_REG[(addr-16)]
            xfer.txCnt = 16
            status = pigpio.bscXfer(byref(xfer))
            # print(f"Got address: {addr}. Sending {xfer.txCnt} with {binascii.hexlify(bytes(xfer.txBuf[0:16]))}")
            xfer.txBuf[0:16] = blank_buf
            xfer.txCnt = 0

            ### This is a logic error that, when fixed, breaks everything somehow. This makes no sense
            ### and is technically unicorn magic.
            STATUS_REG[BAD_ADDR] = 0
            return

            ## Debug: Used for testing whether we are synced.
            # if (addr - 16) % 1000 == 0:
            #     print(f"Address: {addr}: {binascii.hexlify(IMG_REG[(addr-16)])} | Count = {count}")
                
        elif addr < 3:    # CTRL_REG range
            STATUS_REG[WAIT] = 1

            if addr == TAKE:      # TAKE = 0x0
                STATUS_REG[IMG_RDY] = 0
                ## Debug
                # print("Spawning thread to take image. I've recieved address 0.")
                xfer.txCnt = 16
                status = pigpio.bscXfer(byref(xfer))
                xfer.txCnt = 0
                take_image()

            if addr == REBOOT:    # REBOOT = 0x1
                os.system("reboot")
            
            if addr == STOP:      # REBOOT = 0x2
                delete()
                running = False

        elif addr < 5:      # STATUS_REG range

            xfer.txBuf[0] = ord(STATUS_REG[(addr-3)])
            xfer.txCnt = 1
            status = pigpio.bscXfer(byref(xfer))
            ## Debug:
            # print(f"Got address: {addr}. Sending {xfer.txCnt} with {binascii.hexlify(bytes(xfer.txBuf[0]))}")
            xfer.txBuf[0] = blank_buf[0]
            xfer.txCnt = 0
        
        elif addr < 9:      # STATUS_REG range
            xfer.txBuf[0] = ord(STATUS_REG[(2)])
            xfer.txBuf[1] = ord(STATUS_REG[(3)])
            xfer.txBuf[2] = ord(STATUS_REG[(4)])
            xfer.txBuf[3] = ord(STATUS_REG[(5)])

            xfer.txCnt = 4
            status = pigpio.bscXfer(byref(xfer))
            ## Debug:
            # print(f"Got address: {addr}. Sending {xfer.txCnt} with {binascii.hexlify(bytes(xfer.txBuf[0:4]))}")
            # memset(xfer.txBuf, 0, xfer.txCnt)
            xfer.txBuf[0:4] = blank_buf[0:4]
            xfer.txCnt = 0
        elif addr < 10:
            xfer.txBuf[0] = ord(STATUS_REG[(6)])
            xfer.txCnt = 1
            status = pigpio.bscXfer(byref(xfer))
            xfer.txBuf[0] = blank_buf[0]
            xfer.txCnt = 0
        else:
            ## Debug:
            # print(f"Got BAD ADDRESS.")
            ### Another logic error with unicorn magic.
            STATUS_REG[BAD_ADDR] = 1
            return

        STATUS_REG[BAD_ADDR] = 0

    return

def get_size():
    return os.path.getsize('out.png')

def parse_status(status):
    # 0 = Transmit busy
    # 1 = Recieve FIFO empty
    # 2 = Transmit fifo full
    # 3 = Recieve Fifo full
    # 4 = Transmit Fifo empty 
    # 5   = Receieve Busy
    # 6-10 = Num of Bytes in Transmit Fifo
    # 11-15 = Num of Bytes in Receive Fifo 
    # 16-20 = Num of Bytes successfully copied to transmit FIFO

    if status & 2**0:
        print("|Transmit busy| ", end='')
    if status & 2**1:
        print("|Recieve FIFO empty| ", end='')
    if status & 2**2:
        print("|Transmit FIFO full| ", end='')
    if status & 2**3:
        print("|Recieve FIFO full| ", end='')
    if status & 2**4:
        print("|Transmit FIFO empty| ", end='')
    if status & 2**5:
        print("|Receieve Busy| ", end='')
    
    print(f"|T-FIFO: {(status >> 6) & 31}| ", end='')
    print(f"|R-FIFO: {(status >> 11) & 31}| ", end='')
    print(f"|ST-FIFO: {(status >> 16) & 31}| ", end='')

    print("\n")

def create_img_hex():

    global IMG_REG

    with open("out.png", 'rb') as img_file:
        img_hex = img_file.read()

    # Split image hex into 16 byte messages and place in IMG_REG array
    for begin in range(0, len(img_hex), 16):
        if begin + 16 > len(img_hex):
            temp_array = bytearray(img_hex[begin:])

            while(len(temp_array) < 16):    # Make sure that last chunk is a full 16 bytes
                temp_array += bytearray([0])
            
            IMG_REG[int(begin/16)] = temp_array
        else:
            IMG_REG[int(begin/16)] = bytearray(img_hex[begin:begin+16])

def take_image(path='/home/microsd/images/'):

    global STATUS_REG
    global IMG_SIZE

    print("I am blocked")
    
    print("TAKING IMAGE")

    cmd_list = [os.getcwd() + '/get_raw_frame']

    sch_sub = subprocess.Popen(cmd_list, stdout=subprocess.PIPE)
    sch_sub.communicate()

    print("Converting to png")

    im = Image.open("out.ppm")
    im.save("out.png")
    im.close()

    print("Compressing png image")

    cmd_list = ['pngquant', '-f', '-o', './out.png', '-s10', '--', './out.png']
    sch_sub = subprocess.Popen(cmd_list, stdout=subprocess.PIPE)
    sch_sub.communicate()

    print("Testing image integrety")

    try:
        im = Image.open("out.png")
        im.verify()
        im.close()
        im = Image.open("out.png")
        im.transpose(Image.FLIP_LEFT_RIGHT)
        im.close()
        print("Image passed check")
    except Exception as e:
        print(f"Image CORRUPTED: {e}")

    img_size = get_size().to_bytes(4, 'big')

    print(f"Image size: {binascii.hexlify(img_size)}")

    STATUS_REG[IMG_SIZE] = bytearray([img_size[0]])
    STATUS_REG[IMG_SIZE + 1] = bytearray([img_size[1]])
    STATUS_REG[IMG_SIZE + 2] = bytearray([img_size[2]])
    STATUS_REG[IMG_SIZE + 3] = bytearray([img_size[3]])

    create_img_hex()

    STATUS_REG[IMG_RDY] = bytearray([1])
    STATUS_REG[WAIT]    = bytearray([0])

    print("I am unblocked")

    return

def main():

    global xfer

    parser = argparse.ArgumentParser()
    
    parser.add_argument('--config', '-c', nargs=1)
    
    args = parser.parse_args()

    ######### START #########

    print("Initializing GPIO with pigpio.")
    
    if initialize_pigpio() == None:
        sys.stderr.write("(fatal) Could not intialize I2C device.")
        sys.exit(-1)
    else:
        print("Connected successfully.")

    global txBuf_state

    txBuf_state = bytearray(512)

    xfer.txBuf = (ByteArr512).from_buffer(txBuf_state)
    xfer.txCnt = 0

    EVENTFUNC = CFUNCTYPE(None,c_int,c_uint32)
    event_func = EVENTFUNC(wait_for_master)
    pigpio.eventSetFunc(31,event_func)

    status = pigpio.bscXfer(byref(xfer))

    print(f"Initial slave address send status: {status}")
    parse_status(status)

    ### Can only be killed through master STOP command ###
    while isRunning():
        time.sleep(30)  # Can only be killed once every 30 seconds.
    
if __name__ == "__main__":
    main()

