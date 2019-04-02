//
//  TPCircularBuffer.h
//  Circular/Ring buffer implementation
//
//  https://github.com/michaeltyson/TPCircularBuffer
//
//  Created by Michael Tyson on 10/12/2011.
//
//
//  This implementation makes use of a virtual memory mapping technique that inserts a virtual copy
//  of the buffer memory directly after the buffer's end, negating the need for any buffer wrap-around
//  logic. Clients can simply use the returned memory address as if it were contiguous space.
//  
//  The implementation is thread-safe in the case of a single producer and single consumer.
//
//  Virtual memory technique originally proposed by Philip Howard (http://vrb.slashusr.org/), and
//  adapted to Darwin by Kurt Revis (http://www.snoize.com,
//  http://www.snoize.com/Code/PlayBufferedSoundFile.tar.gz)
//
//
//  Copyright (C) 2012-2013 A Tasty Pixel
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//

#ifndef avt_circular_buffer_h
#define avt_circular_buffer_h

//#include <libkern/OSAtomic.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//only define bool if not previoulsy defined by CoreRender crWindow global header
//and not using C++
#ifndef __cplusplus
//typedef unsigned char bool;
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif
#endif

typedef struct {
    void             *buffer;
	HANDLE			  mapping;
	char			 *baseptr;
    int32_t           length;
    int32_t           tail;
    int32_t           head;
    volatile int32_t  fillCount; //since fill count is volatile it will be atomic on win32
    bool              atomic;
} avt_circ_buffer;

/*!
 * Initialise buffer
 *
 *  Note that the length is advisory only: Because of the way the
 *  memory mirroring technique works, the true buffer length will
 *  be multiples of the device page size (e.g. 4096 bytes)
 *
 * @param buffer Circular buffer
 * @param length Length of buffer
 */
#define avt_circ_buffer_init(buffer, length) \
    _avt_circ_buffer_init(buffer, length, sizeof(*buffer))
bool _avt_circ_buffer_init(avt_circ_buffer *buffer, int32_t length, size_t structSize);

/*!
 * Cleanup buffer
 *
 *  Releases buffer resources.
 */
void  avt_circ_buffer_cleanup(avt_circ_buffer *buffer);

/*!
 * Clear buffer
 *
 *  Resets buffer to original, empty state.
 *
 *  This is safe for use by consumer while producer is accessing 
 *  buffer.
 */
void  avt_circ_buffer_clear(avt_circ_buffer *buffer);
    
/*!
 * Set the atomicity
 *
 *  If you set the atomiticy to false using this method, the buffer will
 *  not use atomic operations. This can be used to give the compiler a little
 *  more optimisation opportunities when the buffer is only used on one thread.
 *
 *  Important note: Only set this to false if you know what you're doing!
 *
 *  The default value is true (the buffer will use atomic operations)
 *
 * @param buffer Circular buffer
 * @param atomic Whether the buffer is atomic (default true)
 */
void  avt_circ_buffer_set_atomic(avt_circ_buffer *buffer, bool atomic);

// Reading (consuming)

/*!
 * Access end of buffer
 *
 *  This gives you a pointer to the end of the buffer, ready
 *  for reading, and the number of available bytes to read.
 *
 * @param buffer Circular buffer
 * @param availableBytes On output, the number of bytes ready for reading
 * @return Pointer to the first bytes ready for reading, or NULL if buffer is empty
 */
static __inline void* avt_circ_buffer_tail(avt_circ_buffer *buffer, int32_t* availableBytes) {
    *availableBytes = buffer->fillCount;
    if ( *availableBytes == 0 ) return NULL;
    return (void*)((char*)buffer->buffer + buffer->tail);
}

/*!
 * Consume bytes in buffer
 *
 *  This frees up the just-read bytes, ready for writing again.
 *
 * @param buffer Circular buffer
 * @param amount Number of bytes to consume
 */
static __inline void avt_circ_buffer_consume(avt_circ_buffer *buffer, int32_t amount) {
    buffer->tail = (buffer->tail + amount) % buffer->length;
    if ( buffer->atomic ) {
        //OSAtomicAdd32Barrier(-amount, &buffer->fillCount);
		//since filcount is volatile it will always be atomic on windows
		buffer->fillCount -= amount;

	} else {
        buffer->fillCount -= amount;
    }
    assert(buffer->fillCount >= 0);
}

/*!
 * Access front of buffer
 *
 *  This gives you a pointer to the front of the buffer, ready
 *  for writing, and the number of available bytes to write.
 *
 * @param buffer Circular buffer
 * @param availableBytes On output, the number of bytes ready for writing
 * @return Pointer to the first bytes ready for writing, or NULL if buffer is full
 */
static __inline void* avt_circ_buffer_head(avt_circ_buffer *buffer, int32_t* availableBytes) {
    *availableBytes = (buffer->length - buffer->fillCount);
    if ( *availableBytes == 0 ) return NULL;
    return (void*)((char*)buffer->buffer + buffer->head);
}
    
// Writing (producing)

/*!
 * Produce bytes in buffer
 *
 *  This marks the given section of the buffer ready for reading.
 *
 * @param buffer Circular buffer
 * @param amount Number of bytes to produce
 */
static __inline void avt_circ_buffer_produce(avt_circ_buffer *buffer, int32_t amount) {
    buffer->head = (buffer->head + amount) % buffer->length;
    if ( buffer->atomic ) {
        //OSAtomicAdd32Barrier(amount, &buffer->fillCount);
		buffer->fillCount += amount;

	} else {
        buffer->fillCount += amount;
    }
    assert(buffer->fillCount <= buffer->length);
}

/*!
 * Helper routine to copy bytes to buffer
 *
 *  This copies the given bytes to the buffer, and marks them ready for reading.
 *
 * @param buffer Circular buffer
 * @param src Source buffer
 * @param len Number of bytes in source buffer
 * @return true if bytes copied, false if there was insufficient space
 */
static __inline bool avt_circ_buffer_produce_bytes(avt_circ_buffer *buffer, const void* src, int32_t len) {
    int32_t space;
    void *ptr = avt_circ_buffer_head(buffer, &space);
    if ( space < len ) return false;
    memcpy(ptr, src, len);
    avt_circ_buffer_produce(buffer, len);
    return true;
}


#ifdef __cplusplus
}
#endif

#endif
