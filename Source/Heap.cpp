#include "Heap.h"




THeap::THeap(uint8_t* HeapStartAddress,size_t HeapSize) :
	mStartAddress	( HeapStartAddress ),
	mSize			( HeapSize ),
	mUsed			( 0 )
{
	//	CPU base memory address starts at 0x0! first alloc looks like it fails :)
	if ( mStartAddress == nullptr )
		mStartAddress = &mStartAddress[0x80000];

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
		NextData[i] = 0;
	mUsed += Size;
	//std::Debug << "Allocated " << Size << " now using " << HeapOffset << "/" << HEAP_SIZE << std::endl;
	return NextData;
}

void THeap::Free(uint8_t* Data)
{
}
	
