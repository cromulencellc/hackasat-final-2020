# gzip app so that the transfer is smaller and then use FM to decompress
cmd("CFDP SEND_FILE with CLASS 1, DEST_ID '24', SRCFILENAME '/home/op1/c4/bin/chal4.obj.gz', DSTFILENAME '/cf/chal4.obj.gz'")