#include <thread>

#include "queue_wait_strategy.h"

void YieldingStrategy::wait()
{
	std::this_thread::yield();
}

SleepStrategy::SleepStrategy(unsigned int nanosToWait)
{
	this->nanosToWait = nanosToWait;
}

void SleepStrategy::wait()
{
	std::this_thread::sleep_for(std::chrono::nanoseconds(this->nanosToWait));
}

