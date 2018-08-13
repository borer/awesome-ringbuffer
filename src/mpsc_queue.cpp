#include <cstring>
#include <cassert>

#include "mpsc_queue.h"
#include "queue_atomic_64.h"
#include "binutils.h"

//#define ZERO_OUT_READ_MEMORY

MpscQueue::MpscQueue(size_t capacity) : capacity(capacity)
{
   assert(IS_POWER_OF_TWO(capacity));

   this->buffer = new uint8_t[this->capacity];

   this->head = 0;
   this->cacheHead = 0;

   this->tail = 0;
   this->cacheTail = 0;
   this->writersCacheTail = 0;
}

size_t MpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus MpscQueue::write(const void* message, size_t offset, size_t lenght)
{
	size_t alignedLength = ALIGN(lenght, ALIGNMENT);
	size_t recordLength = alignedLength + RECORD_HEADER_LENGTH;

	if (recordLength > this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	size_t localTail = 0;
	size_t newTail = 0;
	do {
		newTail = this->writersCacheTail.load(std::memory_order_acquire);
		localTail = newTail;

		bool isOverridingNonReadData = (newTail + recordLength) - this->cacheHead >= this->capacity;
		if (isOverridingNonReadData)
		{
			RING_BUFFER_GET_VOLATILE(this->cacheHead, this->head);
			bool isStillOverridingNonReadData = (newTail + recordLength) - this->cacheHead >= this->capacity;
			if (isStillOverridingNonReadData)
			{
				return WriteStatus::QUEUE_FULL;
			}
		}

		size_t localTailPosition = GET_POSITION(newTail, this->capacity);
		bool isNeedForWrap = localTailPosition + recordLength >= this->capacity;
		if (isNeedForWrap)
		{
			//don't write padding header if there is not enought space and just wrap
			size_t remainingCapacity = this->capacity - localTailPosition;
			if (remainingCapacity >= RECORD_HEADER_LENGTH)
			{
				size_t paddingSize = this->capacity - localTailPosition - RECORD_HEADER_LENGTH;
				newTail = newTail + paddingSize + RECORD_HEADER_LENGTH;
			}
			else
			{
				newTail = newTail + remainingCapacity;
			}
		}

		newTail = newTail + recordLength;

	} while (!this->writersCacheTail.compare_exchange_weak(
		localTail,
		newTail,
		std::memory_order_release,
		std::memory_order_relaxed));

	size_t localTailPosition = GET_POSITION(localTail, this->capacity);
	bool isNeedForWrap = localTailPosition + recordLength >= this->capacity;
	if (isNeedForWrap)
	{
		//don't write padding header if there is not enought space and just wrap
		size_t remainingCapacity = this->capacity - localTailPosition;
		if (remainingCapacity >= RECORD_HEADER_LENGTH)
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
	WRITE_DATA_MSG(header, alignedLength)

	//store the message contents
	void* bufferOffset = (void*)(this->buffer + localTailPosition + RECORD_HEADER_LENGTH);
	std::memcpy(bufferOffset, (const void*)((uint8_t*)message + offset), lenght);

	localTail = localTail + recordLength;
	RING_BUFFER_PUT_ORDERED(this->tail, localTail);
	
	return WriteStatus::SUCCESSFUL;
}

size_t MpscQueue::read(MessageHandler* handler)
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

	size_t localHeadPosition = GET_POSITION(localHead, this->capacity);

	while (localHead != this->cacheTail)
	{
		size_t localHeadPosition = GET_POSITION(localHead, this->capacity);
		//check if the remaining capacity is less than a record header even to fit in
		size_t remainingCapacity = this->capacity - localHeadPosition;
		if (remainingCapacity <= RECORD_HEADER_LENGTH)
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
		handler->onMessage(msg, msgLength);

		#ifdef ZERO_OUT_READ_MEMORY
				std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
		#endif

		size_t alignedLength = ALIGN(msgLength, ALIGNMENT);
		localHead = localHead + RECORD_HEADER_LENGTH + alignedLength;
	}

	size_t readBytes = localHead - this->head;
	RING_BUFFER_PUT_ORDERED(this->head, localHead);
	return readBytes;
}

MpscQueue::~MpscQueue()
{
	delete[] this->buffer;
}
