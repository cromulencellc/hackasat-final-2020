.text 
.globl _start
_start:
    set 0x409FD256, %l3
    set 0x4000B6C4, %l4
    set 0x40000000, %l0;
    jmp %l0;
    st %l3, [%l4];

.globl init
init:
    mov %o7, %o1;
    set State_Table, %o0
init_next:
    call setup;
    mov %o1, %o7;
    
shim:
    ba UPLINK_Implant;
    nop;

.globl State_Table
State_Table: 
.align 4;

_unlocked:
.word    0x0;

_OS_GetSymbolByName:
.word    0x4003B4D8;
_CFE_SB_GetMsgId:
.word   0x0;
_CFE_SB_GetCmdCode:
.word   0x0;
_CFE_SB_GetTotalMsgLength:
.word   0x0;
_CFE_SB_SendMsg:
.word   0x0;
_CFE_EVS_SendEvent:
.word   0x0;
_CFE_PSP_MemRead8:
.word   0x0;

_PKTMGR_IngestCommands:
.word    0x0;
