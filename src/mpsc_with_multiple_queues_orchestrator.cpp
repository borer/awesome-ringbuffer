#include <thread>
#include <cassert>

#include "mpsc_with_multiple_queues_orchestrator.h"
#include "binutils.h"

#define synchronized(m) \
    for(std::unique_lock<std::recursive_mutex> lk(m); lk; lk.unlock())

MpscWithMultipleQueuesOrchestrator::MpscWithMultipleQueuesOrchestrator(
	std::shared_ptr<MessageHandler> handler,
	std::shared_ptr<QueueWaitStrategy> waitStrategy)
	: handler(handler), waitStrategy(waitStrategy)
{
	assert(handler != nullptr);
	assert(waitStrategy != nullptr);

	this->shouldConsume = false;
	this->isConsumerStarted = false;
}

size_t MpscWithMultipleQueuesOrchestrator::addPublisher(size_t capacity, size_t maxBatchRead)
{
	if (this->isConsumerStarted)
	{
		return -1;
	}

	this->queues.push_back(new SpscQueue(capacity, maxBatchRead));
	size_t index = this->queues.size() - 1;
	return index;
}

void MpscWithMultipleQueuesOrchestrator::consumerTask()
{
	MessageHandler *messageHandler = this->handler.get();
	QueueWaitStrategy *myWaitStrategy = this->waitStrategy.get();
	this->shouldConsume = true;
	size_t queueSize = this->queues.size();

	while (shouldConsume)
	{
		size_t readBytes = 0;
		for (size_t index = 0; index < queueSize; index++)
		{
			readBytes = readBytes + this->queues[index]->read(messageHandler);
		}

		if (readBytes == 0)
		{
			myWaitStrategy->wait();
		}
	}

	synchronized(this->consumerMutex)
	{
		this->shouldConsume = false;
		this->isConsumerStarted = false;
	}
}

void MpscWithMultipleQueuesOrchestrator::startConsumer()
{
	synchronized(this->consumerMutex)
	{
		if (this->isConsumerStarted)
		{
			return;
		}

		this->isConsumerStarted = true;
		std::thread consumerThread(&MpscWithMultipleQueuesOrchestrator::consumerTask, this);
		consumerThread.detach();
	}
}

void MpscWithMultipleQueuesOrchestrator::stopConsumer()
{
	this->shouldConsume = false;
}

WriteStatus MpscWithMultipleQueuesOrchestrator::write(size_t publisher, const void * message, size_t offset, size_t lenght)
{
	return this->queues[publisher]->write(message, offset, lenght);
}

MpscWithMultipleQueuesOrchestrator::~MpscWithMultipleQueuesOrchestrator()
{
	for (size_t i = 0; i < this->queues.size(); i++) {
		SpscQueue* queue = this->queues[i];
		delete queue;
	}
}
