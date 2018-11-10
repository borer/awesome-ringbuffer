#ifndef MPSC_QUEUE_ORCHESTRATOR_H
#define MPSC_QUEUE_ORCHESTRATOR_H

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

#include "queue.h"
#include "spsc_queue.h"
#include "queue_wait_strategy.h"

#pragma pack(push)
#pragma pack(4)
class MpscWithMultipleQueuesOrchestrator
{
protected:
	std::vector<SpscQueue*> queues;
	std::shared_ptr<MessageHandler> handler;
	std::shared_ptr<QueueWaitStrategy> waitStrategy;

	std::recursive_mutex consumerMutex;
	bool isConsumerStarted;
	bool shouldConsume;
	
	void consumerTask();
public:
	MpscWithMultipleQueuesOrchestrator(
		std::shared_ptr<MessageHandler> handler,
		std::shared_ptr<QueueWaitStrategy> waitStrategy);

	//needs to add all the publishers before starting the consumer
	size_t addPublisher(size_t capacity, size_t maxBatchRead);

	void startConsumer();
	void stopConsumer();

	WriteStatus write(size_t publisher, const void* message, size_t offset, size_t lenght);

	~MpscWithMultipleQueuesOrchestrator();
};

#pragma pack(pop)

#endif // MPSC_QUEUE_ORCHESTRATOR_H
