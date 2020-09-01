// ----------------------------------------------------------------------------
//
// This code runs in the Heltec LoRa 32 v2 radio. It relays data 
// between the groundstation UDP bridge (via USB) and the two flatsat 
// processors (via LoRa).
//
// Note the radio device is running FreeRTOS and has two CPU cores. 
//
//   - Core 0 handles the USB, oled display, and blinkenlights
//   - Core 1 is dedicated to the LoRa radio protocol.
//
// Format of messages sent through USB:
//
// [ 0xA5, 0x5_, 0xHH, 0xLL, ... ]
// 
//   - Where _ is the node (0: LoRa Radio, 1: Manager CPU, 2: Flight CPU)
//   - HH & LL are the uint16_t big-endian length of payload (in bytes)
//   - ... are optional payload bytes
//
// ----------------------------------------------------------------------------
#include "GroundReceiver.h"
#include "oled.h" 
#include "usb_uart.h"      
// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------
uint32_t      g_tm_disp_last = 0;         // for oled display update rate
uint8_t       g_led_lit = false;        // for blipping the white led
uint32_t      g_tm_led_off = 0;         //
hw_timer_t    *radio_timer = NULL;      // Used by LoRa task
// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
void setup() 
{
  pinMode( LED_BUILTIN, OUTPUT );
  
  oledInit();
  uartInit();
  attachInterrupt(DIO0_PIN, Interrupt_DIO0, RISING);  // Separate interrupts
  g_radioState = RADIOSTATE_OFF;
  g_timerCounter = 0;
  radio_init( ); // MY_TEAM_ID );
  g_radioState = RADIOSTATE_WRITEPACKET;
  randomSeed( 10 );
  xTaskCreatePinnedToCore(
                    onRadioTimerTask,    // Function to implement the task 
                    "onRadioTimerTask",  // Name of the task 
                    10000,          // Stack size in words 
                    (void *)NULL,      // Task input parameter 
                    20,              // Priority of the task 
                    &handle_onRadioTimerTask,           // Task handle. 
                    1);             // Core 1 
  xTaskCreatePinnedToCore(
                    onRadioInterruptTask,    // Function to implement the task 
                    "onRadioInterruptTask",  // Name of the task 
                    10000,          // Stack size in words 
                    (void *)NULL,      // Task input parameter 
                    21,              // Priority of the task 
                    &handle_onRadioInterruptTask,           // Task handle. 
                    1);             // Core 1                
  radio_timer = timerBegin(0, 80, true);        // Prescaler -- Divide by 80
  timerAttachInterrupt(radio_timer, &onRadioTimer, true);
  timerAlarmWrite(radio_timer, 6250, true);     // 5.3msZZ
  timerAlarmEnable(radio_timer);  
}
// ----------------------------------------------------------------------------
// called repeatedly, runs on CPU core 0 
// ----------------------------------------------------------------------------
void loop() 
{
  uint32_t tm_now = millis();

  // handle usb uart tx/rx and commands
  uartService(tm_now);

  // update oled when usb comms are idle (2Hz max)
  
  int dtc = tm_now - g_tm_comms_last;
  int dtd = tm_now - g_tm_disp_last;  
  if ((dtc > COMMS_IDLE_THRESH_MS)&&(dtc > DISPLAY_PERIOD_MS))
  {
    g_tm_disp_last = g_tm_comms_last = tm_now;
    oledRedraw();
  }

  // if time to turn off led blip
  
  if (g_led_lit && (tm_now > g_tm_led_off))
  {
    digitalWrite( LED_BUILTIN, LOW );
    g_led_lit = false;    
  }

  // if time to blank stale oled debug strings
  
  if(g_dbgstr_lit&&(tm_now > g_tm_dbgstr_off))
  {
    g_dbgstr_lit=false;
  }
  
}
// ----------------------------------------------------------------------------
// Turn onboard white led on for ~10mS
// ----------------------------------------------------------------------------
void ledBlip()
{
  digitalWrite( LED_BUILTIN, HIGH );         
  g_led_lit=true;
  g_tm_led_off = millis()+10;  
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
