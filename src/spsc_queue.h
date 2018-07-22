#include <cstddef>
#include <cstdint>
#include <atomic>

#define RING_BUFFER_CACHE_LINE_LENGTH (64)

class MessageHandler
{
public:
	virtual void onMessage(const uint8_t* buffer, unsigned long lenght, unsigned long messageSequence) = 0;
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
	unsigned long capacity;
	unsigned int batchSize;
	uint8_t end_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH)];

	std::atomic<unsigned long> head;
	uint8_t head_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(std::atomic<unsigned long>)];
	unsigned long cacheHead; //used locally by writes
	uint8_t cacheHead_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(unsigned long)];
	unsigned long privateCacheHead; //used locally by read
	uint8_t privateCacheHead_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(unsigned long)];

	std::atomic<unsigned long> tail;
	uint8_t tail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(std::atomic<unsigned long>)];
	unsigned long cacheTail; //used locally by read
	uint8_t cacheTail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(unsigned long)];
	unsigned long privateCacheTail; //used locally by write
	uint8_t privateCacheTail_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(unsigned long)];
	unsigned long messageSequence;
	uint8_t messageSequence_pad[(2 * RING_BUFFER_CACHE_LINE_LENGTH) - sizeof(unsigned long)];

public:
	SpscQueue(unsigned long capacity, unsigned int batchSize = 10);
	WriteStatus write(const void* msg, unsigned long offset, unsigned long lenght);
	unsigned long read(MessageHandler* handler);
	int getCapacity();
	~SpscQueue();
	
	unsigned long getTail()
	{
		return tail;
	}

	unsigned long getTailPosition()
	{
		return tail & (capacity - 1);
	}

	unsigned long getHead()
	{
		return head;
	}

	unsigned long getHeadPosition()
	{
		return head & (capacity - 1);
	}
};
