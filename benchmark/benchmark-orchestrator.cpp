#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "spsc_queue_orchestrator.h"
#include "binutils.h"

typedef struct Message
{
	uint64_t sequence;
} Message;

class TestMessageHandler : public MessageHandler
{
	uint64_t msgSequence = 0;

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
		}
	};

	virtual ~TestMessageHandler()
	{
	};
};

void publisherTask(SpscQueueOrchestrator* queue)
{
	uint64_t messagesPerIteration = 268435455;
	long numIterations = 0;
	uint64_t numMessage = 0;
	size_t msgSize = sizeof(Message);
	Message* msg = new Message();
	auto start = std::chrono::system_clock::now();
	while (true)
	{
		++numMessage;
		int numberTries = 0;
		msg->sequence = numMessage;

		WriteStatus status = queue->write(msg, 0, msgSize);
		
		while (status != WriteStatus::SUCCESSFUL && numberTries < 1000)
		{
			std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			status = queue->write(msg, 0, msgSize);
			numberTries++;
		}

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

			numIterations++;
			if (numIterations >= 10)
			{
				exit(0);
			}
		}

		if (status != WriteStatus::SUCCESSFUL)
		{
			std::cout << "Error writing msg : " << numMessage << ", status: ";
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
	std::shared_ptr<TestMessageHandler> handler = std::make_shared<TestMessageHandler>();
	std::shared_ptr<QueueWaitStrategy> waitStrategy = std::make_shared<YieldingStrategy>();
	SpscQueueOrchestrator myRingBuffer(capacity, 0, handler, waitStrategy);
	std::cout << "Created RingBuffer with size : " << capacity << std::endl;

	myRingBuffer.startConsumer();

	std::thread publisherThread(publisherTask, &myRingBuffer);
	publisherThread.join();

	std::cout << "Ending" << std::endl;
	return 0;
}
