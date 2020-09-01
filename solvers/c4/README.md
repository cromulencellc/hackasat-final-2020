# finals-challenge-4-cfs-cosmos

This repo contains the cfs app, cosmos target directories, and example ruby scripts necessary to solve challenge

## Cosmos
Start with the cosmos directory from the opensatkit repo
* Drop the CHAL4 directory into cosmos/config/targets
* Update the fsw_msg_id.rb to include the challenge 4 message ids by replacing the file or adding to it
*  the cmd_tlm_server.txt file to include CHAL4 as a target on the LOCAL_CFS_INT

## cFS
* Add the chal4 directory to the cfs/apps directory from the opensatkit repo
* Update targets.cmake to include chal4

* Build cfs using our typical build process from opensatkit (make docker-build)
    * This will build a new core-cpu3.exe, but also the chal4.obj file in the opensatkit/cfs/build/exe/cpu3/eeprom directory

## Solving the challenge
Upload the chal4.obj binary to the eeprom directory using cosmos and the CFDP application (see upload_chal4_app.rb and decompress_chal4.rb)

Add the appropriate messaging for the chal4 app to the cFS system and start the application (see chal4_add_msgs_start_app.rb)

Enable the payload console uart using the command from cosmos
* cmd("CHAL4 ENABLE_CONSOLE with CCSDS_STREAMID 6623, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 2, CCSDS_FUNCCODE 2, CCSDS_CHECKSUM 0, ON_OFF ENABLE")
Power on Payload
* cmd("PL_IF POWER with CCSDS_STREAMID 6617, CCSDS_SEQUENCE 49152, CCSDS_LENGTH 2, CCSDS_FUNCCODE 5, CCSDS_CHECKSUM 0, ON_OFF ON")

Send a command to the payload console at the appropriate time to interrupt the autoboot countdown timer (might take a few tries)
Uncorrupt the bootloader (see solve_chal4.rb)