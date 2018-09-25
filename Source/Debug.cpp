#include "Debug.h"
#include "Kernel.h"



void Debug::Print(const char* String)
{
	TKernel::DebugLog(String);
}



Debug::TLogger::TLogger() :
	mFirstUnpopped	( 0 ),
	mLogCount		( 0 ),
	mLogsBuffer		( nullptr )
{
}

Debug::TLogger::TLogger(THeap& Heap) :
	TLogger()
{
	//	alloc data
	mLogsBuffer = reinterpret_cast<char*>( Heap.Alloc(mLogsBufferSize) );
}
	
char* Debug::TLogger::GetLogBuffer(size_t Index)
{
	auto Offset = (Index % mMaxLogs) * mMaxLogLength;
	auto* Buffer = &mLogsBuffer[Offset];
	return Buffer;
}

void Debug::TLogger::SetLogBuffer(size_t Index,const char* String)
{
	auto* Buffer = GetLogBuffer(Index);
	for ( int i=0;	i<mMaxLogLength;	i++ )
	{
		auto Char = String[i];
		Buffer[i] = Char;
		if ( Char == 0 )
			break;
	}
	Buffer[mMaxLogLength-1] = 0;
}

void Debug::TLogger::Pop(std::function<void(const char*)> EnumLog)
{
	for ( int i=mFirstUnpopped;	i<mLogCount;	i++ )
	{
		auto* Buffer = GetLogBuffer(i);
		EnumLog(Buffer);
		
		//	clear after pop
		mFirstUnpopped = i+1;
		Buffer[0] = 0;
	}
}

void Debug::TLogger::Push(const char* String)
{
	auto Index = mLogCount++;
	SetLogBuffer( Index, String );
}

