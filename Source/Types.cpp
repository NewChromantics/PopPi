#include "Types.h"
#include "Debug.h"



#if !defined(TARGET_OSX)
#include "Kernel.h"
//	struggling to find the proper declarations, in case calling them is breaking the stack
//	https://github.com/bminor/newlib/blob/6497fdfaf41d47e835fdefc78ecb0a934875d7cf/libgloss/arm/syscalls.c
#include <unistd.h>
#define SLEEP_ON_STDFUNC_MS	30000
extern "C" void _exit(int Status)			{	Debug::Print("_exit()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);}
extern "C" void* _sbrk(ptrdiff_t)			{	Debug::Print("_sbrk()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);	return nullptr;	}
extern "C" int _kill(int,int)				{	Debug::Print("_kill()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
extern "C" pid_t _getpid()					{	Debug::Print("_getpid()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
extern "C" int _write(int, const void *, size_t)		{	Debug::Print("_write()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
extern "C" off_t _lseek(int,off_t,int)		{	Debug::Print("_lseek()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
extern "C" int _read(int, void *, size_t)			{	Debug::Print("_read()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
extern "C" int _gettimeofday(struct timeval * tp, void * tzvp)	{	Debug::Print("_gettimeofday()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
extern "C" int _close(int)		{	Debug::Print("_close()");	TKernel::Sleep(SLEEP_ON_STDFUNC_MS);return -1;	}
#endif

 #if !defined(__EXCEPTIONS)
namespace std
{
	void __throw_bad_function_call()
	{
		//throw bad_function_call("invalid function called");
	}
}
#endif

