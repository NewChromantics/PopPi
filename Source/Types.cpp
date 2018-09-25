#include "Types.h"
#include "Debug.h"

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
//	struggling to find the proper declarations, in case calling them is breaking the stack
//	https://github.com/bminor/newlib/blob/6497fdfaf41d47e835fdefc78ecb0a934875d7cf/libgloss/arm/syscalls.c
#include <unistd.h>
extern "C" void _exit(int Status)			{	Debug::Print("_exit()");	}
extern "C" void* _sbrk(ptrdiff_t)			{	Debug::Print("_sbrk()");	return nullptr;	}
extern "C" int _kill(int,int)				{	Debug::Print("_kill()");	return -1;	}
extern "C" pid_t _getpid()					{	Debug::Print("_getpid()");	return -1;	}
extern "C" int _write(int, const void *, size_t)		{	Debug::Print("_write()");	return -1;	}
extern "C" _off_t _lseek(int,_off_t,int)		{	Debug::Print("_lseek()");	return -1;	}
extern "C" int _read(int, void *, size_t)			{	Debug::Print("_read()");	return -1;	}
extern "C" int _gettimeofday(struct timeval * tp, void * tzvp)	{	Debug::Print("_gettimeofday()");	return -1;	}
extern "C" int _close(int)		{	Debug::Print("_close()");	return -1;	}


 #if !defined(__EXCEPTIONS)
namespace std
{
	void __throw_bad_function_call()
	{
		//throw bad_function_call("invalid function called");
	}
}
#endif


/*

 namespace std
 {
 inline void __throw_bad_function_call() { while(1); };
 }
 #endif
 */
