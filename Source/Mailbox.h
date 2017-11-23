#pragma once


#include "Types.h"

namespace TMailbox
{
	enum class TChannel	: uint32_t;
	enum class TTag		: uint32_t;
	
	volatile uint32_t*		AllocBuffer(int Size);

	void					Write(volatile void* Data,TMailbox::TChannel Channel);
	uint32_t				Read(TMailbox::TChannel Channel);

	//	returns amount of data returned
	template<size_t PAYLOADSIZE>
	int						SetProperty(TTag Tag,TChannel Channel,uint32_t (& Payload)[PAYLOADSIZE],bool Read=true);

	bool					EnableQpu(bool Enable=true);
}




//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
enum class TMailbox::TChannel : uint32_t
{
	One		= 1,
	Gpu		= 8,	//	Arm communicating with video controller (ArmToVc)
};

	
enum class TMailbox::TTag : uint32_t
{
	GetCpuBaseAddress	= 0x00010005,	//	arm
	GetGpuBaseAddress	= 0x00010006,	//	videocore
		
	AllocGpuMemory	= 0x3000c,
	FreeGpuMemory	= 0x3000f,
	LockGpuMemory	= 0x3000d,
	UnlockGpuMemory	= 0x3000e,
		
	SetResolution	= 0x00048003,
		
	SetClockRate	= 0x00038002,
};
		




//	returns number of elements returned
template<size_t PAYLOADSIZE>
int TMailbox::SetProperty(TMailbox::TTag Tag,TMailbox::TChannel Channel,uint32_t (& Payload)[PAYLOADSIZE],bool ReadData)
{
	auto Tag32 = static_cast<uint32_t>( Tag );
	auto Size = PAYLOADSIZE + 6;
	//volatile static uint32_t Data[PAYLOADSIZE + 6] __attribute__ ((aligned(16)));
	auto* Data = AllocBuffer(Size+10);
	
	Data[0] = Size * sizeof(uint32_t);
	Data[1] = 0x00000000;	// process request
	
	Data[2] = Tag32;
	Data[3] = PAYLOADSIZE * sizeof(uint32_t);	//	size of buffer
	//Data[4] = PAYLOADSIZE * sizeof(uint32_t);	//	size of data returned. needs to be 0 to send
	Data[4] = 0;	//	size of data returned. needs to be 0 to send
	for ( unsigned i=0;	i<PAYLOADSIZE;	i++ )
	{
		Data[5+i] = Payload[i];
	}
	Data[5+PAYLOADSIZE] = 0;	//	end tag
	
	//	set data
	TMailbox::Write( Data, Channel );
	
	if ( !ReadData )
		return 0;
	
	TMailbox::Read( Channel );
	
	auto ReturnedTag = Data[2];
	if ( ReturnedTag != static_cast<uint32_t>( Tag ) )
		return -1;
	
#define BIT_RESPONSE_VALID	(1<<31)
	auto ReturnedDataSize = Data[4] & ~BIT_RESPONSE_VALID;
	
	//	read back and return
	for ( unsigned i=0;	i<PAYLOADSIZE;	i++ )
	{
		Payload[i] = Data[5+i];
	}
	
	return ReturnedDataSize / sizeof(uint32_t);
}

