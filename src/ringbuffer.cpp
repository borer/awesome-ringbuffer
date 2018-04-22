#include "ringbuffer.h"
 
RingBuffer::RingBuffer()
{
   this->capacity = 10;
}

int RingBuffer::getCapacity()
{
	return this->capacity;
}

RingBuffer::~RingBuffer()
{

}
