#include <thread>

#include "spsc_queue_orchestrator.h"

#define synchronized(m) \
    for(std::unique_lock<std::recursive_mutex> lk(m); lk; lk.unlock())

SpscQueueOrchestrator::SpscQueueOrchestrator(size_t capacity, std::shared_ptr<MessageHandler> handler)
{
	this->queue = std::make_unique<SpscQueue>(capacity);
	this->handler = handler;

	this->shouldStopConsumer = false;
	this->isConsumerStarted = false;
}

void SpscQueueOrchestrator::consumerTask()
{
	while (!shouldStopConsumer)
	{
		size_t readBytes = this->queue->read(handler.get());
		if (readBytes == 0)
		{
			//TODO: add waiting strategies (busy spin, yielding, pausing)
			std::this_thread::yield();
		}
	}

	synchronized(this->consumerMutex)
	{
		this->shouldStopConsumer = false;
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
		this->shouldStopConsumer = false;
		
		std::thread consumerThread(&SpscQueueOrchestrator::consumerTask, this);
		consumerThread.detach();
	}
}

void SpscQueueOrchestrator::stopConsumer()
{
	this->shouldStopConsumer = true;
}

WriteStatus SpscQueueOrchestrator::write(const void * message, size_t offset, size_t lenght)
{
	return queue->write(message, offset, lenght);
}

WriteStatus SpscQueueOrchestrator::writeBatch(const void * message, size_t offset, size_t lenght, bool isEndOfBatch)
{
	//TODO: add producer batching
	return WriteStatus::INVALID_MSG;
}

SpscQueueOrchestrator::~SpscQueueOrchestrator()
{
}
