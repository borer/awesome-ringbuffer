#ifndef QUEUE_ATOMIC64_H
#define QUEUE_ATOMIC64_H

#if defined(__GNUC__) || defined(__clang__)
/* LY: noting needed */
#elif defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#	include <windows.h>
#	include <winnt.h>
#	include <intrin.h>
#   pragma intrinsic(_ReadWriteBarrier)
#endif

#if defined(__clang__) || defined(__GNUC__)
	#define COMPILER_BARRIER __asm__ volatile("" ::: "memory")
#elif defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
	#define COMPILER_BARRIER _ReadWriteBarrier()
#else
	#error "Could not guess the kind of compiler, please report to us."
#endif

#define RING_BUFFER_GET_VOLATILE(dst,src) \
do \
{ \
    dst = src; \
    COMPILER_BARRIER; \
} while(false)

#define RING_BUFFER_PUT_ORDERED(dst,src) \
do \
{ \
    COMPILER_BARRIER; \
    dst = src; \
} while(false)

#endif // QUEUE_ATOMIC64_H
