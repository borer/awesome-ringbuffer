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
   this->head = this->tail = 0;
   this->totalStoredSize = 0;
   this->totalReadSize = 0;
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
	size_t storedSize = recordLength;

	if (msg == nullptr)
	{
		return WriteStatus::INVALID_MSG;
	}
	else if (recordLength >= this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	bool isOverridingNonReadData = cacheHead > this->tail && this->tail + recordLength >= cacheHead;
	if (isOverridingNonReadData)
	{
		return WriteStatus::QUEUE_FULL;
	}

	bool isNeedForWrap = this->tail + recordLength >= this->capacity;
	if (isNeedForWrap)
	{
		bool isEnoughtSpaceOnceWraped = cacheHead < recordLength;
		if (isEnoughtSpaceOnceWraped)
		{
			return WriteStatus::QUEUE_FULL;
		}

		//don't write padding header if there is not enought space and just wrap
		unsigned long remainingCapacity = this->capacity - this->tail;
		if (remainingCapacity > recordHeaderLength)
		{
			RecordHeader* header = (RecordHeader*)&this->buffer[this->tail];
			long paddingSize = this->capacity - this->tail - sizeof(RecordHeader);
			header->writePaddingMsg(paddingSize);
			storedSize = storedSize + paddingSize;
		}
		else
		{
			storedSize = storedSize + remainingCapacity;
		}

		this->tail = 0;
	}

	//store the message header - msg size
	//TODO what about message size alingment ?
	RecordHeader* header = (RecordHeader*)&this->buffer[this->tail];
	this->messageSequence++;
	header->writeDataMsg(lenght, this->messageSequence);

	//store the message contents
	void* bufferOffset = (void*)&this->buffer[this->tail + recordHeaderLength];
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	this->tail = this->tail + recordLength;
	this->totalStoredSize.store(this->totalStoredSize + storedSize, std::memory_order_release);

	return WriteStatus::SUCCESSFUL;
}

void SpscQueue::read(MessageHandler* handler)
{
	unsigned long totalStoredSize = this->totalStoredSize.load(std::memory_order_acquire);
	if (this->totalReadSize == totalStoredSize)
	{
		return;
	}

	if (this->totalReadSize > totalStoredSize)
	{
		std::cout << "should never happen" << std::endl;
	}

	if (this->head == this->capacity)
	{
		this->head = 0;
	}

	RecordHeader* header = (RecordHeader*)&this->buffer[this->head];
	unsigned long msgLength = header->length;
	size_t readSize = msgLength + recordHeaderLength;
	if (header->type == MSG_PADDING_TYPE)
	{
		std::memset((void*)&this->buffer[this->head], 0, msgLength + recordHeaderLength);
		this->head = this->head + recordHeaderLength + msgLength;
		if (this->head == this->capacity)
		{
			this->head = 0;
		}

		header = (RecordHeader*)&this->buffer[this->head];
		msgLength = header->length;
		readSize = readSize + msgLength + recordHeaderLength;
	}

	//TODO add multiple messages read if available
	uint8_t* msg = (uint8_t*)&this->buffer[this->head + recordHeaderLength];
	handler->onMessage(msg, msgLength, header->sequence);

	//wrap to the start if the remaining capacity is less than a record header even to fit in
	unsigned long remainingCapacity = this->capacity - this->head;
	if (remainingCapacity < recordHeaderLength)
	{
		readSize = readSize + remainingCapacity;
		this->head = 0;
	}

	std::memset((void*)&this->buffer[this->head], 0, msgLength + recordHeaderLength);
	this->head.store(this->head + msgLength + recordHeaderLength, std::memory_order_release);
	this->totalReadSize = this->totalReadSize + readSize;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
