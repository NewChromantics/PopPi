#pragma once

#include <functional>

class THeap;

namespace Debug
{
	void		Print(const char* String);
	
	class TLogger;
}


class Debug::TLogger
{
public:
	TLogger();
	TLogger(THeap& Heap);

	void			Pop(std::function<void(const char*)> EnumLog);
	void			Push(const char* String);

private:
	char*			GetLogBuffer(size_t Index);
	void			SetLogBuffer(size_t Index,const char* String);

	static const size_t	mMaxLogLength = 200;
	static const size_t 	mMaxLogs = 50;
	size_t			mFirstUnpopped;
	size_t			mLogCount;
	char*			mLogsBuffer;
	static const size_t	mLogsBufferSize = mMaxLogLength * mMaxLogs;
};

