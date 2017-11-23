#pragma once

#include "Types.h"
#include "Blitter.h"



#define MAX_TILE_WIDTH		40
#define MAX_TILE_HEIGHT		40
#define TILE_STRUCT_SIZE	48

#define TILE_BIN_BLOCK_SIZE	(32)	//	gr; if not 32, there's flags for 64,128,256



typedef uint8_t TTileBin;


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

enum class TGpuThread : uint32_t
{
	Thread0		= 0,
	Thread1		= 1,
};



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


class TDisplay : public TBlitter
{
public:
	TDisplay(int Width,int Height);
	
	void		SetResolution(uint32_t Width,uint32_t Height);


	void		SetupGpu();
	template<typename LAMBDA>
	void		GpuExecute(size_t ProgramSizeAlloc,LAMBDA& SetupProgram,TGpuThread GpuThread);
	void		GpuNopTest();
	
	bool		SetupBinControl(void* ProgramMemory,TTileBin* TileBinMemory,size_t TileBinMemorySize,void* TileStateMemory);
	uint8_t*	SetupRenderControlProgram(uint8_t* Program,TTileBin* TileBinMemory);
	bool		SetupRenderControl(void* ProgramMemory,TTileBin* TileBinMemory);
	

	uint8_t		GetTileWidth() const	{	return (mWidth-(mWidth%64))/64;	}	//	round down so we don't overflow pixel buffer
	uint8_t		GetTileHeight() const	{	return (mHeight-(mHeight%64))/64;	}	//	round down so we don't overflow pixel buffer

	TCanvas<uint32_t>	LockCanvas();
	
	void		WaitForVsync();
	void		EnableVsync();
	void		OnVsync();
	
public:
	uint32_t	mVsyncCount;
	bool		mVsyncEnabled;
	uint32_t	mClearColour;
	uint32_t	mWidth;
	uint32_t	mHeight;
	uint32_t*	mScreenBuffer;
};


