#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <cstddef>
#include <cstdint>

#define RING_BUFFER_CACHE_LINE_LENGTH (64)

class MessageHandler
{
public:
	virtual void onMessage(const uint8_t* buffer, size_t lenght, unsigned long long messageSequence) = 0;
	virtual ~MessageHandler() {}
};

enum WriteStatus
{
	INVALID_MSG = 1,
	MSG_TOO_BIG = 2,
	QUEUE_FULL = 3,
	SUCCESSFUL = 0
};

class SpscQueue
{
	uint8_t begin_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH)];
	uint8_t* buffer;
	const size_t capacity;
	uint8_t end_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH)];

	size_t head;
	uint8_t head_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];
	size_t cacheHead; //used locally by writes
	uint8_t cacheHead_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];
	size_t privateCacheHead; //used locally by read
	uint8_t privateCacheHead_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];

	size_t tail;
	uint8_t tail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];
	size_t cacheTail; //used locally by read
	uint8_t cacheTail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];
	size_t privateCacheTail; //used locally by write
	uint8_t privateCacheTail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];
	size_t messageSequence;
	uint8_t messageSequence_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(size_t)];

public:
	SpscQueue(size_t capacity);
	WriteStatus write(const void* msg, size_t offset, size_t lenght);
	size_t read(MessageHandler* handler);
	size_t getCapacity();
	~SpscQueue();
	
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

#endif // SPSC_QUEUE_H
