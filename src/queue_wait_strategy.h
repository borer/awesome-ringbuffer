#ifndef QUEUE_WAIT_STRATEGY_H
#define QUEUE_WAIT_STRATEGY_H

class QueueWaitStrategy
{
public:
	virtual void wait() = 0;
	virtual ~QueueWaitStrategy() {}
};

class YieldingStrategy : public QueueWaitStrategy
{
public:
	void wait() final;
	~YieldingStrategy() {};
};

class SleepStrategy : public QueueWaitStrategy
{
	unsigned int nanosToWait;
public:
	SleepStrategy(unsigned int nanosToWait);
	void wait() final;
	~SleepStrategy() {};
};

class BusySpinStrategy : public QueueWaitStrategy
{
public:
	void wait() final {};
	~BusySpinStrategy() {};
};

#endif // QUEUE_WAIT_STRATEGY_H
