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
	std::string* lastMsg;
	unsigned long lastMsgLength;
public:
	void onMessage(const uint8_t* buffer, unsigned long length)
	{
		lastMsgLength = length;
		lastMsg = new std::string((const char*)buffer, length);
		numMsgRead++;
	};

	const std::string* getLastMsg()
	{
		return lastMsg;
	}

	unsigned long getLastMsgSize()
	{
		return lastMsgLength;
	}

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
	unsigned long lastHead = 99999;
	unsigned long lastTail = 99999;
	while (true)
	{
		queue->read((MessageHandler*)handler);

		if (lastHead != queue->getHead() || lastTail != queue->getTail())
		{
			lastHead = queue->getHead();
			lastTail = queue->getTail();
			std::cout << "Read msg on head: " << queue->getHead() 
				<< ", tail: " << queue->getTail() 
				<< ", size: " << handler->getLastMsgSize() 
				<< ", msg: " << handler->getLastMsg()->c_str() << std::endl;
		}

		if (numberMessages == handler->getNumMsgRead())
		{
			break;
		}
	}
}

void publisherTask(SpscQueue* queue, int numberMessages)
{
	for (size_t i = 0; i < (size_t)numberMessages; i++)
	{
		int numTries = 0;
		const char* msg = "test";
		size_t msgSize = sizeof(msg);
		//std::cout << "Writing msg : " << msg << ", head: " << queue->getHead() << ", tail: " << queue->getTail() << std::endl;
		WriteStatus status = queue->write(msg, 0, msgSize);
		while (status != WriteStatus::SUCCESSFUL && numTries < 10)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(PARK_NANOS));
			status = queue->write(msg, 0, msgSize);
			numTries++;
		}

		/*if (status != WriteStatus::SUCCESSFUL)
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
		}*/
	}
}

int main(int argc, char **argv)
{
	unsigned long capacity = 20;
	int numberMsg = 4;
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

	consumerThread.join();
	publisherThread.join();

	std::cout << "Ending" << std::endl;
	return 0;
}
