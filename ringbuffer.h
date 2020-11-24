#ifndef RINGBUFFER_H
#define RINGBUFFER_H

class RingBufferX
{
  public:
    RingBufferX();
    ~RingBufferX();
    void begin();
    void end();
    void store(float x);
    float getat(int pos);
    float max(void);
    float min(void);

  private:
    int ringposition=0;
    float ringbuffer[32]={NULL};

};

#endif /* RINGBUFFER_H */
