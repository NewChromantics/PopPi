#include "Memory.h"
#include "Kernel.h"
#include "Mailbox.h"


#define ALLOC_ALIGNMENT				0x1000




uint8_t* TGpuMemory::GetCpuAddress() const	{	return TKernel::GetCpuAddress(mLockedAddress);	}
uint8_t* TGpuMemory::GetGpuAddress() const	{	return TKernel::GetGpuAddress(mLockedAddress);	}
uint8_t* TGpuMemory::GetBusAddress() const	{	return TKernel::GetBusAddress(mLockedAddress);	}

uint8_t* TCpuMemory::GetCpuAddress() const	{	return TKernel::GetCpuAddress(mLockedAddress);	}
uint8_t* TCpuMemory::GetGpuAddress() const	{	return TKernel::GetGpuAddress(mLockedAddress);	}
uint8_t* TCpuMemory::GetBusAddress() const	{	return TKernel::GetBusAddress(mLockedAddress);	}

uint8_t* TMappedMemory::GetCpuAddress() const	{	return TKernel::GetCpuAddress(mLockedAddress);	}
uint8_t* TMappedMemory::GetGpuAddress() const	{	return TKernel::GetGpuAddress(mLockedAddress);	}
uint8_t* TMappedMemory::GetBusAddress() const	{	return TKernel::GetBusAddress(mLockedAddress);	}






TGpuMemory::TGpuMemory(uint32_t Size,bool Lock) :
	mHandle			( 0 ),
	mSize			( Size ),
	mLockedAddress	( nullptr )
{
	//	gr: most things need to be aligned to 16 bytes
	auto AlignBytes = ALLOC_ALIGNMENT;
	//auto AlignBytes = 16;
	auto Flags = (uint32_t)(TGpuMemFlags::Coherent | TGpuMemFlags::ZeroMemory);
	
	uint32_t Data[3];
	Data[0] = Size;
	Data[1] = AlignBytes;
	Data[2] = Flags;
	
	auto ReturnSize = TMailbox::SetProperty( TMailbox::TTag::AllocGpuMemory, TMailbox::TChannel::Gpu, Data );
	if ( ReturnSize == -1 )
	{
		mHandle = 0x0bad2000 | 0xffff;
		return;
	}
	if ( ReturnSize != 1 )
	{
		mHandle = 0x0bad2000 | ReturnSize;
		return;
	}
	
	mHandle = Data[0];
	
	if ( Lock )
	mLockedAddress = this->Lock();
}

void TGpuMemory::Clear(uint8_t Value)
{
	auto* Addr = GetGpuAddress();
	for ( int i=0;	i<mSize;	i++ )
	{
		Addr[i] = Value;
	}
}


void TGpuMemory::Free()
{
	Unlock();
	
	uint32_t Data[1];
	Data[0] = mHandle;
	
	TMailbox::SetProperty( TMailbox::TTag::FreeGpuMemory, TMailbox::TChannel::Gpu, Data );
}

uint8_t* TGpuMemory::Lock()
{
	uint32_t Data[1];
	Data[0] = mHandle;
	
	auto ResponseSize = TMailbox::SetProperty( TMailbox::TTag::LockGpuMemory, TMailbox::TChannel::Gpu, Data );
	//if ( ResponseSize != 1 )
	//	return (uint8_t*)(uint32_t)(0xbad10000|ResponseSize);
	
	auto Address = Data[0];
	return (uint8_t*)Address;
}


bool TGpuMemory::Unlock()
{
	uint32_t Data[1];
	Data[0] = mHandle;
	
	auto ResponseSize = TMailbox::SetProperty( TMailbox::TTag::UnlockGpuMemory, TMailbox::TChannel::Gpu, Data );
	if ( ResponseSize != 1 )
	return false;
	
	bool Success = (Data[0] == 0);
	
	mLockedAddress = nullptr;
	return Success;
}







//	10mb causes trash startup...
uint8_t gCpuMemoryBlock[1024*1024]__attribute__ ((aligned(16)));
//uint8_t gCpuMemoryBlock[ 1024 * 1024 * 10]__attribute__ ((aligned(16)));
int gCpuMemoryBlockAllocated = 1;
int gCpuMemoryBlockHandleNext = 1;


TCpuMemory::TCpuMemory(uint32_t Size,bool Lock) :
	mHandle			( 0xa110c000 | (gCpuMemoryBlockHandleNext++) ),
	mSize			( Size ),
	mLockedAddress	( nullptr )
{
	//	alloc
	auto Align = ALLOC_ALIGNMENT;
	uint32_t NewAddress = (uint32_t)&gCpuMemoryBlock[ gCpuMemoryBlockAllocated ];
	
	//	align
	NewAddress += Align-1;
	NewAddress -= NewAddress % Align;
	
	mLockedAddress = (uint8_t*)NewAddress;
	
	//	count bytes eaten
	auto Eaten = mSize + NewAddress - (uint32_t)gCpuMemoryBlock;
	gCpuMemoryBlockAllocated = Eaten;
	
	Clear(0);
}

void TCpuMemory::Clear(uint8_t Value)
{
	for ( int i=0;	i<mSize;	i++ )
		mLockedAddress[i] = Value;
}


void TCpuMemory::Free()
{
	mLockedAddress = nullptr;
	mHandle = 0;
}

uint8_t* TCpuMemory::Lock()
{
	return mLockedAddress;
}

bool TCpuMemory::Unlock()
{
	return true;
}





void TMappedMemory::Clear(uint8_t Value)
{
	for ( int i=0;	i<mSize;	i++ )
	{
		mLockedAddress[i] = Value;
	}
}


