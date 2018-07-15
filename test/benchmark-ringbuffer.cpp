#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "spsc_queue.h"

typedef struct Message
{
	long sequence;
	const char* name;
};

class TestMessageHandler : public MessageHandler
{
	unsigned long msgSequence = 0;
	bool invalidState = false;

public:
	void onMessage(const uint8_t* buffer, unsigned long length, unsigned long sequence)
	{
		Message* message = (Message*)buffer;
		if (msgSequence + 1 == sequence)
		{
			msgSequence = sequence;
		}
		else
		{
			std::cout << "Expected " << msgSequence + 1 << " got from ringbuffer " << sequence << std::endl;
			invalidState = true;
		}
	};

	unsigned long getMsgSequence()
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
	auto start = std::chrono::system_clock::now();
	while (true)
	{
		queue->read((MessageHandler*)handler);

		if (handler->isInvalidState())
		{
			std::cout << "Last read msg : " << handler->getMsgSequence() << " (size " << 12 + sizeof(Message) << " ) "
				<< ", head: " << queue->getHead()
				<< ", headPosition: " << queue->getHeadPosition()
				<< ", tail: " << queue->getTail()
				<< ", tailPosition: " << queue->getTailPosition()
				<< std::endl;
			break;
		}

		if (handler->getMsgSequence() > 100000000)
		{
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

			std::cout << "finished computation at " << std::ctime(&end_time)
				<< "elapsed time: " << elapsed_seconds.count() << "s\n" << std::endl;

			exit(0);
		}
	}
}

void publisherTask(SpscQueue* queue)
{
	long numMessage = 0;
	size_t msgSize = sizeof(Message);
	Message* msg = new Message();
	while (true)
	{
		numMessage++;
		msg->name = "fixed name";
		msg->sequence = numMessage;
		WriteStatus status = queue->write(msg, 0, msgSize);
		int numberTries = 0;
		bool isTrying = false;
		while (status != WriteStatus::SUCCESSFUL)
		{
			if (!isTrying && numberTries == 2)
			{
				std::cout << "Is trying to write message : " << numMessage
					<< ", head: " << queue->getHead()
					<< ", headPosition: " << queue->getHeadPosition()
					<< ", tail: " << queue->getTail()
					<< ", tailPosition: " << queue->getTailPosition()
					<< ", error code " << status
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
