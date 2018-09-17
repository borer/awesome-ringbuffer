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

void publisherTask(SpscQueue* queue)
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
		WriteStatus status = queue->write(msg, 0, msgSize);

		if (numMessage == maxMessages)
		{
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);

			start = end;
			double elapsedTime = elapsed_seconds.count();
			double messagesPerSecond = (double)numMessage / elapsedTime;
			char numPerSecond[50];
			sprintf(numPerSecond, "%F", messagesPerSecond);
			int messageBytes = ALIGN(msgSize, ALIGNMENT) + sizeof(RecordHeader);
			std::cout << "finished computation at " << std::ctime(&end_time)  
				<< " elapsed time: " << elapsedTime << "s (" << numMessage << ")\n"
				<< " msg/s : " << numPerSecond << "\n"
				<< " MiB/s : " << (double)(messagesPerSecond * messageBytes) / 1000000
				<< std::endl;

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
			return;
		}
	}
}

int main()
{
	size_t capacity = 4294967296; //~1 GiB in bytes
	size_t message_size = ALIGN(sizeof(Message), ALIGNMENT) + sizeof(RecordHeader);
	std::cout << "Init" 
		<< " buffer capacity: " << capacity
		<< " msg size: " << message_size << std::endl;

	for (size_t i = 0; i < 10; i++)
	{
		SpscQueue myRingBuffer(capacity, 0);
		publisherTask(&myRingBuffer);
	}

	std::cout << "Ending" << std::endl;
	return 0;
}
