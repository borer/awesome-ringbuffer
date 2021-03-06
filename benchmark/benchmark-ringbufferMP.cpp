#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "mpsc_queue.h"
#include "binutils.h"

typedef struct Message
{
	uint64_t sequence;
} Message;

class TestMessageHandler : public MessageHandler
{
	uint64_t msgSequence = 0;
	bool invalidState = false;

public:
	void onMessage(const uint8_t* buffer, size_t length) final
	{
		//nothing to do
	};

	uint64_t getMsgSequence()
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

void consumerTask(MpscQueue* queue)
{
	TestMessageHandler* handler = new TestMessageHandler();
	long numIterations = 1;
	uint64_t numMessage = 0;
	uint64_t numMessageOffset = 0;
	uint64_t messagesPerIteration = 268435455;
	size_t headerSize = 12;
	size_t msgSize = sizeof(Message);
	auto start = std::chrono::system_clock::now();
	while (true)
	{
		size_t readBytes = queue->read((MessageHandler*)handler);

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

		if (readBytes == 0)
		{
			std::this_thread::yield();
		}

		numMessage = numMessage + (readBytes / (msgSize + headerSize));
		if ((numMessage & messagesPerIteration) == 0)
		{
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

			start = end;
			double elapsedTime = elapsed_seconds.count();
			double messagesPerSecond = (double)messagesPerIteration / elapsedTime;
			char numPerSecond[50];
			sprintf(numPerSecond, "%F", messagesPerSecond);
			int messageBytes = ALIGN(msgSize, ALIGNMENT) + sizeof(RecordHeader);
			std::cout << "finished computation at " << std::ctime(&end_time)
				<< " elapsed time: " << elapsedTime << "s (270 millions)\n"
				<< " msg/s : " << numPerSecond << "\n"
				<< " MiB/s : " << (double)(messagesPerSecond * messageBytes) / 1000000
				<< std::endl;

			numMessageOffset = numIterations * messagesPerIteration;
			numIterations++;
			numMessage = 0;
			if (numIterations >= 10)
			{
				exit(0);
			}
		}
	}
}

void publisherTask(MpscQueue* queue)
{
	uint64_t numMessages = 0;
	size_t msgSize = sizeof(Message);
	Message* msg = new Message();
	
	while (true)
	{
		int numberTries = 0;
		numMessages = numMessages + 1;
		msg->sequence = numMessages;
		WriteStatus status = queue->write(msg, 0, msgSize);
		
		while (status != WriteStatus::SUCCESSFUL && numberTries < 1000)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(100));
			status = queue->write(msg, 0, msgSize);
			numberTries++;
		}

		if (status != WriteStatus::SUCCESSFUL)
		{
			std::cout << "Error writing msg, head: " << queue->getHead() << ", tail : " << queue->getTail() << ", status: ";
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

int main()
{
	size_t capacity = 1048576; //~1 MiB in bytes (2^20)

	std::cout << "Init" << std::endl;
	MpscQueue myRingBuffer(capacity, 0);
	std::cout << "Created RingBuffer with size : " << myRingBuffer.getCapacity() << std::endl;

	std::thread consumerThread(consumerTask, &myRingBuffer);
	std::thread publisherThread1(publisherTask, &myRingBuffer);
	std::thread publisherThread2(publisherTask, &myRingBuffer);

	publisherThread1.join();
	publisherThread2.join();
	consumerThread.join();

	std::cout << "Ending" << std::endl;
	return 0;
}
