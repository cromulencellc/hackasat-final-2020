// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//#include <Wire.h>
#include "oled.h"
// ----------------------------------------------------------------------------
String    g_macaddr_str;            // e.g. "24:6F:28:77:90:A8"
String    g_teamid_str;             // e.g. "TEAM 0"

uint8_t   g_dbgstr_lit = false;     // true while showing dbg string oled
uint32_t  g_tm_dbgstr_off = 0;      // timer to turn off stale dbg string

char      g_dbg_str0[32]="";        // next-to-last line on oled
char      g_dbg_str1[32]="";        // last line on oled
// ----------------------------------------------------------------------------
void oledInit()
{
  char s[8];  

  // cache radio team id and mac address as Strings
  
  sprintf(s,"TEAM %d",MY_TEAM_ID+1);
  g_teamid_str = String(s);
  g_macaddr_str = getMacAddress();

  // heltec oled init
  
  Heltec.begin(   
                true,  // Display enable
                false, // LoRa Disabled (initially)
                false  // Serial Disabled (initially)
              );

  //Wire.setClock(3400000);
  Heltec.display->clear();
  Heltec.display->flipScreenVertically();
}

// ----------------------------------------------------------------------------
// Print team #, mac address, etc on oled.
// (Called by mainloop periodically while USB comms are idle)
// ----------------------------------------------------------------------------
void oledRedraw()
{

  Heltec.display->clear();
  
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, 0, g_teamid_str);

  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 16, g_macaddr_str);

  // todo: oled rows 28-32 are for fun.
  
  if(g_dbgstr_lit)    // for briefly showing debug strings
  {
    Heltec.display->drawString(0, 34, g_dbg_str0);
    Heltec.display->drawString(0, 44, g_dbg_str1);
  }

  if(g_data_loss_pct >= 0.0001)
  {
    char s[20];
    sprintf(s,"loss: %3.04f%%", g_data_loss_pct);
    Heltec.display->drawString(0, 54, s);
  }

  // Send updated frame buffer via I2C to the oled
  Heltec.display->display();    
}

#if DEBUG_LOGGING
// ----------------------------------------------------------------------------
// Set string at pos 0 of oled
// ----------------------------------------------------------------------------
void _dbgStr0(char * buf)
{
  uint16_t ct = strlen(buf);
  bool chopped = false;
  
  if(ct>18) // if too big, truncate and show ellipsis
  {
    ct=16;
    chopped=true;
  }

  memcpy(g_dbg_str0,buf,ct);
  g_dbg_str0[ct]='\0';

  if(chopped)
    strcat(g_dbg_str0,"...");
  
  g_dbg_str0[ct]='\0';
  g_dbgstr_lit=true;
  g_tm_dbgstr_off=millis()+10000; // for ten seconds
}
// ----------------------------------------------------------------------------
// Set string at pos 1 of oled
// ----------------------------------------------------------------------------
void _dbgStr1(char * buf)
{
  uint16_t ct = strlen(buf);
  bool chopped = false;
  
  if(ct>18) // if too big, truncate and show ellipsis
  {
    ct=16;
    chopped=true;
  }

  memcpy(g_dbg_str1,buf,ct);
  g_dbg_str1[ct]='\0';

  if(chopped)
    strcat(g_dbg_str1,"...");
  
  g_dbg_str1[ct]='\0';
  g_dbgstr_lit=true;
  g_tm_dbgstr_off=millis()+10000; // for ten seconds
}
// ----------------------------------------------------------------------------
// Show bytes as hex at pos 0 of oled
// ----------------------------------------------------------------------------
void _dbgHex0(uint8_t * buf, uint16_t ct)
{
  char lbuf[3];
  bool chopped = false;
  
  if(ct>14) // if too big, truncate and show ellipsis
  {
    ct=12;
    chopped=true;
  }
  
  g_dbg_str0[0]='\0';
  for(int i=0;i<ct;i++)
  {
    sprintf(lbuf,"%02x",buf[i]);
    strcat(g_dbg_str0,lbuf);
  }
  if(chopped)
    strcat(g_dbg_str0,"...");
  g_dbgstr_lit=true;
  g_tm_dbgstr_off=millis()+10000; // for ten seconds
}
// ----------------------------------------------------------------------------
// Show bytes as hex at pos 1 of oled
// ----------------------------------------------------------------------------
void _dbgHex1(uint8_t * buf, uint16_t ct)
{
  char lbuf[3];
  bool chopped = false;
  
  if(ct>14) // if too big, truncate and show ellipsis
  {
    ct=12;
    chopped=true;
  }
  
  g_dbg_str1[0]='\0';
  for(int i=0;i<ct;i++)
  {
    sprintf(lbuf,"%02x",buf[i]);
    strcat(g_dbg_str1,lbuf);
  }
  if(chopped)
    strcat(g_dbg_str1,"...");
  g_dbgstr_lit=true;
  g_tm_dbgstr_off=millis()+10000; // for ten seconds
}
#endif // DEBUG_LOGGING
