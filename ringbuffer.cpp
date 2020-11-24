#include <Arduino.h>
#include "ringbuffer.h"
#define MAXBUF 32

/*-----------------------------
constructor for class, not needed by Arduino but for complete class. does not do anything.
*/
RingBufferX::RingBufferX() 
{
}

/*-----------------------------
destructor for class, not needed by Arduino but for complete class. Calls Arduino end function
*/
RingBufferX::~RingBufferX() 
{
  end();
}

/*-----------------------------
Arduino begin function. Forward data to initialize function
*/
void RingBufferX::begin() 
{
  RingBufferX::ringposition=0;
  for(int i=0;i<MAXBUF;i++) RingBufferX::ringbuffer[i]=NULL;
}

/*-----------------------------
Arduino end function. stop SPI if enabled
*/
void RingBufferX::end() 
{

}

void RingBufferX::store(float x)
{
  RingBufferX::ringposition++;
  if(RingBufferX::ringposition>=MAXBUF) RingBufferX::ringposition=0;
  RingBufferX::ringbuffer[RingBufferX::ringposition]=x;
}
float RingBufferX::getat(int pos)
{
  int x=1+pos+RingBufferX::ringposition;
  while(x>=MAXBUF) x-=MAXBUF;
  return RingBufferX::ringbuffer[x];
}
float RingBufferX::min(void)
{
  float x=RingBufferX::ringbuffer[0];
  for(int i=1;i<MAXBUF;i++) if(RingBufferX::ringbuffer[i]<x) x=RingBufferX::ringbuffer[i];
  return x;
}
float RingBufferX::max(void)
{
  float x=RingBufferX::ringbuffer[0];
  for(int i=1;i<MAXBUF;i++) if(RingBufferX::ringbuffer[i]>x) x=RingBufferX::ringbuffer[i];
  return x;
}
