#include <cstring>
#include <cassert>
#include "spsc_queue.h"

//#define ZERO_OUT_READ_MEMORY
 
#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02
#define MSG_HEADER_ENDING 0x66

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

#define RECORD_HEADER_LENGTH sizeof(RecordHeader)

#define ALIGNMENT (2 * sizeof(int32_t))
#define IS_POWER_OF_TWO(value) ((value) > 0 && (((value) & (~(value) + 1)) == (value)))
#define ALIGN(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))
#define GET_POSITION(value, capacity) value & (capacity - 1)

typedef struct
{
	unsigned long length;
	unsigned long sequence;
	char type;
	char padding1, padding2;
	char end;

} RecordHeader;

SpscQueue::SpscQueue(unsigned long capacity)
{
   assert(IS_POWER_OF_TWO(capacity));

   this->capacity = capacity;
   this->buffer = new uint8_t[this->capacity];
   this->messageSequence = 0;

   this->head = 0;
   this->cacheHead = 0;
   this->privateCacheHead = 0;

   this->tail = 0;
   this->cacheTail = 0;
   this->privateCacheTail = 0;
}

int SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, unsigned long offset, unsigned long lenght)
{
	size_t alignedLength = ALIGN(lenght, ALIGNMENT);
	size_t recordLength = alignedLength + RECORD_HEADER_LENGTH;

	if (recordLength >= this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	unsigned long localTailPosition = GET_POSITION(this->privateCacheTail, this->capacity);
	bool isOverridingNonReadData = (this->privateCacheTail + recordLength) - this->cacheHead >= this->capacity;
	if (isOverridingNonReadData)
	{
		this->cacheHead = this->head.load(std::memory_order_acquire);
		bool isStillOverridingNonReadData = (this->privateCacheTail + recordLength) - this->cacheHead >= this->capacity;
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
		if (remainingCapacity > RECORD_HEADER_LENGTH)
		{
			RecordHeader* header = (RecordHeader*)(this->buffer + localTailPosition);
			long paddingSize = this->capacity - localTailPosition - RECORD_HEADER_LENGTH;
			WRITE_PADDING_MSG(header, paddingSize)
			this->privateCacheTail = this->privateCacheTail + paddingSize + RECORD_HEADER_LENGTH;
		}
		else
		{
			this->privateCacheTail = this->privateCacheTail + remainingCapacity;
		}

		localTailPosition = GET_POSITION(this->privateCacheTail, this->capacity);
	}

	//store the message header
	//TODO what about message size alingment ?
	RecordHeader* header = (RecordHeader*)(this->buffer + localTailPosition);
	this->messageSequence++;
	WRITE_DATA_MSG(header, alignedLength, this->messageSequence)

	//store the message contents
	void* bufferOffset = (void*)(this->buffer + localTailPosition + RECORD_HEADER_LENGTH);
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	this->privateCacheTail = this->privateCacheTail + recordLength;
	this->tail.store(this->privateCacheTail, std::memory_order_release);

	return WriteStatus::SUCCESSFUL;
}

unsigned long SpscQueue::read(MessageHandler* handler)
{
	unsigned long localPrivateCacheHead = this->privateCacheHead;
	if (localPrivateCacheHead == this->cacheTail)
	{
		this->cacheTail = this->tail.load(std::memory_order_acquire);
		if (localPrivateCacheHead == this->cacheTail)
		{
			return 0;
		}
	}

	while (localPrivateCacheHead != this->cacheTail)
	{
		unsigned long localHeadPosition = GET_POSITION(localPrivateCacheHead, this->capacity);
		//check if the remaining capacity is less than a record header even to fit in
		unsigned long remainingCapacity = this->capacity - localHeadPosition;
		if (remainingCapacity < RECORD_HEADER_LENGTH)
		{
			#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, remainingCapacity);
			#endif
			localPrivateCacheHead = localPrivateCacheHead + remainingCapacity;
			localHeadPosition = GET_POSITION(localPrivateCacheHead, this->capacity);
		}

		RecordHeader* header = (RecordHeader*)(this->buffer + localHeadPosition);
		unsigned long msgLength = header->length;
		if (header->type == MSG_PADDING_TYPE)
		{
			#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
			#endif
			localPrivateCacheHead = localPrivateCacheHead + RECORD_HEADER_LENGTH + msgLength;
			localHeadPosition = GET_POSITION(localPrivateCacheHead, this->capacity);
			header = (RecordHeader*)(this->buffer + localHeadPosition);
			msgLength = header->length;
		}

		uint8_t* msg = (uint8_t*)(this->buffer + localHeadPosition + RECORD_HEADER_LENGTH);
		handler->onMessage(msg, msgLength, header->sequence);

		#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
		#endif
		localPrivateCacheHead = localPrivateCacheHead + RECORD_HEADER_LENGTH + msgLength;
	}

	unsigned long readBytes = localPrivateCacheHead - this->privateCacheHead;
	this->privateCacheHead = localPrivateCacheHead;
	this->head.store(this->privateCacheHead, std::memory_order_release);
	return readBytes;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
