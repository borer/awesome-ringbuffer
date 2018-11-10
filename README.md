# Awesome RingBuffer


[![Build Status](https://api.travis-ci.org/borer/awesome-ringbuffer.svg?branch=master)](https://travis-ci.org/borer/awesome-ringbuffer)

Very fast implementation of a circular queue algorithm (aka ring buffer).
It has various variants :
- Single Publisher -> Single Consumer
- Multiple Publishers -> Single Consumer


# Usage

To compile and run the test :
 
```sh
mkdir build
cd build
cmake ..
make
make test
```

There are two main methods to use the library. 
 1) By orchestrators, that take care of starting the consuming thread.
 2) Using the raw wing buffers, that allow you to construct your own threads.

__Orchestrator__
```c++
//size of the ring buffer... the bigger, the better :)
size_t capacity = 1048576; //~1 MiB in bytes (2^20)

//create handler for the received messages
std::shared_ptr<TestMessageHandler> handler = std::make_shared<TestMessageHandler>();
//waiting strategy when there is no message to process
std::shared_ptr<QueueWaitStrategy> waitStrategy = std::make_shared<YieldingStrategy>();

//create the orchestrator 
SpscQueueOrchestrator queue(capacity, 0, handler, waitStrategy);

//starts a new thread that consumes newlly arrived messages
queue.startConsumer();

//push a singe message to the queue. In real world, this method shoud be called from a different thread.
Message* msg = new Message();
WriteStatus status = queue->write(msg, 0, sizeof(Message));

queue.stopConsumer();
```

__Raw Ring Buffer__

```c++
size_t capacity = 1048576; //~1 MiB in bytes (2^20)
SpscQueue queue(capacity, 0);

//push a singe message to the queue. In real world, this method shoud be called from a different thread.
Message* msg = new Message();
WriteStatus status = queue->write(msg, 0, msgSize);

//read the previously pushed message. In real world, this method also shoud be called from a different thread.
TestMessageHandler* handler = new TestMessageHandler();
size_t readBytes = queue->read((MessageHandler*)handler);
```

# Benchmarks

The source code for the benchmarks is included. Here are the result that I got from a untuned Windows laptop, 8GB of Ram, i5 8th Gen CPU.

Single Producer Single Consumer RingBuffer with size : 1048576 bytes (~ 1Mib)
 - elapsed time: 0.822732s (200 millions)
 - msg/s : 243,092,378.142349
 - MiB/s : 5834.22
 
 Multi Producer Single Consumer RingBuffer with size : 1048576 bytes (~ 1Mib) (using 2 publisher threads)
 - elapsed time: 20.774s (200 millions)
 - msg/s : 48,137,109.506653
 - MiB/s : 1155.29
 
  Multi Producer Single Consumer Orchestrator with multiple queues (each queue with size : 1048576 bytes (~ 1Mib)) (using 5 publisher threads)
 - elapsed time: 9.93218s (200 millions)
 - msg/s : 162,887,345.710341
 - MiB/s : 3909.3

License
----
Bogdan Gochev
MIT
