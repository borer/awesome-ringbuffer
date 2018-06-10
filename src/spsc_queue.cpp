#include <cstring>
#include "spsc_queue.h"
 
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

SpscQueue::SpscQueue(unsigned long size)
{
   this->capacity = size;
   this->buffer = new uint8_t[size];
   this->head = this->tail = 0;
}

int SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, unsigned long offset, unsigned long lenght)
{
	size_t recordHeaderLength = sizeof(record_header);
	size_t recordLength = lenght + recordHeaderLength;

	if (msg == nullptr)
	{
		return WriteStatus::INVALID_MSG;
	}
	else if (recordLength >= this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	//not enought space
	if (this->tail + recordLength == this->head ||
		this->head > this->tail && this->tail + recordLength >= this->head)
	{
		return WriteStatus::QUEUE_FULL;
	}

	//we need to wrap
	if (this->tail + recordLength >= this->capacity)
	{
		if (this->head < recordLength)
		{
			return WriteStatus::QUEUE_FULL;
		}

		record_header* header = (record_header*)&this->buffer[this->tail];
		header->setLength(this->capacity - this->tail);
		header->setType(MSG_PADDING_TYPE);
		this->tail = 0;
	}

	//store the message header - msg size
	record_header* header = (record_header*)&this->buffer[tail];
	header->setLength(lenght);
	header->setType(MSG_DATA_TYPE);
	//store the message contents
	void* bufferOffset = (void*)&this->buffer[tail + recordHeaderLength];
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	this->tail = this->tail + recordLength;

	return WriteStatus::SUCCESSFUL;
}

void SpscQueue::read(MessageHandler* handler)
{
	if (this->head == this->tail)
	{
		return;
	}

	if (this->head == this->capacity - 1)
	{
		this->head = 0;
	}

	record_header* header = (record_header*)&this->buffer[head];
	unsigned long msgLength = header->getLenght();
	size_t headerSize = sizeof(record_header);
	if (header->getType() == MSG_PADDING_TYPE)
	{
		this->head += msgLength;
		if (this->head == this->capacity - 1)
		{
			this->head = 0;
		}

		header = (record_header*)&this->buffer[head];
		msgLength = header->getLenght();
	}

	uint8_t* msg = (uint8_t*)&this->buffer[head + headerSize];
	handler->onMessage(msg, msgLength);

	this->head = this->head + msgLength + headerSize;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
