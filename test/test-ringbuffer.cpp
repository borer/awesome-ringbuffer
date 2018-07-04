#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "spsc_queue.h"
#include "test-ringbuffer.h"

#define PARK_NANOS 10

class TestMessageHandler : public MessageHandler
{
	long numMsgRead = 0;
	std::string* lastMsg;
	unsigned long lastMsgLength;
	unsigned long lastSequence = 0;

public:
	void onMessage(const uint8_t* buffer, unsigned long length, unsigned long sequence)
	{
		lastMsgLength = length;
		lastMsg = new std::string((const char*)buffer, length);
		numMsgRead++;

		if (lastSequence + 1 != sequence)
		{
			std::cout << "Expected " << lastSequence + 1 << " got " << sequence << std::endl;
		}

		lastSequence = sequence;
	};

	const std::string* getLastMsg()
	{
		return lastMsg;
	}

	unsigned long getLastMsgSize()
	{
		return lastMsgLength;
	}

	long getNumMsgRead()
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

		/*if (lastHead != queue->getHead() || lastTail != queue->getTail())
		{
			lastHead = queue->getHead();
			lastTail = queue->getTail();
			std::cout << "Read msg on head: " << queue->getHead()
				<< ", tail: " << queue->getTail()
				<< ", size: " << handler->getLastMsgSize()
				<< ", msg: " << handler->getLastMsg()->c_str() << std::endl;
		}*/

		/*if (handler->getNumMsgRead() % 100000 == 0 || handler->getNumMsgRead() > 9900000)
		{
			std::cout << handler->getNumMsgRead() << std::endl;
		}*/

		if (numberMessages == handler->getNumMsgRead())
		{
			break;
		}
	}
}

void publisherTask(SpscQueue* queue, long numberMessages)
{
	const char* msg = "test";
	size_t msgSize = sizeof(msg);
	for (long i = 0; i <= numberMessages; i++)
	{
		WriteStatus status = queue->write(msg, 0, msgSize);
		bool isTrying = false;
		int numTries = 0;
		while (status != WriteStatus::SUCCESSFUL)
		{
			if (!isTrying && numTries == 100)
			{
				std::cout << "Is trying to write message " << i <<  std::endl;
				std::cout << "Writing msg : " << msg 
					<< ", head: " << queue->getHead() 
					<< ", tail: " << queue->getTail() 
					<< ", readSize: " << queue->getTotalReadSize() 
					<< ", storeSize: " << queue->getTotalStoreSize() 
					<< std::endl;
				isTrying = true;
			}

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

		if (i % 100000 == 0)
		{
			std::cout << i << std::endl;
		}
	}
}

int main(int argc, char **argv)
{
	unsigned long capacity = 1024000;
	long numberMsg = 10000000;
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
