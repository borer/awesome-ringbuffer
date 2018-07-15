#include <cstring>
#include "spsc_queue.h"

#include <iostream>
 
#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02
#define MSG_HEADER_ENDING 0xbb

typedef struct
{
	unsigned long length;
	unsigned long sequence;
	char type;
	char padding1, padding2;
	char end;

	void writeDataMsg(unsigned long lengthMsg, unsigned long sequenceMsg)
	{
		this->length = lengthMsg;
		this->sequence = sequenceMsg;
		this->type = MSG_DATA_TYPE;
		this->padding1 = this->padding2 = 0x00;
		this->end = MSG_HEADER_ENDING;
	}

	void writePaddingMsg(unsigned long lengthMsg)
	{
		this->length = lengthMsg;
		this->sequence = 0;
		this->type = MSG_PADDING_TYPE;
		this->padding1 = this->padding2 = 0x00;
		this->end = MSG_HEADER_ENDING;
	}
} RecordHeader;

SpscQueue::SpscQueue(unsigned long capacity)
{
   this->capacity = capacity;
   this->buffer = new uint8_t[capacity];
   this->head = 0;
this->tail = 0;
   this->messageSequence = 0;
   this->recordHeaderLength = sizeof(RecordHeader);
}

int SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, unsigned long offset, unsigned long lenght)
{
	unsigned long cacheHead = this->head.load(std::memory_order_acquire);
	//std::cout << "chead " << cacheHead << std::endl;

	size_t recordLength = lenght + recordHeaderLength;

	if (msg == nullptr)
	{
		return WriteStatus::INVALID_MSG;
	}
	else if (recordLength >= this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	unsigned long localTail = this->tail.load(std::memory_order_relaxed);
	unsigned long localTailPosition = localTail % this->capacity;

	bool isOverridingNonReadData = (localTail + recordLength) - cacheHead >= this->capacity;
	if (isOverridingNonReadData)
	{
		return WriteStatus::QUEUE_FULL;
	}

	bool isNeedForWrap = localTailPosition + recordLength >= this->capacity;
	if (isNeedForWrap)
	{
		//don't write padding header if there is not enought space and just wrap
		unsigned long remainingCapacity = this->capacity - localTailPosition;
		if (remainingCapacity > recordHeaderLength)
		{
			RecordHeader* header = (RecordHeader*)&this->buffer[localTailPosition];
			long paddingSize = this->capacity - localTailPosition - recordHeaderLength;
			header->writePaddingMsg(paddingSize);
			localTail = localTail + paddingSize + recordHeaderLength;
		}
		else
		{
			localTail = localTail + remainingCapacity;
		}

		localTailPosition = localTail % this->capacity;
	}

	//store the message header - msg size
	//TODO what about message size alingment ?
	RecordHeader* header = (RecordHeader*)&this->buffer[localTailPosition];
	this->messageSequence++;
	header->writeDataMsg(lenght, this->messageSequence);

	//store the message contents
	void* bufferOffset = (void*)&this->buffer[localTailPosition + recordHeaderLength];
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	localTail = localTail + recordLength;
	this->tail.store(localTail, std::memory_order_release);

	return WriteStatus::SUCCESSFUL;
}

void SpscQueue::read(MessageHandler* handler)
{
	unsigned long cacheTail = this->tail.load(std::memory_order_acquire);
	unsigned long localHead = this->head.load(std::memory_order_relaxed);
	unsigned long localHeadPosition = localHead % this->capacity;
	if (localHead == cacheTail)
	{
		return;
	}

	if (localHead > cacheTail)
	{
		std::cout << "should never happen " << localHead << " ,stored " << cacheTail << std::endl;
	}

	RecordHeader* header = (RecordHeader*)&this->buffer[localHeadPosition];
	unsigned long msgLength = header->length;
	if (header->type == MSG_PADDING_TYPE)
	{
		std::memset((void*)&this->buffer[localHeadPosition], 0, msgLength + recordHeaderLength);
		localHead = localHead + recordHeaderLength + msgLength;
		localHeadPosition = localHead % this->capacity;

		header = (RecordHeader*)&this->buffer[localHeadPosition];
		msgLength = header->length;
	}

	//TODO add multiple messages read if available
	uint8_t* msg = (uint8_t*)&this->buffer[localHeadPosition + recordHeaderLength];
	handler->onMessage(msg, msgLength, header->sequence);

	std::memset((void*)&this->buffer[localHeadPosition], 0, msgLength + recordHeaderLength);
	localHead = localHead + msgLength + recordHeaderLength;
	localHeadPosition = localHead % this->capacity;

	//wrap to the start if the remaining capacity is less than a record header even to fit in
	unsigned long remainingCapacity = this->capacity - localHeadPosition;
	if (remainingCapacity < recordHeaderLength)
	{
		localHead = localHead + remainingCapacity;
		localHeadPosition = localHead % this->capacity;
	}

	this->head.store(localHead, std::memory_order_release);
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
