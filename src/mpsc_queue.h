#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <cstddef>
#include <cstdint>
#include <atomic>

#include "queue.h"

#pragma pack(push)
#pragma pack(4)
class MpscQueue
{
protected:
	uint8_t begin_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH)];
	uint8_t* buffer;
	const size_t capacity;
	uint8_t end_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH)];

	std::atomic<size_t> head;
	uint8_t head_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(std::atomic<size_t>)];
	size_t cacheTail; //used locally by read
	uint8_t cacheTail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];

	std::atomic<size_t> tail;
	uint8_t tail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(std::atomic<size_t>)];
	size_t cacheHead; //used locally by writes
	uint8_t cacheHead_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];
	std::atomic<size_t> writersCacheTail; //used locally by writes
	uint8_t writersCacheTail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(std::atomic<size_t>)];

public:
	MpscQueue(size_t capacity);
	WriteStatus write(const void* message, size_t offset, size_t lenght);
	size_t read(MessageHandler* handler);
	size_t getCapacity();
	~MpscQueue();
	
	size_t getTail()
	{
		return tail;
	}

	size_t getTailPosition()
	{
		return tail & (capacity - 1);
	}

	size_t getHead()
	{
		return head;
	}

	size_t getHeadPosition()
	{
		return head & (capacity - 1);
	}
};
#pragma pack(pop)

#endif // SPSC_QUEUE_H
