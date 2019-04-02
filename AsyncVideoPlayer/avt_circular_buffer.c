//
//  TPCircularBuffer.c
//  Circular/Ring buffer implementation
//
//  https://github.com/michaeltyson/TPCircularBuffer
//
//  Created by Michael Tyson on 10/12/2011.
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

#include "avt_circular_buffer.h"
//#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

/*
#define reportResult(result,operation) (_reportResult((result),(operation),strrchr(__FILE__, '/')+1,__LINE__))
static inline bool _reportResult(kern_return_t result, const char *operation, const char* file, int line) {
    if ( result != ERR_SUCCESS ) {
        printf("%s:%d: %s: %s\n", file, line, operation, mach_error_string(result)); 
        return false;
    }
    return true;
}
*/

  // Allocate a magic ring buffer at a given target address.
  //   ring_size      size of one copy of the ring; must be a multiple of 64k.
  //   desired_addr   location where you'd like it.
  void *_avt_circ_buffer_alloc_at(avt_circ_buffer * buffer, size_t ring_size, void *desired_addr)
  {
	size_t alloc_size;
    // if we already hold one allocation, refuse to make another.
    if (buffer->baseptr)
	{
	  fprintf(stderr, "TPCircularBufferInit failed:  buffer->baseptr already valid");
      return 0;
	}
    // is ring_size a multiple of 64k? if not, this won't ever work!
    if ((ring_size & 0xffff) != 0)
	{
	  fprintf(stderr, "TPCircularBufferInit failed:  ring_size is a multiple of 64k");
      return 0;
	}
    // try to allocate and map our space
    alloc_size = ring_size * 2;
    if (!(buffer->mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, (unsigned long long)alloc_size >> 32, alloc_size & 0xffffffffu, 0)) ||
      !(buffer->baseptr = (char *)MapViewOfFileEx(buffer->mapping, FILE_MAP_ALL_ACCESS, 0, 0, ring_size, desired_addr)) ||
      !MapViewOfFileEx(buffer->mapping, FILE_MAP_ALL_ACCESS, 0, 0, ring_size, (char *)desired_addr + ring_size))
    {
      // something went wrong - clean up
	  fprintf(stderr, "TPCircularBufferInit failed:  OS Virtual Mapping failed");
      avt_circ_buffer_cleanup(buffer);
    }
    else // success!
	{
	  fprintf(stderr, "TPCircularBufferInit Success:  OS Virtual Mapping succeeded");
      buffer->length = ring_size;
	}
    return buffer->baseptr;
  }

  
  // Determine a viable target address of "size" memory mapped bytes by
  // allocating memory using VirtualAlloc and immediately freeing it. This
  // is subject to a potential race condition, see notes above.
  static void *determine_viable_addr(size_t size)
  {
    void *ptr = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    if (!ptr)
      return 0;

    VirtualFree(ptr, 0, MEM_RELEASE);
    return ptr;
  }


  // This function will allocate a magic ring buffer at a system-determined base address.
  //
  // Sadly, there's no way (that I can see) in the Win32 API to first reserve
  // a memory region then fill it in using mmaps; you can reserve memory via
  // VirtualAlloc, but that address range can then only be used to commit
  // memory via another VirtualAlloc, and can not be mmap'ed. Furthermore,
  // there's also no way to do the two back-to-back mmaps atomically. What we
  // do here is to reserve enough memory via VirtualAlloc, then immediately
  // free it and try to put our allocation there. This is subject to a race
  // condition - another thread might end up allocating that very memory
  // region in the interim. What this means is that even when an alloc should
  // work (i.e. there's enough memory available) it can still fail spuriously
  // sometimes. Hence the "dicey" comment above.
  //
  // What we do here is just retry the alloc a given number of times and hope
  // that we don't get screwed every single time. This increases the
  // likelihood of success, but doesn't eliminate the chance of spurious
  // failure, so be religious about checking return values!
  void *_avt_circ_buffer_alloc(avt_circ_buffer * buffer, size_t ring_size, int num_retries)
  {
    void *ptr = 0;
	//buffer->length = (int32_t)round_page(ring_size);
    while (!ptr && num_retries-- != 0)
    {
      void *target_addr = determine_viable_addr(ring_size * 2);
      if (target_addr){
        ptr = _avt_circ_buffer_alloc_at(buffer, ring_size, target_addr);

		buffer->buffer = target_addr;
        buffer->fillCount = 0;
        buffer->head = buffer->tail = 0;
        buffer->atomic = true;

	  }
    }

    return ptr;
  }

  // Frees the allocated region again.
  void avt_circ_buffer_cleanup(avt_circ_buffer * buffer)
  {
    if (buffer->baseptr)
    {
      UnmapViewOfFile(buffer->baseptr);
      UnmapViewOfFile(buffer->baseptr + buffer->length);
      buffer->baseptr = 0;
    }

    if (buffer->mapping)
    {
      CloseHandle(buffer->mapping);
      buffer->mapping = 0;
    }

    buffer->length = 0;

	memset(buffer, 0, sizeof(avt_circ_buffer));
  }


bool _avt_circ_buffer_init (avt_circ_buffer *buffer, int32_t length, size_t structSize) {
    
    assert(length > 0);
    
    if ( structSize != sizeof(avt_circ_buffer) ) {
        fprintf(stderr, "avt_circ_buffer: Header version mismatch. Check for old versions of TPCircularBuffer in your project\n");
        abort();
    }
  
	_avt_circ_buffer_alloc(buffer, length, 5);

	if( buffer->baseptr != NULL)
		return true;
	/*
    // Keep trying until we get our buffer, needed to handle race conditions
    int retries = 3;
    while ( true ) {

        buffer->length = (int32_t)round_page(length);    // We need whole page sizes

        // Temporarily allocate twice the length, so we have the contiguous address space to
        // support a second instance of the buffer directly after
        vm_address_t bufferAddress;
        kern_return_t result = vm_allocate(mach_task_self(),
                                           &bufferAddress,
                                           buffer->length * 2,
                                           VM_FLAGS_ANYWHERE); // allocate anywhere it'll fit
        if ( result != ERR_SUCCESS ) {
            if ( retries-- == 0 ) {
                reportResult(result, "Buffer allocation");
                return false;
            }
            // Try again if we fail
            continue;
        }
        
        // Now replace the second half of the allocation with a virtual copy of the first half. Deallocate the second half...
        result = vm_deallocate(mach_task_self(),
                               bufferAddress + buffer->length,
                               buffer->length);
        if ( result != ERR_SUCCESS ) {
            if ( retries-- == 0 ) {
                reportResult(result, "Buffer deallocation");
                return false;
            }
            // If this fails somehow, deallocate the whole region and try again
            vm_deallocate(mach_task_self(), bufferAddress, buffer->length);
            continue;
        }
        
        // Re-map the buffer to the address space immediately after the buffer
        vm_address_t virtualAddress = bufferAddress + buffer->length;
        vm_prot_t cur_prot, max_prot;
        result = vm_remap(mach_task_self(),
                          &virtualAddress,   // mirror target
                          buffer->length,    // size of mirror
                          0,                 // auto alignment
                          0,                 // force remapping to virtualAddress
                          mach_task_self(),  // same task
                          bufferAddress,     // mirror source
                          0,                 // MAP READ-WRITE, NOT COPY
                          &cur_prot,         // unused protection struct
                          &max_prot,         // unused protection struct
                          VM_INHERIT_DEFAULT);
        if ( result != ERR_SUCCESS ) {
            if ( retries-- == 0 ) {
                reportResult(result, "Remap buffer memory");
                return false;
            }
            // If this remap failed, we hit a race condition, so deallocate and try again
            vm_deallocate(mach_task_self(), bufferAddress, buffer->length);
            continue;
        }
        
        if ( virtualAddress != bufferAddress+buffer->length ) {
            // If the memory is not contiguous, clean up both allocated buffers and try again
            if ( retries-- == 0 ) {
                printf("Couldn't map buffer memory to end of buffer\n");
                return false;
            }

            vm_deallocate(mach_task_self(), virtualAddress, buffer->length);
            vm_deallocate(mach_task_self(), bufferAddress, buffer->length);
            continue;
        }
        
        buffer->buffer = (void*)bufferAddress;
        buffer->fillCount = 0;
        buffer->head = buffer->tail = 0;
        buffer->atomic = true;
        
        return true;
    }
	*/

	printf("\navt_circ_buffer_init Failed\n");
    return false;
}

/*
void TPCircularBufferCleanup(TPCircularBuffer *buffer) {
    vm_deallocate(mach_task_self(), (vm_address_t)buffer->buffer, buffer->length * 2);
    memset(buffer, 0, sizeof(TPCircularBuffer));
}
*/

void avt_circ_buffer_clear(avt_circ_buffer *buffer) {
    int32_t fillCount;
    if ( avt_circ_buffer_tail(buffer, &fillCount) ) {
        avt_circ_buffer_consume(buffer, fillCount);
    }
}

void  avt_circ_buffer_set_atomic(avt_circ_buffer *buffer, bool atomic) {
    buffer->atomic = atomic;
}
