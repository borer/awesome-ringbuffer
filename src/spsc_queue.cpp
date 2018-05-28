#include <cstring>
#include "spsc_queue.h"
 
SpscQueue::SpscQueue(size_t size)
{
   this->capacity = size;
   this->buffer = new uint8_t[size];
   this->head = this->tail = 0;
}

int SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, size_t offset, size_t lenght)
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
		this->tail < this->head && this->tail + recordLength >= this->head)
	{
		return WriteStatus::QUEUE_FULL;
	}

	//we need to wrap
	if (this->tail + recordLength >= this->capacity)
	{
		if (recordLength < this->head)
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

	this->tail =+ recordLength;

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
	size_t msgLength = header->getLenght();
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

	uint8_t* msg = (uint8_t*)&this->buffer[head + sizeof(record_header)];
	handler->onMessage(msg, msgLength);

	this->head += msgLength;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
