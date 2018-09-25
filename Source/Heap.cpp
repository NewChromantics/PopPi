#include "Heap.h"



#define RASPPI 1

#ifndef MEGABYTE
#define MEGABYTE	0x100000
#endif
#define KERNEL_MAX_SIZE		(2 * MEGABYTE)




#define MEM_SIZE		(256 * MEGABYTE)		// default size
#define GPU_MEM_SIZE		(64 * MEGABYTE)			// set in config.txt
#define ARM_MEM_SIZE		(MEM_SIZE - GPU_MEM_SIZE)	// normally overwritten

#define PAGE_SIZE		4096				// page size used by us

#define KERNEL_STACK_SIZE	0x20000				// all sizes must be a multiple of 16K
#define EXCEPTION_STACK_SIZE	0x8000
#define PAGE_TABLE1_SIZE	0x4000
#define PAGE_RESERVE		(4 * MEGABYTE)

#define MEM_KERNEL_START	0x8000
#define MEM_KERNEL_END		(MEM_KERNEL_START + KERNEL_MAX_SIZE)
#define MEM_KERNEL_STACK	(MEM_KERNEL_END + KERNEL_STACK_SIZE)		// expands down
#if RASPPI == 1
#define MEM_ABORT_STACK		(MEM_KERNEL_STACK + EXCEPTION_STACK_SIZE)	// expands down
#define MEM_IRQ_STACK		(MEM_ABORT_STACK + EXCEPTION_STACK_SIZE)	// expands down
#define MEM_FIQ_STACK		(MEM_IRQ_STACK + EXCEPTION_STACK_SIZE)		// expands down
#define MEM_PAGE_TABLE1		MEM_FIQ_STACK				// must be 16K aligned
#else
#define CORES			4					// must be a power of 2
#define MEM_ABORT_STACK		(MEM_KERNEL_STACK + KERNEL_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_IRQ_STACK		(MEM_ABORT_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_FIQ_STACK		(MEM_IRQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_PAGE_TABLE1		(MEM_FIQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1))
#endif
#define MEM_PAGE_TABLE1_END	(MEM_PAGE_TABLE1 + PAGE_TABLE1_SIZE)

// coherent memory region (1 section)
#define MEM_COHERENT_REGION	((MEM_PAGE_TABLE1_END + 2*MEGABYTE) & ~(MEGABYTE-1))

#define MEM_HEAP_START		(MEM_COHERENT_REGION + MEGABYTE)

#if !defined(TARGET_OSX)
//#define USE_GLOBAL_AS_HEAP
#define USE_OFFSET_FROM_LOADER		(0x8000+0x3000000)
#endif


#if defined(USE_GLOBAL_AS_HEAP)
uint8_t HeapData[MEGABYTE*30];
#endif


THeap::THeap(uint8_t* HeapStartAddress,size_t HeapSize) :
	mStartAddress	( HeapStartAddress ),
	mSize			( HeapSize ),
	mUsed			( 0 )
{
	/*
	auto StartAddressInt = (uintptr_t)mStartAddress;
	if ( StartAddressInt < MEM_HEAP_START )
		StartAddressInt = MEM_HEAP_START;
	mStartAddress = (uint8_t*)StartAddressInt;

	//	CPU base memory address starts at 0x0! first alloc looks like it fails :)
	if ( mStartAddress == nullptr )
		mStartAddress = &mStartAddress[MEM_HEAP_START];
*/
#if defined(USE_GLOBAL_AS_HEAP)
	mStartAddress = HeapData;
	mSize = sizeof(HeapData);
#endif
#if defined(USE_OFFSET_FROM_LOADER)
	mStartAddress = (uint8_t*)(USE_OFFSET_FROM_LOADER);
#endif
}

uint8_t* THeap::Alloc(size_t Size)
{
	#define ALLOC_ALIGNMENT				0x1000
	//	auto align
	auto Align = ALLOC_ALIGNMENT;
	mUsed += Align-1;
	mUsed -= mUsed % Align;
	
	uint8_t* NextData = &mStartAddress[mUsed];
	for ( int i=0;	i<Size;	i++ )
		NextData[i] = 0xba;
	mUsed += Size;
	//std::Debug << "Allocated " << Size << " now using " << HeapOffset << "/" << HEAP_SIZE << std::endl;
	return NextData;
}

void THeap::Free(uint8_t* Data)
{
}
	
