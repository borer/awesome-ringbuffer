#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "spsc_queue.h"
#include "binutils.h"

typedef struct Message
{
	unsigned long long testValue;
} Message;

class TestSequenceMessageHandler : public MessageHandler
{
public:

	uint64_t msgSequence = 0;
	Message* lastMessage;

	void onMessage(const uint8_t* buffer, size_t length) final
	{
		Message* message = (Message*)buffer;
		lastMessage = message;
		msgSequence = msgSequence + 1;
	};

	Message* getLastMessage()
	{
		return lastMessage;
	}

	virtual ~TestSequenceMessageHandler() {};
};

unsigned int findNextPowerOf2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

class MutableSPSCQueue : public SpscQueue
{
public:
	MutableSPSCQueue(size_t capacity, size_t maxBatchRead) : SpscQueue(capacity, maxBatchRead) {};

	void setHead(size_t head)
	{
		this->head = head;
	};

	void setTail(size_t tail)
	{
		this->tail = tail;
	};

	void setCacheTail(size_t cacheTail)
	{
		this->cacheTail = cacheTail;
	};
};

TEST_CASE("Should be able to write a single message", "[ringbuffer]") 
{	
	Message msg;
	size_t capacity = 64;
	SpscQueue ringbuffer(capacity, 0);

	WriteStatus status = ringbuffer.write((const void*)&msg, 0, sizeof(Message));
	REQUIRE(status == WriteStatus::SUCCESSFUL);
}

TEST_CASE("Should be able to write a padding message before wrapping", "[ringbuffer]")
{
	Message msg;
	size_t messageSize = sizeof(Message);
	size_t totalMessageSize = ALIGN(messageSize, ALIGNMENT) + RECORD_HEADER_LENGTH;
	size_t capacity = 128;
	unsigned long long expectedValue = 456;
	
	MutableSPSCQueue ringbuffer(capacity, 0);
	ringbuffer.setHead(capacity - RECORD_HEADER_LENGTH);
	ringbuffer.setTail(capacity - RECORD_HEADER_LENGTH);
	ringbuffer.setCacheTail(capacity + totalMessageSize);

	TestSequenceMessageHandler handler;

	msg.testValue = expectedValue;
	WriteStatus status = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status == WriteStatus::SUCCESSFUL);
	REQUIRE(ringbuffer.getTail() == capacity + totalMessageSize);

	size_t readBytes = ringbuffer.read((MessageHandler*)&handler);
	REQUIRE(readBytes > 0);
	REQUIRE(handler.getLastMessage()->testValue == expectedValue);
}

TEST_CASE("Should skip writing a padding message before wrapping if not enought space", "[ringbuffer]")
{
	Message msg;
	size_t messageSize = sizeof(Message);
	size_t totalMessageSize = ALIGN(messageSize, ALIGNMENT) + RECORD_HEADER_LENGTH;
	size_t capacity = 128;
	MutableSPSCQueue ringbuffer(capacity, 0);
	ringbuffer.setHead(capacity - 2);
	ringbuffer.setTail(capacity - 2);
	ringbuffer.setCacheTail(capacity + totalMessageSize);
	unsigned long long expectedValue = 123;

	msg.testValue = expectedValue;
	WriteStatus status = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status == WriteStatus::SUCCESSFUL);
	REQUIRE(ringbuffer.getTail() == capacity + totalMessageSize);

	TestSequenceMessageHandler handler;

	size_t readBytes = ringbuffer.read((MessageHandler*)&handler);
	REQUIRE(readBytes > 0);
	REQUIRE(handler.getLastMessage()->testValue == expectedValue);
}

TEST_CASE("Should read nothing from empty buffer", "[ringbuffer]")
{
	SpscQueue ringbuffer(64, 0);
	TestSequenceMessageHandler handler;

	size_t readBytes = ringbuffer.read((MessageHandler*)&handler);
	REQUIRE(readBytes == 0);
}

TEST_CASE("Should be able to read a single message", "[ringbuffer]") 
{
	Message msg;
	msg.testValue = 123456;
	size_t messageSize = sizeof(Message);
	size_t capacity = 64;
	SpscQueue ringbuffer(capacity, 0);

	WriteStatus status = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status == WriteStatus::SUCCESSFUL);

	TestSequenceMessageHandler handler;
	size_t readBytes = ringbuffer.read((MessageHandler*)&handler);
	REQUIRE(handler.lastMessage->testValue == msg.testValue);
	REQUIRE(handler.msgSequence == 1);
	REQUIRE(readBytes > 0);
}

TEST_CASE("Should read only up until max batch read in a single read command", "[ringbuffer]")
{
	Message msg;
	
	size_t messageSize = sizeof(Message);
	size_t capacity = 64 + (messageSize * 4);
	size_t maxBatchRead = 2;
	SpscQueue ringbuffer(capacity, maxBatchRead);

	msg.testValue = 123;
	WriteStatus status1 = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status1 == WriteStatus::SUCCESSFUL);
	msg.testValue = 456;
	WriteStatus status2 = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status2 == WriteStatus::SUCCESSFUL);
	msg.testValue = 789;
	WriteStatus status3 = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status3 == WriteStatus::SUCCESSFUL);

	TestSequenceMessageHandler handler;
	size_t readBytes = ringbuffer.read((MessageHandler*)&handler);
	REQUIRE(handler.lastMessage->testValue == 456);
	REQUIRE(handler.msgSequence == 2);
	REQUIRE(readBytes > 0);
}

TEST_CASE("Should reject writing message bigger that capacity", "[ringbuffer]") 
{
	uint8_t msg[100];
	size_t messageSize = sizeof(msg);
	size_t capacity = 64;
	SpscQueue ringbuffer(capacity, 0);

	WriteStatus status = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status == WriteStatus::MSG_TOO_BIG);
}

TEST_CASE("Should reject write when buffer full", "[ringbuffer]") 
{
	uint8_t msg[32];
	size_t messageSize = sizeof(msg);
	size_t totalMessageSize = ALIGN(messageSize, ALIGNMENT) + RECORD_HEADER_LENGTH;
	size_t capacity = findNextPowerOf2(totalMessageSize);
	SpscQueue ringbuffer(capacity, 0);

	for (size_t i = 0; i + totalMessageSize < capacity; i = i + totalMessageSize)
	{
		REQUIRE(ringbuffer.write((const void*)&msg, 0, messageSize) == WriteStatus::SUCCESSFUL);
	}

	REQUIRE(ringbuffer.write((const void*)&msg, 0, messageSize) == WriteStatus::QUEUE_FULL);
}
