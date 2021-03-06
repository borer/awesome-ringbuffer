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
	bool shouldConsume;

	void consumerTask();

public:
	SpscQueueOrchestrator(
		size_t capacity, 
		size_t maxBatchRead, 
		std::shared_ptr<MessageHandler> handler, 
		std::shared_ptr<QueueWaitStrategy> waitStrategy);
	void startConsumer();
	void stopConsumer();
	WriteStatus write(const void* message, size_t offset, size_t lenght);

	~SpscQueueOrchestrator();
};

#endif // SPSC_QUEUE_ORCHESTRATOR_H
