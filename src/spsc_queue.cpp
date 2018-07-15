#include <cstring>
#include "spsc_queue.h"

//#define ZERO_OUT_READ_MEMORY
 
#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02
#define MSG_HEADER_ENDING 0xbb

#define WRITE_DATA_MSG(header, lengthMsg, sequenceMsg) \
	header->length = lengthMsg; \
	header->sequence = sequenceMsg; \
	header->type = MSG_DATA_TYPE; \
	header->padding1 = header->padding2 = 0x00; \
	header->end = MSG_HEADER_ENDING;

#define WRITE_PADDING_MSG(header, lengthMsg) \
	header->length = lengthMsg; \
	header->sequence = 0; \
	header->type = MSG_PADDING_TYPE; \
	header->padding1 = header->padding2 = 0x00; \
	header->end = MSG_HEADER_ENDING;


typedef struct
{
	unsigned long length;
	unsigned long sequence;
	char type;
	char padding1, padding2;
	char end;

} RecordHeader;

SpscQueue::SpscQueue(unsigned long capacity, unsigned int batchSize)
{
   this->capacity = capacity;
   this->buffer = new uint8_t[capacity];
   this->head = 0;
   this->cacheHead = 0;
   this->privateCacheHead = 0;

   this->tail = 0;
   this->cacheTail = 0;
   this->privateCacheTail = 0;

   this->messageSequence = 0;
   this->recordHeaderLength = sizeof(RecordHeader);
   this->batchSize = batchSize;
}

int SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, unsigned long offset, unsigned long lenght)
{
	size_t recordLength = lenght + recordHeaderLength;

	/*if (msg == nullptr)
	{
		return WriteStatus::INVALID_MSG;
	}
	else if (recordLength >= this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}*/

	unsigned long localTail = this->privateCacheTail;
	unsigned long localTailPosition = localTail % this->capacity;

	bool isOverridingNonReadData = (localTail + recordLength) - this->cacheHead >= this->capacity;
	if (isOverridingNonReadData)
	{
		this->cacheHead = this->head.load(std::memory_order_acquire);
		bool isStillOverridingNonReadData = (localTail + recordLength) - this->cacheHead >= this->capacity;
		if (isStillOverridingNonReadData)
		{
			return WriteStatus::QUEUE_FULL;
		}
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
			WRITE_PADDING_MSG(header, paddingSize)
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
	WRITE_DATA_MSG(header, lenght, this->messageSequence)

	//store the message contents
	void* bufferOffset = (void*)&this->buffer[localTailPosition + recordHeaderLength];
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	localTail = localTail + recordLength;
	this->privateCacheTail = localTail;
	this->tail.store(localTail, std::memory_order_release);

	return WriteStatus::SUCCESSFUL;
}

void SpscQueue::read(MessageHandler* handler)
{
	unsigned long localHead = this->privateCacheHead;
	if (localHead == this->cacheTail)
	{
		this->cacheTail = this->tail.load(std::memory_order_acquire);
		if (localHead == this->cacheTail)
		{
			return;
		}
	}

	unsigned int currentBatchIteration = 0;
	while (localHead < this->cacheTail && currentBatchIteration < this->batchSize)
	{
		unsigned long localHeadPosition = localHead % this->capacity;
		RecordHeader* header = (RecordHeader*)&this->buffer[localHeadPosition];
		unsigned long msgLength = header->length;
		if (header->type == MSG_PADDING_TYPE)
		{
			#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, msgLength + recordHeaderLength);
			#endif
			localHead = localHead + recordHeaderLength + msgLength;
			localHeadPosition = localHead % this->capacity;

			header = (RecordHeader*)&this->buffer[localHeadPosition];
			msgLength = header->length;
		}

		//TODO add multiple messages read if available
		uint8_t* msg = (uint8_t*)&this->buffer[localHeadPosition + recordHeaderLength];
		handler->onMessage(msg, msgLength, header->sequence);

		#ifdef ZERO_OUT_READ_MEMORY
			std::memset((void*)&this->buffer[localHeadPosition], 0, msgLength + recordHeaderLength);
		#endif
		localHead = localHead + msgLength + recordHeaderLength;
		localHeadPosition = localHead % this->capacity;

		//wrap to the start if the remaining capacity is less than a record header even to fit in
		unsigned long remainingCapacity = this->capacity - localHeadPosition;
		if (remainingCapacity < recordHeaderLength)
		{
			localHead = localHead + remainingCapacity;
		}

		currentBatchIteration++;
		this->head.store(localHead, std::memory_order_release);
	}

	this->privateCacheHead = localHead;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
