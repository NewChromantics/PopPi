#if defined(TARGET_OSX)
#include "Kernel.h"
#include "Display.h"
#include <thread>

extern int notmain();

int main(int argc, const char * argv[])
{
	notmain();
	return 0;
}


extern std::function<void(const char*)>* gPrintFunc;


#define HEAP_SIZE	0x8000000
uint8_t HeapMemory[HEAP_SIZE];

TKernel::TKernel()
{
	mHeap = THeap( (uint8_t*)HeapMemory, HEAP_SIZE );
	mDebugLogger = Debug::TLogger( mHeap );
	
	mPrintFunc = [&](const char* String)
	{
		mDebugLogger.Push( String );
	};
	gPrintFunc = &mPrintFunc;
	Debug::Print("Setup debug/kernel print");
	
}


void TKernel::Sleep(uint32_t Ms)
{
	std::this_thread::sleep_for( std::chrono::milliseconds(Ms) );
}


TDisplay::TDisplay(int Width,int Height,bool Mirror) :
	mScreenBuffer	( nullptr ),
	mWidth			( Width ),
	mHeight			( Height ),
	mClearColour	( RGBA( 255,0,255,255 ) ),
	TBlitter		( [this]{	return this->LockCanvas();	}, Mirror )
{
	mScreenBuffer = new uint32_t[ mWidth * mHeight ];
}


TCanvas<uint32_t> TDisplay::LockCanvas()
{
	TCanvas<uint32_t> Canvas( nullptr );
	Canvas.mPixels = mScreenBuffer;
	Canvas.mWidth = mWidth;
	Canvas.mHeight = mHeight;
	return Canvas;
}


#endif
