#include <cstddef>
#include <cstdint>

#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02

#define PARK_NANOS 10

class MessageHandler
{
public:
	virtual void onMessage(const uint8_t* buffer, size_t lenght) = 0;
	virtual ~MessageHandler() {}
};

class record_header
{
	size_t msgLength;
	char msgType;
public:
	void setLength(size_t lenght)
	{
		this->msgLength = lenght;
	}

	size_t getLenght()
	{
		return this->msgLength;
	}

	void setType(char type)
	{
		this->msgType = type;
	}

	char getType()
	{
		return this->msgType;
	}
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
	size_t capacity;
	size_t head;
	size_t tail;

public:
	SpscQueue(size_t size);
	WriteStatus write(const void* msg, size_t offset, size_t lenght);
	void read(MessageHandler* handler);
	int getCapacity();
	~SpscQueue();
	
};
