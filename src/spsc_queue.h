#include <cstddef>
#include <cstdint>
#include <atomic>

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
	uint8_t* buffer;
	unsigned long capacity;
	std::atomic<unsigned long> head;
	std::atomic<unsigned long> tail;
	unsigned long messageSequence;
	size_t recordHeaderLength;

public:
	SpscQueue(unsigned long capacity);
	WriteStatus write(const void* msg, unsigned long offset, unsigned long lenght);
	void read(MessageHandler* handler);
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
