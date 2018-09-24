#include "Types.h"

/*
extern "C" void memset(uint8_t* Data,uint8_t Value,size_t Length)
{
	for ( int i=0;	i<Length;	i++ )
	Data[i] = Value;
}


extern "C" void memcpy(uint8_t* Destination,const uint8_t* Source,size_t Length)
{
	for ( int i=0;	i<Length;	i++ )
	Destination[i] = Source[i];
}

*/

extern "C" void _exit()	{}
extern "C" void _sbrk()	{}
extern "C" void _kill()	{}
extern "C" void _getpid()	{}
extern "C" void _write()	{}
extern "C" void _lseek()	{}
extern "C" void _read()	{}
extern "C" void _gettimeofday(){}
extern "C" void _close(){}
