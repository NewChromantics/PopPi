#include "Heap.h"




THeap::THeap(uint8_t* HeapStartAddress,size_t HeapSize) :
	mStartAddress	( HeapStartAddress ),
	mSize			( HeapSize ),
	mUsed			( 0 )
{
	//	CPU base memory address starts at 0x0! first alloc looks like it fails :)
	if ( mStartAddress == nullptr )
		mStartAddress = &mStartAddress[0x20];
}

uint8_t* THeap::Alloc(size_t Size)
{
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
	
