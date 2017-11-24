#pragma once

#include "Types.h"

enum class TGpuMemFlags : uint32_t
{
	Discardable		= 1<<0,	//	can be resized to 0 at any time. Use for cached data
	Normal			= 0,	//	normal allocating alias. Don't use from ARM
	Direct			= 1<<2,	//	0xC alias uncached
	Coherent		= 1<<3,	//	same as 2<<2	0x8 alias. Non-allocating in L2 but coherent
	L1NonAllocating	= Direct | Coherent,	//	Allocating in L2
	ZeroMemory		= 1<<4,	//	initialise all zero
	NoInit			= 1<<5,	//	don't initialise (default is initialise to all ones)
	HintPermalock	= 1<<6,	//	Likely to be locked for long periods of time
};


constexpr enum TGpuMemFlags operator |(const enum TGpuMemFlags selfValue,const enum TGpuMemFlags inValue)
{
	return (enum TGpuMemFlags)(uint32_t(selfValue) | uint32_t(inValue));
}

class TGpuMemory
{
	public:
	TGpuMemory(uint32_t Size,bool Lock);
	
	void		Clear(uint8_t Value);
	
	//	gr: making this explicit instead of in destructor as I can't debug to make sure any RValue copy is working correctly
	void 		Free();
	
	size_t		GetSize() const				{	return mSize;	}
	uint8_t*	GetCpuAddress() const;
	uint8_t*	GetGpuAddress() const;
	uint8_t*	GetBusAddress() const;
	uint8_t*	GetAllocAddress() const	{	return mLockedAddress;	}
	
	uint8_t*	Lock();
	bool		Unlock();
	
	public:
	uint32_t	mHandle;
	size_t		mSize;
	uint8_t*	mLockedAddress;
};



class TCpuMemory
{
public:
	TCpuMemory(uint32_t Size,bool Lock);
	
	void		Clear(uint8_t Value);
	
	//	gr: making this explicit instead of in destructor as I can't debug to make sure any RValue copy is working correctly
	void 		Free();
	
	size_t		GetSize() const				{	return mSize;	}
	uint8_t*	GetCpuAddress() const;
	uint8_t*	GetGpuAddress() const;
	uint8_t*	GetBusAddress() const;
	uint8_t*	GetAllocAddress() const	{	return mLockedAddress;	}
	
	

	
	uint8_t*	Lock();
	bool		Unlock();
	
	public:
	uint32_t	mHandle;
	size_t		mSize;
	uint8_t*	mLockedAddress;
};


class TMappedMemory
{
public:
	template<typename TYPE,size_t SIZE>
	TMappedMemory(TYPE (& Memory)[SIZE]) :
		mHandle			( 0x0051a11c ),
		mLockedAddress	( Memory ),
		mSize			( SIZE*sizeof(TYPE) )
	{
	}
	
	void		Clear(uint8_t Value);
	
	void 		Free()	{}
	
	size_t		GetSize() const				{	return mSize;	}
	uint8_t*	GetCpuAddress() const;
	uint8_t*	GetGpuAddress() const;
	uint8_t*	GetBusAddress() const;
	uint8_t*	GetAllocAddress() const	{	return mLockedAddress;	}
	
	uint8_t*	Lock()	{	return mLockedAddress;	}
	bool		Unlock()	{	return true;	}
	
public:
	uint32_t	mHandle;
	uint8_t*	mLockedAddress;
	size_t		mSize;
};


