#pragma once

#include "Types.h"
#include "Blitter.h"



#define MAX_TILE_WIDTH		40
#define MAX_TILE_HEIGHT		40
#define TILE_STRUCT_SIZE	48

#define TILE_BIN_BLOCK_SIZE	(32)	//	gr; if not 32, there's flags for 64,128,256



typedef uint8_t TTileBin;

enum class TGpuThread : uint32_t
{
	Thread0		= 0,
	Thread1		= 1,
};



class TDisplay : public TBlitter
{
public:
	TDisplay(int Width,int Height,bool Mirror);
	
	void		SetResolution(uint32_t Width,uint32_t Height);


	void		SetupGpu();
	template<typename LAMBDA>
	void		GpuExecute(size_t ProgramSizeAlloc,LAMBDA& SetupProgram,TGpuThread GpuThread);
	void		GpuNopTest();
	
	//	returns program end
	uint8_t*	SetupBinControl(uint8_t* ProgramMemory,TTileBin* TileBinMemory,size_t TileBinMemorySize,void* TileStateMemory);
	uint8_t*	SetupRenderControl(uint8_t* Program,TTileBin* TileBinMemory);
	
	bool		ExecuteThread(uint8_t* ProgramStart,uint8_t* ProgramEnd,int Thread);

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


