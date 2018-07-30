#include <cstring>
#include <cassert>
#include "spsc_queue.h"
#include "queue_atomic_64.h"

//#define ZERO_OUT_READ_MEMORY
 
#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02
#define MSG_HEADER_ENDING 0x66

#define WRITE_DATA_MSG(header, lengthMsg, sequenceMsg) \
	header->length = lengthMsg; \
	header->sequence = sequenceMsg; \
	header->type = MSG_DATA_TYPE;

#define WRITE_PADDING_MSG(header, lengthMsg) \
	header->length = lengthMsg; \
	header->sequence = 0; \
	header->type = MSG_PADDING_TYPE;

#define RECORD_HEADER_LENGTH sizeof(RecordHeader)

#define ALIGNMENT (2 * sizeof(int32_t))
#define IS_POWER_OF_TWO(value) ((value) > 0 && (((value) & (~(value) + 1)) == (value)))
#define ALIGN(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))
#define GET_POSITION(value, capacity) value & (capacity - 1)

typedef struct
{
	size_t length;
	uint64_t sequence;
	int type;

} RecordHeader;

SpscQueue::SpscQueue(size_t capacity) : capacity(capacity)
{
   assert(IS_POWER_OF_TWO(capacity));

   this->buffer = new uint8_t[this->capacity];
   this->messageSequence = 0;

   this->head = 0;
   this->cacheHead = 0;

   this->tail = 0;
   this->cacheTail = 0;
}

size_t SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* msg, size_t offset, size_t lenght)
{
	size_t alignedLength = ALIGN(lenght, ALIGNMENT);
	size_t recordLength = alignedLength + RECORD_HEADER_LENGTH;
	size_t localTail = this->tail;

	if (recordLength >= this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	size_t localTailPosition = GET_POSITION(localTail, this->capacity);
	bool isOverridingNonReadData = (localTail + recordLength) - this->cacheHead >= this->capacity;
	if (isOverridingNonReadData)
	{
		RING_BUFFER_GET_VOLATILE(this->cacheHead, this->head);
		bool isStillOverridingNonReadData = (localTail + recordLength) - this->cacheHead >= this->capacity;
		if (isStillOverridingNonReadData)
		{
			return WriteStatus::QUEUE_FULL;
		}
	}

	uint64_t currentSequence = ++this->messageSequence;

	bool isNeedForWrap = localTailPosition + recordLength >= this->capacity;
	if (isNeedForWrap)
	{
		//don't write padding header if there is not enought space and just wrap
		size_t remainingCapacity = this->capacity - localTailPosition;
		if (remainingCapacity > RECORD_HEADER_LENGTH)
		{
			RecordHeader* header = (RecordHeader*)(this->buffer + localTailPosition);
			size_t paddingSize = this->capacity - localTailPosition - RECORD_HEADER_LENGTH;
			WRITE_PADDING_MSG(header, paddingSize)
			localTail = localTail + paddingSize + RECORD_HEADER_LENGTH;
		}
		else
		{
			localTail = localTail + remainingCapacity;
		}

		localTailPosition = GET_POSITION(localTail, this->capacity);
	}

	//store the message header
	RecordHeader* header = (RecordHeader*)(this->buffer + localTailPosition);
	WRITE_DATA_MSG(header, alignedLength, this->messageSequence)

	//store the message contents
	void* bufferOffset = (void*)(this->buffer + localTailPosition + RECORD_HEADER_LENGTH);
	std::memcpy(bufferOffset, (const void*)((uint8_t*)msg + offset), lenght);

	localTail = localTail + recordLength;
	RING_BUFFER_PUT_ORDERED(this->tail, localTail);
	
	return WriteStatus::SUCCESSFUL;
}

size_t SpscQueue::read(MessageHandler* handler)
{
	size_t localHead = this->head;
	if (localHead == this->cacheTail)
	{
		RING_BUFFER_GET_VOLATILE(this->cacheTail, this->tail);
		if (localHead == this->cacheTail)
		{
			return 0;
		}
	}

	while (localHead != this->cacheTail)
	{
		size_t localHeadPosition = GET_POSITION(localHead, this->capacity);
		//check if the remaining capacity is less than a record header even to fit in
		size_t remainingCapacity = this->capacity - localHeadPosition;
		if (remainingCapacity < RECORD_HEADER_LENGTH)
		{
			#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, remainingCapacity);
			#endif
			localHead = localHead + remainingCapacity;
			localHeadPosition = GET_POSITION(localHead, this->capacity);
		}

		RecordHeader* header = (RecordHeader*)(this->buffer + localHeadPosition);
		size_t msgLength = header->length;
		if (header->type == MSG_PADDING_TYPE)
		{
			#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
			#endif
			localHead = localHead + RECORD_HEADER_LENGTH + msgLength;
			localHeadPosition = GET_POSITION(localHead, this->capacity);
			header = (RecordHeader*)(this->buffer + localHeadPosition);
			msgLength = header->length;
		}

		uint8_t* msg = (uint8_t*)(this->buffer + localHeadPosition + RECORD_HEADER_LENGTH);
		handler->onMessage(msg, msgLength, header->sequence);

		#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
		#endif
		localHead = localHead + RECORD_HEADER_LENGTH + msgLength;
	}

	size_t readBytes = localHead - this->head;
	RING_BUFFER_PUT_ORDERED(this->head, localHead);
	return readBytes;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
