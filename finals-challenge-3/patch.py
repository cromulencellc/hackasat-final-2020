from pwnlib.elf import ELF
from struct import unpack, pack
import re
def patch_old(f, data):
    
    # Work in patch, it's stuck at the end of the file to avoid messing 

    with open(f, 'rb') as fd:
        raw = fd.read()

    e = ELF(f)
    sections_start = e.symbols["sections"]
    sections_end = sections_start

    raw_text_h_start = e.header.e_ehsize
    print(".text Header @ %08x" % raw_text_h_start)

    raw_text_start  = unpack(">I", raw[raw_text_h_start+4 : raw_text_h_start+8])[0]
    text_size,text_size_mesz = unpack(">II", raw[raw_text_h_start+0x10:raw_text_h_start+0x18])
    print("Original .text offset,size: %08x %x,%x" % (raw_text_start,text_size,text_size_mesz))

    implant_addr = raw_text_start + text_size
    print("Writing Implant @ %08x" % implant_addr)
    raw = raw[:implant_addr] + data + raw[implant_addr + len(data):]

    print("Updating size of .text to %x" % (text_size + len(data)))

    raw = raw[:raw_text_h_start+0x10] + 2*pack(">I", len(data) + text_size) + raw[raw_text_h_start+0x18:]


    first_free = 0
    print("Section start: %08x" % sections_start)
    while True:
        section = e.read(sections_end, 16 + 4*4)
        paddr,raddr,plen,comp = unpack(">IIII", section[:4*4])
        name = section[4*4:]
        name = name[:name.index(b'\x00')]
        if paddr == 0 and raddr == 0:
            break
        print("Section %s @ %08x:%08x" % (name, paddr, paddr + plen))
        first_free = paddr + plen
        sections_end += 16 + 4 * 4

    first_free = ( first_free + 0xFFFF ) & ~(0xFFFF)
    print("First free, rounded: %x" % first_free)

    patch = pack(">IIII", first_free, implant_addr, len(data), 0) + b".challenge"
    print("Sections end: %08x" % sections_end)
    raw = raw[:sections_end+raw_text_start] + patch + raw[sections_end + raw_text_start + len(patch):]
    
    with open("build/patched_" + f, "wb") as fd:
        fd.write(raw)

def patch_file(data, address):
    with open("./build/data_%08x.bin" % address, 'wb') as f:
        f.write(data)

def patch(f, implant_elf, implant):
    e = ELF(f)
    
    sections_base_addr  = e.symbols["sections"]
    implant_section_num = 3 # base 0
    section_patch_addr  = sections_base_addr + implant_section_num * 0x20

    implant_prom_addr = 0x00220000
    implant_prom_len  = len(implant)
    implant_load_addr = 0x42800000
    implant_section_flags = 0 # Not Compressed
    section_patch = pack(">IIII", 
                        implant_load_addr,
                        implant_prom_addr, 
                        implant_prom_len,
                        implant_section_flags)
    section_patch = section_patch + b".challenge"

    # Patch in Implant
    patch_file(   implant, 
                    implant_prom_addr)

    # Patch Sections
    e.write( section_patch_addr, section_patch )
    patch_file( section_patch, 
                section_patch_addr)

    # Patch Entry Point
    e.write( e.symbols["_entry"], pack(">I", implant_load_addr) )
    
    patch_file( pack(">I", implant_load_addr), 
                e.symbols["_entry"])

    e.save(f + ".patched")
    
    with open(implant_elf, "r+b") as f:
        data = f.read()

    #data = re.sub(b'\x42\x80', b'\x00,\x22', data)
    #data = re.sub(b'\x42\x7f', b'\x00,\x21', data)
    #data = re.sub(b'\x42\x81', b'\x00,\x23', data)

    with open(implant_elf, 'wb') as f:
        f.write(data[:0x18])
        f.write(pack(">I", implant_prom_addr))
        f.write(data[0x1c:0x3c])
        f.write(pack(">II", 0x00210000, 0x00210000))
        f.write(data[0x44:0x5c])
        f.write(pack(">H", 0x0023))
        f.write(data[0x5e:0x60])
        f.write(pack(">H", 0x0023))
        f.write(data[0x62:])

        #f.write(data[0x1c:0x1122c])
        #f.write(pack(">H", 0x0022))
        #f.write(data[0x1c:0x1122c])

import sys
if __name__ == '__main__':
    with open(sys.argv[2], 'rb') as fd:
        data = fd.read()
    patch(sys.argv[1], sys.argv[3], data)


