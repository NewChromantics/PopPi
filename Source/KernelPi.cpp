#include "Kernel.h"
#include "Mailbox.h"

extern std::function<void(const char*)>* gPrintFunc;


uint32_t TKernel::mCpuMemoryBase = 0xbad0bad1;
uint32_t TKernel::mCpuMemorySize = 0xbad2bad3;
uint32_t TKernel::mGpuMemoryBase = 0xbad4bad5;
uint32_t TKernel::mGpuMemorySize = 0xbad6bad7;

void TKernel::Sleep(uint32_t Ms)
{
	//	random number, tis loop is basicaly "sleep for x ticks" so 250/1000mhz of nops is what we need?
	Ms *= 1 * 250 * 100;
	while ( Ms > 0 )
	{
		Ms--;
		asm ("nop");
	}
}




TKernel::TKernel()
{
	
	//	read base addresses
	{
		uint32_t Data[2];
		auto ResponseSize = TMailbox::SetProperty( TMailbox::TTag::GetCpuBaseAddress, TMailbox::TChannel::Gpu, Data );
		mCpuMemoryBase = Data[0];
		mCpuMemorySize = Data[1];
		if ( ResponseSize != 2 )
		{
			mCpuMemoryBase = 0x0bad3000;
			mCpuMemorySize = ResponseSize;
		}
	}
	{
		uint32_t Data[2];
		auto ResponseSize = TMailbox::SetProperty( TMailbox::TTag::GetGpuBaseAddress, TMailbox::TChannel::Gpu, Data );
		mGpuMemoryBase = Data[0];
		mGpuMemorySize = Data[1];
		if ( ResponseSize != 2 )
		{
			mCpuMemoryBase = 0x0bad4000;
			mCpuMemorySize = ResponseSize;
		}
	}
	
	mHeap = THeap( (uint8_t*)mCpuMemoryBase, mCpuMemorySize );
	mDebugLogger = Debug::TLogger( mHeap );
	
	mPrintFunc = [&](const char* String)
	{
		mDebugLogger.Push( String );
	};
	gPrintFunc = &mPrintFunc;
	Debug::Print("Setup debug/kernel print");
	
	/*
	 //	terminal debug
	 uart_init();
	 hexstring(0x12345678);
	 hexstring(GETPC());
	 
	 //	set clock speed
	 timer_init();
	 
	 //	enable level 1 cache (faster!)
	 //	https://www.raspberrypi.org/forums/viewtopic.php?t=16851
	 start_l1cache();
	 */
	/*
	 uint32_t controlRegister;
	 asm volatile ("MRC p15, 0, %0, c1, c0, 0" : "=r" (controlRegister));
	 controlRegister |= 0x1800;
	 asm volatile ("MCR p15, 0, %0, c1, c0, 0" :: "r" (controlRegister));
	 */
	
	//	https://github.com/dwelch67/raspberrypi/tree/master/twain
	/*
	 //	enable data cache in program space
	 if(add_one(0x00000000,0x0000|8|4)) return(1);
	 if(add_one(0x00100000,0x0000|8|4)) return(1);
	 if(add_one(0x00200000,0x0000|8|4)) return(1);
	 
	 //	disale data cache in program space
	 if(add_one(0x00000000,0x0000)) return(1);
	 if(add_one(0x00100000,0x0000)) return(1);
	 if(add_one(0x00200000,0x0000)) return(1);
	 */
	
	
	//#define MMU
#ifdef MMU
	/*
	 for(nextfree=0;nextfree<TOP_LEVEL_WORDS;nextfree++)
	 PUT32(MMUTABLEBASE+(nextfree<<2),0);
	 //nextfree=TOP_LEVEL_WORDS;
	 */
	//ram used by the stack and the program
	mmu_section(0x00000000,0x0000|8|4);
	mmu_section(0x00100000,0x0000|8|4);
	mmu_section(0x00200000,0x0000|8|4);
	
	//Memory mapped I/O used by the uart, etc, not cached
	mmu_section(0x20000000,0x0000);
	mmu_section(0x20100000,0x0000);
	mmu_section(0x20200000,0x0000);
	
	//	turn off mmu for framebuffer
	mmu_section(0x48000000,0x0000);
	mmu_section(0x48100000,0x0000);
	mmu_section(0x48200000,0x0000);
	//invalidate_tlbs();
	
	//start_mmu(MMUTABLEBASE,0x00000001|0x1000|0x0004);
	/*
	 #define EnableDCache	false
	 if ( EnableDCache )
	 start_mmu(MMUTABLEBASE,0x00800005);
	 else
	 start_mmu(MMUTABLEBASE,0x00800001);
	 */
	
	
#endif
}

