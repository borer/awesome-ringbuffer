#ifndef SPSC_QUEUE_ORCHESTRATOR_H
#define SPSC_QUEUE_ORCHESTRATOR_H

#include <mutex>
#include "spsc_queue.h"
#include "queue_wait_strategy.h"

class SpscQueueOrchestrator
{
	std::unique_ptr<SpscQueue> queue;
	std::shared_ptr<MessageHandler> handler;
	std::shared_ptr<QueueWaitStrategy> waitStrategy;

	std::recursive_mutex consumerMutex;
	bool isConsumerStarted;
	bool shouldStopConsumer;

	void consumerTask();

public:
	SpscQueueOrchestrator(
		size_t capacity, 
		std::shared_ptr<MessageHandler> handler, 
		std::shared_ptr<QueueWaitStrategy> waitStrategy);
	void startConsumer();
	void stopConsumer();
	WriteStatus write(const void* message, size_t offset, size_t lenght);
	WriteStatus writeBatch(const void* message, size_t offset, size_t lenght, bool isEndOfBatch);

	~SpscQueueOrchestrator();
};

#endif // SPSC_QUEUE_ORCHESTRATOR_H
