cmd("CHAL4 SHELL_CMD with CCSDS_STREAMID 6623, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 257, CCSDS_FUNCCODE 3, CCSDS_CHECKSUM 0, CMD_STRING 0x736574656E76206D6D635F626F6F7420226D6D632064657620303B206661746C6F6164206D6D6320303A3120247B6664745F616464725F727D2062636D323730382D7270692D7A65726F2E6474623B206661746C6F6164206D6D6320303A3120247B6B65726E656C5F616464725F727D206B65726E656C3B20626F6F746D20247B6B65726E656C5F616464725F727D202D20247B6664745F616464725F727D22")
sleep(5)
cmd("CHAL4 SHELL_CMD with CCSDS_STREAMID 6623, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 257, CCSDS_FUNCCODE 3, CCSDS_CHECKSUM 0, CMD_STRING 0x736574656E76206B75626F735F637572725F747269656420300A")
sleep(5)
cmd("CHAL4 SHELL_CMD with CCSDS_STREAMID 6623, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 257, CCSDS_FUNCCODE 3, CCSDS_CHECKSUM 0, CMD_STRING 'saveenv'")
sleep(5)
cmd("CHAL4 SHELL_CMD with CCSDS_STREAMID 6623, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 257, CCSDS_FUNCCODE 3, CCSDS_CHECKSUM 0, CMD_STRING 'reset'")
