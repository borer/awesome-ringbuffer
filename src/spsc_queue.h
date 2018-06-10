#include <cstddef>
#include <cstdint>

class MessageHandler
{
public:
	virtual void onMessage(const uint8_t* buffer, unsigned long lenght) = 0;
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
	unsigned long head;
	unsigned long tail;

public:
	SpscQueue(unsigned long size);
	WriteStatus write(const void* msg, unsigned long offset, unsigned long lenght);
	void read(MessageHandler* handler);
	int getCapacity();
	~SpscQueue();
	
	unsigned long getTail()
	{
		return tail;
	}

	unsigned long getHead()
	{
		return head;
	}
};
