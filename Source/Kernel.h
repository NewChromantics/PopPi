#pragma once


#include "Types.h"


#if defined(TARGET_CPP)
#define CAPI	extern "C"
extern "C"
{
#else
#define CAPI
#endif

	extern void PUT32 ( unsigned int, unsigned int );
	extern void PUT16 ( unsigned int, unsigned int );
	extern void PUT8 ( unsigned int, unsigned int );
	extern unsigned int GET32 ( unsigned int );
	extern unsigned int GETPC ( void );
	extern void BRANCHTO ( unsigned int );
	extern void dummy ( unsigned int );

	extern void uart_init ( void );
	extern unsigned int uart_lcr ( void );
	extern void uart_flush ( void );
	extern void uart_send ( unsigned int );
	extern unsigned int uart_recv ( void );
	extern unsigned int uart_check ( void );
	extern void hexstring ( unsigned int );
	extern void hexstrings ( unsigned int );
	extern void timer_init ( void );
	extern unsigned int timer_tick ( void );

	extern void timer_init ( void );
	extern unsigned int timer_tick ( void );
	
	extern void start_l1cache ( void );
	extern void stop_l1cache ( void );
	extern void start_mmu ( unsigned int, unsigned int );
	extern unsigned int LDREX ( unsigned int, unsigned int );
	extern unsigned int STREX ( unsigned int, unsigned int, unsigned int );
	extern unsigned int EXTEST ( unsigned int, unsigned int, unsigned int );
	
#if defined(TARGET_CPP)
}
#endif


class TKernel
{
public:
	TKernel();
	

	#define CPU_START_ADDRESS	0x40000000
	//#define CPU_START_ADDRESS	0x00000000
#define GPU_START_ADDRESS	0x80000000
//#define GPU_START_ADDRESS	0x08000000
#define BUS_MASK_ADDRESS	0x3fffffff
	
	template<typename T>
	static T*	GetUnmodifiedAddress(T* Addr)
	{
		return Addr;
	}

	
	template<typename T>
	static T*	GetGpuAddress(T* Addr)
	{
		if ( !Addr )
			return Addr;
		//	map memory address to physical address... so... the cached/physical memory/virtual memory address over the bus
		//	gr: note; not l1cache!
		//	L2cache enabled
		//BufferAddress |= 0x40000000;
		//	L2cache disabled
		//BufferAddress |= 0xC0000000;
		
		auto Addr32 = (uint32_t)Addr;
		Addr32 &= BUS_MASK_ADDRESS;
		Addr32 |= GPU_START_ADDRESS;
		return (T*)Addr32;
	}
	
	template<typename T>
	static T*	GetCpuAddress(T* Addr)
	{
		if ( !Addr )
			return Addr;
		auto Addr32 = (uint32_t)Addr;
		Addr32 &= BUS_MASK_ADDRESS;
		Addr32 |= CPU_START_ADDRESS;
		return (T*)Addr32;
	}
	
	template<typename T>
	static T*	GetBusAddress(T* Addr)
	{
		if ( !Addr )
			return Addr;
		auto Addr32 = (uint32_t)Addr;
		Addr32 &= BUS_MASK_ADDRESS;
		return (T*)Addr32;
	}
	
	template<typename T> static uint32_t	GetCpuAddress32(T* Addr)	{	return (uint32_t)GetCpuAddress(Addr);	}
	template<typename T> static uint32_t	GetGpuAddress32(T* Addr)	{	return (uint32_t)GetGpuAddress(Addr);	}
	template<typename T> static uint32_t	GetBusAddress32(T* Addr)	{	return (uint32_t)GetBusAddress(Addr);	}
	template<typename T> static uint32_t	GetUnmodifiedAddress32(T* Addr)	{	return (uint32_t)GetUnmodifiedAddress(Addr);	}
	
	static void		Sleep(uint32_t Milliseconds);
	
public:
	static uint32_t	mCpuMemoryBase;
	static uint32_t	mCpuMemorySize;
	static uint32_t	mGpuMemoryBase;
	static uint32_t	mGpuMemorySize;
};



