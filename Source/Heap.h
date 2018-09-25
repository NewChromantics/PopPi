#pragma once

#include "Types.h"

class THeap
{
public:
	THeap()	{}
	THeap(uint8_t* HeapStartAddress,size_t HeapSize);
	
	uint8_t*	Alloc(size_t Size);
	void		Free(uint8_t* Data);
	
public:
	uint8_t*	mStartAddress = nullptr;
	size_t		mSize = 0;
	size_t		mUsed = 0;
};

