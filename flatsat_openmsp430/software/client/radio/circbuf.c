// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#include "circbuf.h"

// ----------------------------------------------------------------------------
uint16_t bufRem(t_circbuf * buf)  // returns free space in buf
{
  return CIRC_BUF_LEN-1-buf->count;
}
// ----------------------------------------------------------------------------
void bufClear(t_circbuf * buf)
{
  buf->count=0;
  buf->in=0;
  buf->out=0;
}
// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------
uint8_t bufPeekByte(t_circbuf * buf, uint16_t idx)    // returns byte at index
{
  if (idx>=buf->count)
  {
    return -1;
  }
 
  uint16_t j,i=buf->out;
  for(j=0;j<idx;j++,i++)
  {
    if (i==CIRC_BUF_LEN)
      i=0;
  }
  uint8_t r = buf->buf[i];
  return r;
}
// ----------------------------------------------------------------------------
int bufAdd(t_circbuf * buf, uint8_t * src, uint16_t len)
{
  uint8_t * p = src;
  uint16_t i;
  int r;  
  // check if theres room 
  if (bufRem(buf)<len)
  {
    return -1;
  }
  for(i=0;i<len;i++)
  {
    r=bufAddByte(buf, *p++);
    if(r<0)
    {
      return -1;
    }
  }
  return len;
}
// ----------------------------------------------------------------------------
uint16_t bufGet(t_circbuf * buf, uint8_t * dest, uint16_t len)
{
  uint8_t * p = dest;
  uint16_t i;
  int r;
  uint16_t l = buf->count;  // check if theres bytes
  if (!l)
  {
    return 0; // nothing to get
  }
  if (l>len)  // too much to get
    l=len;
  for(i=0;i<l;i++)
  {
    r=bufGetByte(buf);
    if(r<0)
    {
      return i;      
    }
    *p++ = (uint8_t)r;
  }
  return l;
}
// ----------------------------------------------------------------------------
uint16_t bufPeek(t_circbuf * buf, uint8_t * dest, uint16_t len)
{
  uint8_t * p = dest;
  uint16_t i;
  int r;
  uint16_t l = buf->count;  // check if theres bytes
  if (!l)
  {
    return 0; // nothing to get
  }
  if (l>len)  // too much to get
    l=len;
  for(i=0;i<l;i++)
  {
    r=bufPeekByte(buf,i);
    if(r<0)
    {
      return i;      
    }
    *p++ = (uint8_t)r;
  }
  return l;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
