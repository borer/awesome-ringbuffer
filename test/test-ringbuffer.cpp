#include <iostream>
#include <string>
#include "spsc_queue.h"

class TestMessageHandler : public MessageHandler
{
public:
	void onMessage(const uint8_t* buffer, size_t length)
	{
		std::cout << "Read msg of size : " << length << " ,content: ";
		for (size_t i = 0; i < length; i++)
		{
			std::cout << (const char)buffer[i];
		}
		std::cout << std::endl;
	};

	virtual ~TestMessageHandler()
	{

	};
};

void writeMsg(SpscQueue* queue, const char* msg)
{
	std::cout << "Writing msg : " << msg << std::endl;
	WriteStatus status = queue->write(msg, 0, sizeof(msg));
	if (status != WriteStatus::SUCCESSFUL)
	{
		std::cout << "Error writing msg : " << msg << " ,returned : " << status << std::endl;
	}
}

int main(int argc, char **argv)
{
	size_t capacity = 100;
	std::cout << "Init" << std::endl;
	SpscQueue myBuffer(capacity);
	MessageHandler* handler = new TestMessageHandler();

	std::cout << "RingBuffer size : " << myBuffer.getCapacity() << std::endl;

	const char* str = "test";
	writeMsg(&myBuffer, str);
	//const char* str2 = "moreTest";
	//writeMsg(&myBuffer, str2);

	myBuffer.read(handler);

	std::cout << "Ending" << std::endl;
	return 0;
}
