# groundstation_rf

<p>This code runs in the Ground Stations' LoRa radio modules and relays data 
with their flatsats via RF.</p>

## Technical Details

<p>This is an Arduino v1.8+ project for a Heltec Automation LoRa 32 V2. The device is based on an ESP32 MCU running FreeRTOS with two CPU cores:</p>

- Core 0: USB_UART, OLED display, and blinkenlights
- Core 1: Manages LoRa radio link

#### Radio protocol features:

- Error correcting 1 of 4 packets
- Tea encrypted (key per team)
- Addressable to either soft cpu of a Hackasat

#### The loss calculation on the OLED

<p>Every four LoRa radio packets, if error correction is not possible for the set, bad_count is incremented. In either case, total_count is also incremented. The number shown is (bad_count * 100.0 / total_count).  To give sort of a quick-and-dirty rolling average effect, every 256 total_count, both values are halved. The time scale for this is tens of seconds. If the loss % is zero, the value isn't shown.</p>

#### Hex lines on OLED

<p>The top line is the first part of the latest message received from ground. The next line whatever was last sent. The white LED is blipped for both Tx and Rx.</p>

#### USB message format

<p>Not the same as the UDP message format from the gs_udp_radio_bridge.</p>

    [ 0xA5, 0x5X, 0xHH, 0xLL, CC, ... ]

- Where X is the uplink node (0: LoRa Radio, 1: Manager CPU, 2: Flight CPU)
- HH & LL are the uint16_t big-endian length of remaining bytes
- CC is a command code - - Not present in Flight CPU messages.
- ... are optional payload bytes

<p>Note: for Leon3 messages, all these protocol bytes are stripped off at the radio before they are passed on to the Leon3. Likewise for returning L3 messages, the protocol is stripped of in the bridge before the UDP is sent.</p>

<p>Some command code examples follow. The full list is in common.h, both in this project and flatsat_openmsp430. Responses may reuse the same CC as the command. </p> 

    CMD_HAIL
    CMD_STATUS
    CMD_LOOPBACK

## Getting Started

<p>Using Ubuntu Bionic x64 VM:</p>

    sudo apt-get install openjdk-11-jdk gcc-msp430 python-pip tk
    sudo pip install pyserial
    sudo usermod -a -G dialout $USER  

<p>Restart machine (or equivalent) to get that usermod command to go into effect. This allows Arduino IDE to upload code to devices.</p>

<p>Download and install latest ZTEX tools for linux from https://www.ztex.de/downloads/ztex-191017.tar.bz2</p>

    cd ~/ztex
    make clean
    make all

<p>To upload fpga bitstream to fpga, connect its mini-usb to the Linux VM. Then do:</p>

    cd ~/ztex/java/FWLoader    
    ./FWLoader -um <WIP>openMSP430_fpga.bit

<p>Connect two UART to USB cables from CD&H board to VM like so (looking at edge of CD&H board)</p>

    G:green, B:black, W:white, o: None
     
    Console port:     Debug Port:
    o W G             o o G W o o o o o o
    o B o             o o o o o o o o B o

<p>Note the dip switches select which softcore cpu is connected to the above. For now, select msp430 accoding to this table:</p>

    For the msp430 management cpu, all five switches are flipped towards the nearby board edge.
    For Leon3 flight cpu they are flipped away from the board edge. (Verify)

<p>From repo flatsat_opensmp430:</p>

    cd repo/software/client/radio
    make team_0
    cd repo/tools/bin
    ./openmsp430-minidebug.tcl

<p>Select the ttyUSB associated with the Debug Port of the msp430. (Or try different ttyUSBx's and pressing connect until it is happy.)</p>

<p>For ELF file, select the following, then push Load button:</p>

    repo/software/client/radio/team_0/team_0.ihex

<p>Install and start Arduino IDE version >= v1.8.13 , NOT from apt-get! Use Arduino website or Ubuntu Software Center.</p>

- Go to file->preferences, and in the Additional Board Manager URLs, enter the following: https://resource.heltec.cn/download/package_heltec_esp32_index.json
- Go to tools->boards->boards_manager and install 'Heltec ESP32 Series Dev-Boards 0.0.5'
- Go to tools->manage_libraries and install 'Heltec ESP32 Dev-Boards 1.1.0'
- From groundstation_rf repo, load GroundReceiver.ino
- Do tools->board->Heltec ESP32 Arduino->Wifi LoRa 32 v2
- Connect the little heltec board to the VM
- Do tools->port and select whatever ttyUSBx port corresponds to heltec board
- Do sketch->upload

