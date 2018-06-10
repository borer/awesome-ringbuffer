#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "spsc_queue.h"
#include "test-ringbuffer.h"

#define PARK_NANOS 10

class TestMessageHandler : public MessageHandler
{
	int numMsgRead = 0;
public:
	void onMessage(const uint8_t* buffer, unsigned long length)
	{
		std::cout << ", size : " << length << " ,content: ";
		for (unsigned long i = 0; i < length; i++)
		{
			std::cout << (const char)buffer[i];
		}
		std::cout << std::endl;
		numMsgRead++;
	};

	int getNumMsgRead()
	{
		return numMsgRead;
	}

	virtual ~TestMessageHandler()
	{

	};
};

void consumerTask(SpscQueue* queue, int numberMessages)
{
	TestMessageHandler* handler = new TestMessageHandler();
	while (true)
	{
		std::cout << "Read msg on head: " << queue->getHead() << ", tail: " << queue->getTail();
		queue->read((MessageHandler*)handler);

		if (numberMessages == handler->getNumMsgRead())
		{
			break;
		}
	}
}

void publisherTask(SpscQueue* queue, int numberMessages)
{
	for (size_t i = 0; i < numberMessages; i++)
	{
		int numTries = 0;
		const char* msg = "test";
		size_t msgSize = sizeof(msg);
		std::cout << "Writing msg : " << msg << ", head: " << queue->getHead() << ", tail: " << queue->getTail() << std::endl;
		WriteStatus status = queue->write(msg, 0, msgSize);
		while (status != WriteStatus::SUCCESSFUL && numTries < 10)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(PARK_NANOS));
			status = queue->write(msg, 0, msgSize);
			numTries++;
		}

		if (status != WriteStatus::SUCCESSFUL)
		{
			std::cout << "Error writing msg : " << msg << ", head: " << queue->getHead() << ", tail : " << queue->getTail() << ", status: ";
			if(status == WriteStatus::MSG_TOO_BIG)
			{
				std::cout << "message too big" << std::endl;
			}
			else if (status == WriteStatus::INVALID_MSG)
			{
				std::cout << "invalid message" << std::endl;
			}
			else if (status == WriteStatus::QUEUE_FULL)
			{
				std::cout << "queue full" << std::endl;
			}
			else 
			{
				std::cout << "ERROR max number of tries exceeded" << std::endl;
			}
		}
	}

	//consumerTask(queue, numberMessages);
}

int main(int argc, char **argv)
{
	unsigned long capacity = 120;
	int numberMsg = 20;
	if (argc == 2)
	{
		capacity = atoi(argv[1]);
	}
	else if (argc == 3)
	{
		capacity = atoi(argv[1]);
		numberMsg = atoi(argv[2]);
	}
	
	std::cout << "Init" << std::endl;
	SpscQueue myRingBuffer(capacity);
	std::cout << "Created RingBuffer with size : " << myRingBuffer.getCapacity() << std::endl;

	std::thread publisherThread(publisherTask, &myRingBuffer, numberMsg);
	std::thread consumerThread(consumerTask, &myRingBuffer, numberMsg);

	publisherThread.join();
	consumerThread.join();

	std::cout << "Ending" << std::endl;
	return 0;
}
