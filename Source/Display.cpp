#include "Display.h"
#include "Kernel.h"


uint8_t* TGpuMemory::GetCpuAddress() const
{
	return TKernel::GetCpuAddress(mLockedAddress);
}

uint8_t* TGpuMemory::GetGpuAddress() const
{
	return TKernel::GetGpuAddress(mLockedAddress);
}

