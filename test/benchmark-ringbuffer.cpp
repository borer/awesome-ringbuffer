#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "spsc_queue.h"

typedef struct Message
{
	long sequence;
	const char* name;

	Message(long sequence, const char* name)
	{
		this->sequence = sequence;
		this->name = name;
	}
};

class TestMessageHandler : public MessageHandler
{
	unsigned long msgSequence = 0;
	bool invalidState = false;

public:
	void onMessage(const uint8_t* buffer, unsigned long length, unsigned long sequence)
	{
		Message* message = (Message*)buffer;
		if (msgSequence + 1 != message->sequence)
		{
			std::cout << "Expected " << msgSequence + 1 << " got  from msg " << message->sequence << std::endl;
			if (msgSequence + 1 == sequence)
			{
				msgSequence = sequence;
			}
			else
			{
				std::cout << "Expected " << msgSequence + 1 << " got from ringbuffer " << sequence << std::endl;
				invalidState = true;
			}
		}
		else
		{
			msgSequence = message->sequence;
		}
	};

	long getMsgSequence()
	{
		return msgSequence;
	}

	bool isInvalidState()
	{
		return invalidState;
	}

	virtual ~TestMessageHandler()
	{

	};
};

void consumerTask(SpscQueue* queue)
{
	TestMessageHandler* handler = new TestMessageHandler();
	while (true)
	{
		queue->read((MessageHandler*)handler);

		if (handler->isInvalidState())
		{
			std::cout << "Writing msg : " << handler->getMsgSequence() << " (size " << 12 + sizeof(Message) << " ) "
				<< ", head: " << queue->getHead()
				<< ", tail: " << queue->getTail()
				<< ", readSize: " << queue->getTotalReadSize()
				<< ", storeSize: " << queue->getTotalStoreSize()
				<< std::endl;
			break;
		}
	}
}

void publisherTask(SpscQueue* queue)
{
	long numMessage = 0;
	size_t msgSize = sizeof(Message);
	while (true)
	{
		numMessage++;
		const Message* msg = new Message(numMessage, "fixed name");
		WriteStatus status = queue->write(msg, 0, msgSize);
		int numberTries = 0;
		bool isTrying = false;
		while (status != WriteStatus::SUCCESSFUL)
		{
			if (!isTrying && numberTries == 100)
			{
				std::cout << "Is trying to write message : " << numMessage
					<< ", head: " << queue->getHead()
					<< ", tail: " << queue->getTail()
					<< ", readSize: " << queue->getTotalReadSize()
					<< ", storeSize: " << queue->getTotalStoreSize()
					<< std::endl;
				isTrying = true;
			}

			std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			status = queue->write(msg, 0, msgSize);
			numberTries++;
		}

		if (status != WriteStatus::SUCCESSFUL)
		{
			std::cout << "Error writing msg : " << numMessage << ", head: " << queue->getHead() << ", tail : " << queue->getTail() << ", status: ";
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
			break;
		}

		/*if (numMessage % 1000 == 0)
		{
			std::cout << numMessage << std::endl;
		}*/
	}
}

int main(int argc, char **argv)
{
	unsigned long capacity = 1024000; //1 MiB in bytes

	std::cout << "Init" << std::endl;
	SpscQueue myRingBuffer(capacity);
	std::cout << "Created RingBuffer with size : " << myRingBuffer.getCapacity() << std::endl;

	std::thread publisherThread(publisherTask, &myRingBuffer);
	std::thread consumerThread(consumerTask, &myRingBuffer);

	consumerThread.join();
	publisherThread.join();

	std::cout << "Ending" << std::endl;
	return 0;
}
