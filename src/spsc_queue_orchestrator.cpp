#include <thread>
#include <cassert>

#include "spsc_queue_orchestrator.h"

#define synchronized(m) \
    for(std::unique_lock<std::recursive_mutex> lk(m); lk; lk.unlock())

SpscQueueOrchestrator::SpscQueueOrchestrator(
	size_t capacity, 
	std::shared_ptr<MessageHandler> handler,
	std::shared_ptr<QueueWaitStrategy> waitStrategy)
{
	assert(handler != nullptr);
	assert(waitStrategy != nullptr);

	this->queue = std::make_unique<SpscQueue>(capacity);
	this->handler = handler;
	this->waitStrategy = waitStrategy;

	this->shouldConsume = false;
	this->isConsumerStarted = false;
}

void SpscQueueOrchestrator::consumerTask()
{
	thread_local MessageHandler *messaageHandler = this->handler.get();
	thread_local QueueWaitStrategy *myWaitStrategy = this->waitStrategy.get();

	while (shouldConsume)
	{
		thread_local size_t readBytes = this->queue->read(messaageHandler);
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

void SpscQueueOrchestrator::startConsumer()
{
	synchronized(this->consumerMutex)
	{
		if (this->isConsumerStarted)
		{
			return;
		}

		this->isConsumerStarted = true;
		this->shouldConsume = true;
		
		std::thread consumerThread(&SpscQueueOrchestrator::consumerTask, this);
		consumerThread.detach();
	}
}

void SpscQueueOrchestrator::stopConsumer()
{
	this->shouldConsume = false;
}

WriteStatus SpscQueueOrchestrator::write(const void * message, size_t offset, size_t lenght)
{
	return queue->write(message, offset, lenght);
}

SpscQueueOrchestrator::~SpscQueueOrchestrator()
{
}
