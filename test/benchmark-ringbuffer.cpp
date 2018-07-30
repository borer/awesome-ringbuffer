#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "spsc_queue.h"

typedef struct Message
{
	long long sequence;
};

class TestMessageHandler : public MessageHandler
{
	uint64_t msgSequence = 0;
	bool invalidState = false;

public:
	void onMessage(const uint8_t* buffer, size_t length, uint64_t sequence) final
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
	TestMessageHandler* handler = new TestMessageHandler();
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

		/*if (readBytes == 0)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(2));
		}*/
	}
}

typedef struct
{
	size_t length;
	uint64_t sequence;
	int type;
} RecordHeader;

#define ALIGNMENT (2 * sizeof(int32_t))
#define ALIGN(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

void publisherTask(SpscQueue* queue)
{
	long numIterations = 0;
	long long numMessage = 0;
	size_t msgSize = sizeof(Message);
	Message* msg = new Message();
	auto start = std::chrono::system_clock::now();
	while (true)
	{
		numMessage++;
		msg->sequence = numMessage;
		WriteStatus status = queue->write(msg, 0, msgSize);
		int numberTries = 0;
		bool isTrying = false;
		while (status != WriteStatus::SUCCESSFUL && numberTries < 1000)
		{
			/*std::cout << "Is trying to write message : " << numMessage
				<< ", head: " << queue->getHead()
				<< ", headPosition: " << queue->getHeadPosition()
				<< ", tail: " << queue->getTail()
				<< ", tailPosition: " << queue->getTailPosition()
				<< ", error code " << status
				<< std::endl;*/

			std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			status = queue->write(msg, 0, msgSize);
			numberTries++;
		}

		if (numMessage > 100000000 && status == WriteStatus::SUCCESSFUL)
		{
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

			start = end;
			double elapsedTime = elapsed_seconds.count();
			double messagesPerSecond = (double)numMessage / elapsedTime;
			char numPerSecond[10];
			sprintf(numPerSecond, "%F", messagesPerSecond);
			int messageBytes = ALIGN(msgSize, ALIGNMENT) + sizeof(RecordHeader);
			std::cout << "finished computation at " << std::ctime(&end_time)  
				<< " elapsed time: " << elapsedTime << "s (100 millions)\n"
				<< " msg/s : " << numPerSecond << "\n"
				<< " MiB/s : " << (double)(messagesPerSecond * messageBytes) / 1000000
				<< std::endl;

			numIterations++;
			numMessage = 0;
			if (numIterations >= 10)
			{
				exit(0);
			}
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
	size_t capacity = 1048576; //~1 MiB in bytes (2^20)

	std::cout << "Init" << std::endl;
	SpscQueue myRingBuffer(capacity);
	std::cout << "Created RingBuffer with size : " << myRingBuffer.getCapacity() << std::endl;

	std::thread publisherThread(publisherTask, &myRingBuffer);
	std::thread consumerThread(consumerTask, &myRingBuffer);

	publisherThread.join();
	consumerThread.join();

	std::cout << "Ending" << std::endl;
	return 0;
}
