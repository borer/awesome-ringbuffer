#include <cstring>
#include "spsc_queue.h"

#include <iostream>
 
#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02

class record_header
{
	unsigned long msgLength;
	char msgType;
public:
	void setLength(unsigned long lenght)
	{
		this->msgLength = lenght;
	}

	unsigned long getLenght()
	{
		return this->msgLength;
	}

	void setType(char type)
	{
		this->msgType = type;
	}

	char getType()
	{
		return this->msgType;
	}
};

SpscQueue::SpscQueue(unsigned long capacity)
{
   this->capacity = capacity;
   this->buffer = new uint8_t[capacity];
   this->head = this->tail = 0;
   this->totalStoredSize = 0;
   this->totalReadSize = 0;
}

int SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, unsigned long offset, unsigned long lenght)
{
	unsigned long cacheHead = this->head.load(std::memory_order_acquire);
	//std::cout << "chead " << cacheHead << std::endl;

	size_t recordHeaderLength = sizeof(record_header);
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

	//not enought space
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

		//TODO: what if space left is less that the header

		record_header* header = (record_header*)&this->buffer[this->tail];
		long paddingSize = this->capacity - this->tail - sizeof(record_header);
		header->setLength(paddingSize);
		header->setType(MSG_PADDING_TYPE);
		this->tail = 0;
		storedSize = storedSize + paddingSize;
	}

	//store the message header - msg size
	record_header* header = (record_header*)&this->buffer[this->tail];
	header->setLength(lenght);
	header->setType(MSG_DATA_TYPE);

	//store the message contents
	void* bufferOffset = (void*)&this->buffer[this->tail + recordHeaderLength];
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	this->tail = this->tail + recordLength;
	this->totalStoredSize.store(this->totalStoredSize + storedSize, std::memory_order_release);

	return WriteStatus::SUCCESSFUL;
}

void SpscQueue::read(MessageHandler* handler)
{
	unsigned long totalSize = this->totalStoredSize.load(std::memory_order_acquire);
	size_t readSize = 0;
	if (this->totalReadSize == totalSize)
	{
		return;
	}

	if (this->head == this->capacity)
	{
		this->head = 0;
	}

	record_header* header = (record_header*)&this->buffer[this->head];
	unsigned long msgLength = header->getLenght();
	size_t headerSize = sizeof(record_header);
	readSize = msgLength + headerSize;
	if (header->getType() == MSG_PADDING_TYPE)
	{
		this->head = this->head + headerSize + msgLength;
		if (this->head == this->capacity)
		{
			this->head = 0;
		}

		header = (record_header*)&this->buffer[this->head];
		msgLength = header->getLenght();
		readSize = readSize + msgLength + headerSize;
	}

	uint8_t* msg = (uint8_t*)&this->buffer[this->head + headerSize];
	handler->onMessage(msg, msgLength);

	this->head.store(this->head + msgLength + headerSize, std::memory_order_release);
	this->totalReadSize = this->totalReadSize + readSize;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
