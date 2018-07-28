#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "spsc_queue.h"

typedef struct Message
{
	size_t sequence;
};

typedef struct
{
	size_t length;
	unsigned long long sequence;
	char type;
	char padding1, padding2;
	char end;

} RecordHeader;

#define ALIGNMENT (2 * sizeof(int32_t))
#define ALIGN(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

class TestMessageHandler : public MessageHandler
{
	unsigned long long msgSequence = 0;
	bool invalidState = false;

public:
	void onMessage(const uint8_t* buffer, size_t length, unsigned long long sequence)
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

	unsigned long long getMsgSequence()
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
			size_t numMessagesRead = queue->getCapacity() / message_size;
			double messagesPerSecond = (double)numMessagesRead / elapsedTime;
			char numPerSecond[50];
			sprintf(numPerSecond, "%F", messagesPerSecond);
			
			std::cout << "finished computation at " << std::ctime(&end_time)
				<< " elapsed time: " << elapsedTime << "s (100 millions)\n"
				<< " msg/s : " << numPerSecond << "\n"
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
	auto start = std::chrono::system_clock::now();
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

int main(int argc, char **argv)
{
	size_t capacity = 1 << 30;
	std::cout << "Init" << std::endl;

	for (size_t i = 0; i < 10; i++)
	{
		SpscQueue myRingBuffer(capacity);
		prepareBuffer(&myRingBuffer);
		consumerTask(&myRingBuffer);
	}

	std::cout << "Ending" << std::endl;
	return 0;
}
