#ifndef QUEUE_H
#define QUEUE_H

#include <cstddef>
#include <cstdint>

#define RING_BUFFER_CACHE_LINE_LENGTH (64)

#define MSG_DATA_TYPE 0x01
#define MSG_PADDING_TYPE 0x02
#define MSG_HEADER_ENDING 0x66

#define WRITE_DATA_MSG(header, lengthMsg, sequenceMsg) \
	header->length = lengthMsg; \
	header->type = MSG_DATA_TYPE;

#define WRITE_PADDING_MSG(header, lengthMsg) \
	header->length = lengthMsg; \
	header->type = MSG_PADDING_TYPE;

#define RECORD_HEADER_LENGTH sizeof(RecordHeader)

#define GET_POSITION(value, capacity) value & (capacity - 1)

typedef struct
{
	size_t length;
	int type;

} RecordHeader;

enum WriteStatus
{
	INVALID_MSG = 1,
	MSG_TOO_BIG = 2,
	QUEUE_FULL = 3,
	SUCCESSFUL = 0
};

class MessageHandler
{
public:
	virtual void onMessage(const uint8_t* buffer, size_t lenght) = 0;
	virtual ~MessageHandler() {}
};

#endif // QUEUE_H
