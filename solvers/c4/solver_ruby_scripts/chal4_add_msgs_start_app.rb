#upload json files
cmd("CFDP SEND_FILE with CLASS 1, DEST_ID '24', SRCFILENAME '/home/op1/c4/cfs/osk_defs/cpu3_uart_to_pkt_tbl.json', DSTFILENAME '/cf/pkt_tbl.json'")
sleep(45)
cmd("CFDP SEND_FILE with CLASS 1, DEST_ID '24', SRCFILENAME '/home/op1/c4/cfs/osk_defs/cpu3_kit_sch_msg_tbl.json', DSTFILENAME '/cf/msg_tbl.json'")
sleep(45)
cmd("CFDP SEND_FILE with CLASS 1, DEST_ID '24', SRCFILENAME '/home/op1/c4/cfs/osk_defs/cpu3_kit_sch_sch_tbl.json', DSTFILENAME '/cf/sch_tbl.json'")
sleep(45)

#load tables
cmd("UART_TO_CI LOAD_TBL with CCSDS_STREAMID 6615, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 67, CCSDS_FUNCCODE 2, CCSDS_CHECKSUM 0, ID 0, TYPE 0, FILENAME '/cf/pkt_tbl.json'")
sleep(5)
cmd("KIT_SCH LOAD_TBL with CCSDS_STREAMID 6293, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 67, CCSDS_FUNCCODE 2, CCSDS_CHECKSUM 0, ID 0, TYPE 0, FILENAME '/cf/msg_tbl.json'")
sleep(5)
cmd("KIT_SCH LOAD_TBL with CCSDS_STREAMID 6293, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 67, CCSDS_FUNCCODE 2, CCSDS_CHECKSUM 0, ID 1, TYPE 0, FILENAME '/cf/sch_tbl.json'")
sleep(5)

#start app
cmd("CFE_ES START_APP with CCSDS_STREAMID 6150, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 113, CCSDS_FUNCCODE 4, CCSDS_CHECKSUM 0, APP_NAME 'CHAL4', APP_ENTRY_POINT 'CHAL4_AppMain', APP_FILENAME '/cf/chal4.obj', STACK_SIZE 16384, EXCEPTION_ACTION 0, PRIORITY 80")