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

void publisherTask(SpscQueue* queue)
{
	long long numMessage = 0;
	size_t message_size = sizeof(Message) + sizeof(RecordHeader);
	size_t maxMessages = (queue->getCapacity() / message_size) - 1;
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
			std::cout << "Error writing message. Should never happen" << std::endl;
			std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			status = queue->write(msg, 0, msgSize);
			numberTries++;
		}

		if (numMessage == maxMessages)
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

			return;
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
			return;
		}
	}
}

int main(int argc, char **argv)
{
	size_t capacity = 1 << 30; //~1 GiB in bytes
	std::cout << capacity << std::endl;

	size_t message_size = ALIGN(sizeof(Message), ALIGNMENT) + sizeof(RecordHeader);

	std::cout << "Init. msg size: " << message_size << std::endl;

	for (size_t i = 0; i < 10; i++)
	{
		SpscQueue myRingBuffer(capacity);
		std::thread publisherThread(publisherTask, &myRingBuffer);
		publisherThread.join();
	}

	std::cout << "Ending" << std::endl;
	return 0;
}
