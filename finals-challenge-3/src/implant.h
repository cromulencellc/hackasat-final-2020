#include <stdint.h>

//#define DEBUG 1

#ifdef DEBUG
#define VERBOSE 1
void DEBUG_Telemetry_On();
void DEBUG_Implant();
void DEBUG_DumpMem();
#endif 

#define DOWNLINK_LOG(...) \
    fn_table->CFE_EVS_SendEvent(\
    126, \
    3, \
    __VA_ARGS__) 

#ifdef VERBOSE
#define VERBOSE_LOG(...) DOWNLINK_LOG(__VA_ARGS__)
#else 
#define VERBOSE_LOG(...) {while(0);}
#endif 

// White List
#define UART_CI_TO      0x19D7
#define PDU_MID         8189
#define CS_MID          6303
//TODO Torque Rod Command

// Special Handling
#define EYASSAT_MID     6613
// Allow 2, 3, 12

// Disallowed
#define MD_CMD_MID      6288
#define MM_CMD_MID      6280    /**< \brief Msg ID for cmds to mm     */
#define SC_CMD_MID      6313
#define PL_IF_MID       6617
#define KIT_SCH_MID     6293
#define HS_MID          6318
#define LC_MID          6308
#define HK_MID          6298
#define FM_MID          6284
#define EPHEM           6620
#define DS_MID          6331
#define CFE_TIME_MID    6149
#define CFE_TBL_MID     6148
#define CFE_SB_MID      6147
#define CFE_EVS_MID     6145
#define CFE_ES          6150



// Other
#define IMPLANT_CMD_ID    0x1834
#define MM_HK_TLM_MID     0x0887    /**< \brief MM Housekeeping Telemetry */

#define IMPLANT_TLM_ID    MM_HK_TLM_MID


typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint8_t *CFE_SB_MsgPtr_t;

typedef struct { 
    int     unlocked;
    int     (*OS_SymbolLookup)(void *, const char *);

    int     (*CFE_SB_GetMsgId)(CFE_SB_MsgPtr_t);
    int     (*CFE_SB_GetCmdCode)(CFE_SB_MsgPtr_t MsgPtr);
    int     (*CFE_SB_GetTotalMsgLength)(CFE_SB_MsgPtr_t);
    int     (*CFE_SB_SendMsg)(CFE_SB_MsgPtr_t);
    int     (*CFE_EVS_SendEvent)(uint16, uint16, const char*, ...);
    int     (*CFE_PSP_MemRead8)(void *, uint8*);

    int     (*PKTMGR_IngestCommands)(void);
} fn_table_t;


int UPLINK_Implant(CFE_SB_MsgPtr_t Msg);
extern fn_table_t State_Table;
