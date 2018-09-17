#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "spsc_queue.h"
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
		Message* message = (Message*)buffer;
		if (msgSequence + 1 == message->sequence)
		{
			msgSequence = message->sequence;
		}
		else
		{
			std::cout << "Expected " << msgSequence + 1 << " got from message " << message->sequence << std::endl;
			invalidState = true;
		}
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

void consumerTask(SpscQueue* queue)
{
	size_t message_size = ALIGN(sizeof(Message), ALIGNMENT) + sizeof(RecordHeader);
	TestMessageHandler* handler = new TestMessageHandler();
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
			return;
		}

		if (readBytes == 0)
		{
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

			start = end;
			double elapsedTime = elapsed_seconds.count();
			double messagesPerSecond = (double)handler->getMsgSequence() / elapsedTime;
			char messagesPerSecondStr[50];
			sprintf(messagesPerSecondStr, "%F", messagesPerSecond);

			size_t maxMessages = (queue->getCapacity() / message_size) - 1;
			
			std::cout << "finished computation at " << std::ctime(&end_time)
				<< " elapsed time: " << elapsedTime << "s (" << maxMessages << ")\n"
				<< " msg/s : " << messagesPerSecondStr << "\n"
				<< " MiB/s : " << (double)(messagesPerSecond * message_size) / 1000000
				<< std::endl;

			return;
		}
	}
}

void prepareBuffer(SpscQueue* queue)
{
	size_t message_size = ALIGN(sizeof(Message), ALIGNMENT) + sizeof(RecordHeader);
	size_t maxMessages = (queue->getCapacity() / message_size) - 1;
	size_t msgSize = sizeof(Message);
	Message* msg = new Message();
	size_t numMessage = 0;
	while(true)
	{
		numMessage++;
		msg->sequence = numMessage;
		WriteStatus status = queue->write(msg, 0, msgSize);

		if (numMessage == maxMessages)
		{
			return;
		}

		if (status != WriteStatus::SUCCESSFUL)
		{
			std::cout << "Error writing message. Should never happen" << std::endl;

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

int main()
{
	size_t capacity = 1 << 30;
	std::cout << "Init" << std::endl;

	for (size_t i = 0; i < 10; i++)
	{
		SpscQueue myRingBuffer(capacity, 0);
		prepareBuffer(&myRingBuffer);
		consumerTask(&myRingBuffer);
	}

	std::cout << "Ending" << std::endl;
	return 0;
}
