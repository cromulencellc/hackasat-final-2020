#ifndef __OOBM_H__
#define __OOBM_H__

#include <stdint.h>
#include "circbuf.h"
#include "common.h"
#include "uart.h"

// called by main loop to handle oob transfers
void oobmRun();

// called by radio when theres bytes for oobm
void oobmGotRadioData( uint8_t *pData, uint8_t dataLen );


// parses packets out of oobm rx buf from radio
uint16_t oobmCheckFrame();

// when we get a full packet
void oobmCmd(uint8_t * buf, uint16_t len);

#endif
