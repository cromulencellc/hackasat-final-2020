// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#ifndef __USB_H__
// ----------------------------------------------------------------------------
#define __USB_H__
// ----------------------------------------------------------------------------
#include <Arduino.h>
#include <stdint.h>

#include "driver/uart.h"

#include "GroundReceiver.h"  // for top-level defines 
#include "radio_hal.h"
#include "common.h"
// ----------------------------------------------------------------------------
#define PAYLOAD_MAX  2048
#define FRAME_MAX    2052  // allowing for 4-byte header
// ----------------------------------------------------------------------------
extern uint32_t g_tm_comms_last;        // for checking if comms idle
// ----------------------------------------------------------------------------
void uartInit();
void uartService(uint32_t tm_now);
void process_leon3_data( volatile uint8_t *buf, uint8_t len );
void process_oobm_data( volatile uint8_t *buf, uint8_t len );
void uartTx();
void uartRx();
void handleRadioCmd(uint8_t *buf, uint16_t len);
void uartSendHail();
void uartSendStatusCode(uint8_t code);
void uartSendStatus();
void uartSendPktLoss();

void sendToOobm(uint8_t * buf, uint16_t len);
void sendToLeon3(uint8_t * buf, uint16_t len);

// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
