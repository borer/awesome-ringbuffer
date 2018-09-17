#include <cstring>
#include <cassert>

#include "mpsc_queue.h"
#include "queue_atomic_64.h"
#include "binutils.h"

//#define ZERO_OUT_READ_MEMORY

MpscQueue::MpscQueue(size_t capacity, size_t maxBatchRead) : 
	capacity(capacity), maxBatchRead(maxBatchRead)
{
   assert(IS_POWER_OF_TWO(capacity));
   this->buffer = new uint8_t[this->capacity];
   this->head = 0;
   this->tail = 0;
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
	size_t futureTail = 0;
	do {
		futureTail = this->tail.load(std::memory_order_acquire);
		localTail = futureTail;
		
		size_t cacheHead2 = this->head.load(std::memory_order_acquire);
		bool isOverridingNonReadData = (futureTail + recordLength) - cacheHead2 >= this->capacity;
		if (isOverridingNonReadData)
		{
			return WriteStatus::QUEUE_FULL;
		}

		size_t localTailPosition = GET_POSITION(futureTail, this->capacity);
		bool isNeedForWrap = localTailPosition + recordLength >= this->capacity;
		if (isNeedForWrap)
		{
			//don't write padding header if there is not enought space and just wrap
			size_t remainingCapacity = this->capacity - localTailPosition;
			if (remainingCapacity >= RECORD_HEADER_LENGTH)
			{
				size_t paddingSize = this->capacity - localTailPosition - RECORD_HEADER_LENGTH;
				futureTail = futureTail + paddingSize + RECORD_HEADER_LENGTH;
			}
			else
			{
				futureTail = futureTail + remainingCapacity;
			}
		}

		futureTail = futureTail + recordLength;

	} while (!this->tail.compare_exchange_weak(
		localTail,
		futureTail,
		std::memory_order_release,
		std::memory_order_relaxed));

	size_t bytesToWrite = 0;
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
			bytesToWrite = bytesToWrite + paddingSize + RECORD_HEADER_LENGTH;
		}
		else
		{
			bytesToWrite = bytesToWrite + remainingCapacity;
		}

		localTailPosition = GET_POSITION(localTail + bytesToWrite, this->capacity);
	}

	//store the message header
	RecordHeader* header = (RecordHeader*)(this->buffer + localTailPosition);
	WRITE_DATA_MSG(header, alignedLength)

	//store the message contents
	void* bufferOffset = (void*)(this->buffer + localTailPosition + RECORD_HEADER_LENGTH);
	std::memcpy(bufferOffset, (const void*)((uint8_t*)message + offset), lenght);
	
	return WriteStatus::SUCCESSFUL;
}

size_t MpscQueue::read(MessageHandler* handler)
{
	size_t localHead = this->head.load(std::memory_order_relaxed);

	size_t messagesRead = 0;
	while (messagesRead < this->maxBatchRead || this->maxBatchRead == 0)
	{
		size_t localHeadPosition = GET_POSITION(localHead, this->capacity);
		//check if the remaining capacity is less than a record header even to fit in
		size_t remainingCapacity = this->capacity - localHeadPosition;
		if (remainingCapacity <= RECORD_HEADER_LENGTH)
		{
			std::memset((void*)&this->buffer[localHeadPosition], 0, remainingCapacity);
			localHead = localHead + remainingCapacity;
			localHeadPosition = GET_POSITION(localHead, this->capacity);
		}

		RecordHeader* header = (RecordHeader*)(this->buffer + localHeadPosition);
		size_t msgLength = header->length;
		if (msgLength <= 0)
		{
			break;
		}

		if (header->type == MSG_PADDING_TYPE)
		{
			std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
			localHead = localHead + RECORD_HEADER_LENGTH + msgLength;
			localHeadPosition = GET_POSITION(localHead, this->capacity);
			header = (RecordHeader*)(this->buffer + localHeadPosition);
			msgLength = header->length;
			if (msgLength <= 0)
			{
				break;
			}
		}

		uint8_t* msg = (uint8_t*)(this->buffer + localHeadPosition + RECORD_HEADER_LENGTH);
		handler->onMessage(msg, msgLength);

		std::memset((void*)&this->buffer[localHeadPosition], 0, RECORD_HEADER_LENGTH + msgLength);
		size_t alignedLength = ALIGN(msgLength, ALIGNMENT);
		localHead = localHead + RECORD_HEADER_LENGTH + alignedLength;
		messagesRead++;
	}

	size_t readBytes = localHead - this->head.load(std::memory_order_relaxed);
	this->head.store(localHead, std::memory_order_release);
	return readBytes;
}

MpscQueue::~MpscQueue()
{
	delete[] this->buffer;
}
