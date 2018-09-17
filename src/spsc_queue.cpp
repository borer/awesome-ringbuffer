#include <cstring>
#include <cassert>

#include "spsc_queue.h"
#include "binutils.h"

//Enable in case setting memory to 0 after read is wanted
//#define ZERO_OUT_READ_MEMORY

SpscQueue::SpscQueue(size_t capacity, size_t maxBatchRead) : 
	capacity(capacity), maxBatchRead(maxBatchRead)
{
   assert(IS_POWER_OF_TWO(capacity));

   this->buffer = new uint8_t[this->capacity];

   this->head = 0;
   this->cacheHead = 0;

   this->tail = 0;
   this->cacheTail = 0;
}

size_t SpscQueue::getCapacity()
{
	return this->capacity;
}

WriteStatus SpscQueue::write(const void* message, size_t offset, size_t lenght)
{
	size_t alignedLength = ALIGN(lenght, ALIGNMENT);
	size_t recordLength = alignedLength + RECORD_HEADER_LENGTH;
	size_t localTail = this->tail.load(std::memory_order_relaxed);

	if (recordLength > this->capacity)
	{
		return WriteStatus::MSG_TOO_BIG;
	}

	size_t localTailPosition = GET_POSITION(localTail, this->capacity);
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
	this->tail.store(localTail, std::memory_order_release);

	return WriteStatus::SUCCESSFUL;
}

size_t SpscQueue::read(MessageHandler* handler)
{
	size_t localHead = this->head.load(std::memory_order_relaxed);
	if (localHead == this->cacheTail)
	{
		this->cacheTail = this->tail.load(std::memory_order_acquire);
		if (localHead == this->cacheTail)
		{
			return 0;
		}
	}

	size_t messagesRead = 0;
	while (localHead != this->cacheTail && (messagesRead < this->maxBatchRead || this->maxBatchRead == 0))
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
		messagesRead++;
	}

	size_t readBytes = localHead - this->head;
	this->head.store(localHead, std::memory_order_release);
	return readBytes;
}

SpscQueue::~SpscQueue()
{
	delete[] this->buffer;
}
