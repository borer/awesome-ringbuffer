#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "spsc_queue.h"

typedef struct
{
	unsigned long long testValue;
} Message;

class TestMessageHandler : public MessageHandler
{
public:

	uint64_t msgSequence = 0;
	Message* lastMessage;

	void onMessage(const uint8_t* buffer, size_t length, uint64_t sequence) final
	{
		Message* message = (Message*)buffer;
		if (msgSequence + 1 == sequence)
		{
			lastMessage = message;
			msgSequence = sequence;
		}
		else
		{
			FAIL();
		}
	};

	virtual ~TestMessageHandler() { };
};

TEST_CASE("Should be able to write a single message", "[ringbuffer]") {
	
	Message msg;
	size_t capacity = 1024;
	SpscQueue ringbuffer(capacity);

	WriteStatus status = ringbuffer.write((const void*)&msg, 0, sizeof(Message));
	REQUIRE(status == WriteStatus::SUCCESSFUL);
}

TEST_CASE("Should be able to read a single message", "[ringbuffer]") {

	Message msg;
	msg.testValue = 123456;
	size_t messageSize = sizeof(Message);
	size_t capacity = 1024;
	SpscQueue ringbuffer(capacity);

	WriteStatus status = ringbuffer.write((const void*)&msg, 0, messageSize);
	REQUIRE(status == WriteStatus::SUCCESSFUL);

	TestMessageHandler handler;
	size_t readBytes = ringbuffer.read((MessageHandler*)&handler);
	REQUIRE(handler.lastMessage->testValue == msg.testValue);
	REQUIRE(handler.msgSequence == 1);
	REQUIRE(readBytes > 0);
}