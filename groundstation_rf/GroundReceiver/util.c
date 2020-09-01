// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#include "util.h"
// ----------------------------------------------------------------------------
uint16_t g_lastrand = 0x18F0;
// ----------------------------------------------------------------------------
void seed_quick_rand( uint16_t new_seed )
{
  g_lastrand = new_seed;
}

uint16_t get_quick_rand( void )
{
  uint16_t r;
  
  r = (((((((((((g_lastrand << 3) - g_lastrand) << 3)
    + g_lastrand) << 1) + g_lastrand) << 4)
    - g_lastrand) << 1) - g_lastrand) + 0xe60);
  
  g_lastrand = r - 1;
  
  return (r);
}

// ----------------------------------------------------------------------------
int bufRem(t_circbuf * buf)  // returns free space in buf
{
  return CIRC_BUF_LEN-1-buf->count;
}

void bufClear(t_circbuf * buf)
{
  buf->count=0;
  buf->in=0;
  buf->out=0;
}

int bufAddByte(t_circbuf * buf, uint8_t b) // ret -1 if fail else pass
{
  if(bufRem(buf)) // if theres at least a byte free
  {
    buf->buf[buf->in++]=b;
    if (buf->in == CIRC_BUF_LEN)  // wraparound
      buf->in = 0;
    buf->count++;    
  } else
  {
    return -1;
  }
  return 0;
}

int bufGetByte(t_circbuf * buf)            // ret -1 if fail else byte
{
  int res = -1;
  if (!buf->count)  // nothing to get
  {
    return -1;
  } else
  {
    res = buf->buf[buf->out++];
    if (buf->out == CIRC_BUF_LEN)
      buf->out = 0;
    buf->count--;
    return res;
  }
}

int bufPeekByte(t_circbuf * buf, int idx)    // returns byte at index
{
  if (idx>=buf->count)
    return -1;
 
  int i=buf->out;
  for(int j=0;j<idx;j++,i++)
  {
    if (i==CIRC_BUF_LEN)
      i=0;
  }
  return buf->buf[i];
}

int bufAdd(t_circbuf * buf, uint8_t * src, uint16_t len)
{
  uint8_t * p = src;
  uint16_t i;
  int r;  
  // check if theres room 
  if (bufRem(buf)<len)
    return -1;
    
  for(i=0;i<len;i++)
  {
    r=bufAddByte(buf, *p++);
    if(r<0)
      return -1;
  }
  return len;
}

int bufGet(t_circbuf * buf, uint8_t * dest, uint16_t len)
{
  uint8_t * p = dest;
  uint16_t i;
  int r;
  uint16_t l = buf->count;  // check if theres bytes
  if (!l)
    return 0; // nothing to get
  if (l>len)  // too much to get
    l=len;
  for(i=0;i<l;i++)
  {
    r=bufGetByte(buf);
    if(r<0)
      return i;      
    *p++ = (uint8_t)r;
  }
  return l;
}

int bufPeek(t_circbuf * buf, uint8_t * dest, uint16_t len)
{
  uint8_t * p = dest;
  uint16_t i;
  int r;
  uint16_t l = buf->count;  // check if theres bytes
  if (!l)
    return 0; // nothing to get
  if (l>len)  // too much to get
    l=len;
  for(i=0;i<l;i++)
  {
    r=bufPeekByte(buf,i);
    if(r<0)
      return i;      
    *p++ = (uint8_t)r;
  }
  return l;
}


// ----------------------------------------------------------------------------
uint8_t read_hex_uint8( char *hexString )
{
  int i = 0;
  uint8_t value = 0;
  
  for ( i = 0; i < 2; i++ )
    value = value | (hex_char_to_int( hexString[i] ) << ((1-i)*4));
  
  return value;
}

uint8_t int_to_hex_char( uint8_t nibble )
{
	if ( nibble < 10 )
		return ('0'+nibble);
	else if ( nibble < 16 )
		return ('a'+(nibble-10));
	else
		return 0;
}

uint8_t hex_to_int( uint8_t dataIn )
{
  if ( dataIn >= '0' && dataIn <= '9' )
    return (dataIn - '0');
  else if ( dataIn >= 'a' && dataIn <= 'f' )
    return (10 + (dataIn - 'a'));
  else if ( dataIn >= 'A' && dataIn <= 'F' )
    return (10 + (dataIn - 'A'));
  else
    return (0);
}

uint8_t int_to_hex( uint8_t dataIn )
{
  if ( dataIn < 10 )
    return '0' + dataIn;
  else if ( dataIn < 16 )
    return ('A' + (dataIn-10));
  else
    return ' ';
}

void    strCatOctet(char * dest,uint8_t b)
{
  char buf[3];
  sprintf(buf,"%02x",b);
  strcat(dest,buf);
}
// ----------------------------------------------------------------------------
void xtea_encrypt(volatile uint32_t v[2], const uint32_t k[4])
{
  volatile uint32_t y=v[0],z=v[1];
  uint32_t sum=0,delta=0x9E3779B9;
  uint16_t n=16;
  
  while(n-->0)
  {
    y += (z << 4 ^ z >> 5) + z ^ sum + k[sum&3];
    sum += delta;
    z += (y << 4 ^ y >> 5) + y ^ sum + k[sum>>11 & 3];
  }
  
  v[0]=y; v[1]=z;
}

void xtea_decrypt(volatile uint32_t v[2], const uint32_t k[4])
{
  volatile uint32_t y=v[0],z=v[1];
  uint32_t sum=0xE3779B90, delta=0x9E3779B9;         
  uint16_t n=16;
  
  /* sum = delta<<5, in general sum = delta * n */
  while(n-->0)
  {
    z -= (y << 4 ^ y >> 5) + y ^ sum + k[sum>>11 & 3];
    sum -= delta;
    y -= (z << 4 ^ z >> 5) + z ^ sum + k[sum&3];
  }
  
  v[0]=y; v[1]=z;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
