#include "Kernel.h"
#include "Sprites.h"
#include "Blitter.h"


uint32_t TKernel::mCpuMemoryBase = 0xbad0bad1;
uint32_t TKernel::mCpuMemorySize = 0xbad2bad3;
uint32_t TKernel::mGpuMemoryBase = 0xbad4bad5;
uint32_t TKernel::mGpuMemorySize = 0xbad6bad7;

#define ENABLE_DRAWING

#define CHECKERBOARD_SIZE		16
#define CHECKERBOARD_COLOURA	RGBA( 247,191,9,255 )
#define CHECKERBOARD_COLOURB	RGBA( 249,221,127,255 )
// = RGBA( 56,185,255,255 );
//Colours[1] = RGBA( 199,214,221,255 );

//	gr: if I don't stop thread 0, it hangs after 22 iterations
//#define RESET_THREAD0
#define STOP_THREAD0
#define RESET_THREAD1
//#define STOP_THREAD1
#define RESET_THREAD_ON_ERROR

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

// 2  outer corner
// 4
// 6
// 8  TX out
// 10 RX in

// The raspberry pi firmware at the time this was written defaults
// loading at address 0x8000.  Although this bootloader could easily
// load at 0x0000, it loads at 0x8000 so that the same binaries built
// for the SD card work with this bootloader.  Change the ARMBASE
// below to use a different location.

//#define DRAW_RED
//#define DRAW_GREEN
#define DRAW_BLUE

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


#define V3D_IDENT0_MAGICNUMBER	0x02443356	//	2V3D

#define V3D_IDENT0  		(0x000) // V3D Identification 0 (V3D block identity)
#define V3D_BFC     0x134
#define V3D_CT0CS   0x100 // Control List Executor Thread 0 Control and Status.
#define V3D_CT1CS   0x104 // Control List Executor Thread 1 Control and Status.
#define V3D_CT0EA   0x108 // Control List Executor Thread 0 End Address.
#define V3D_CT1EA   0x10c // Control List Executor Thread 1 End Address.
#define V3D_CT0CA   0x110 // Control List Executor Thread 0 Current Address.
#define V3D_CT1CA   0x114 // Control List Executor Thread 1 Current Address.
#define V3D_CT00RA0 0x118 // Control List Executor Thread 0 Return Address.
#define V3D_CT01RA0 0x11c // Control List Executor Thread 1 Return Address.
#define V3D_CT0LC   0x120 // Control List Executor Thread 0 List Counter
#define V3D_CT1LC   0x124 // Control List Executor Thread 1 List Counter
#define V3D_CT0PC   0x128 // Control List Executor Thread 0 Primitive List Counter
#define V3D_CT1PC   0x12c // Control List Executor Thread 1 Primitive List Counter

#define V3D_ERRSTAT 0xf20 // Miscellaneous Error Signals (VPM, VDW, VCD, VCM, L2C)

//#define PERPIPHERAL_BASE	0x3F000000
//#define V3D_BASE	0xC00000	//	; V3D Base Address ($20C00000 PHYSICAL, $7EC00000 BUS)

//	gr: if I use 3F... nop execution doesnt finish...
#define V3D_BASE_ADDRESS	(0x20000000 | 0xc00000)	//	pi1 <-- gr: ident is only correct on this
//#define V3D_BASE_ADDRESS	(0x3F000000 | 0xc00000)	//	pi2


typedef uint32_t TTileBin;

volatile uint32_t* GetV3dReg(int Register)
{
	uint32_t Addr = V3D_BASE_ADDRESS;
	Addr += Register;
	return (volatile uint32_t*)Addr;
}

uint32_t ReadV3dReg(int Register)
{
	return *GetV3dReg(Register);
}


#define MALLOCBASE   0x00030000

//#define MMUTABLEBASE 0x00100000
//#define MMUTABLEBASE 0x00200000
#define MMUTABLEBASE 0x00004000	//	from pizero code	https://github.com/dwelch67/raspberrypi/blob/ec6b9f1b6a2d38d56f0a4a43683db1be01280ae2/boards/pizero/mmu/notmain.c


#define MMUTABLESIZE (0x8000)
#define MMUTABLEMASK ((MMUTABLESIZE-1)>>2)

#define TOP_LEVEL_WORDS (1<<((31-20)+1))
#define COARSE_TABLE_WORDS (1<<((19-12)+1))
#define SMALL_TABLE_WORDS (1<<((11-0)+1))

#define SIMPLE_SECTION



void addbyte(uint8_t **list, uint8_t d) {
	*((*list)++) = d;
}
/*
void addshort(uint8_t **list, uint16_t d) {
	*((*list)++) = (d >> 8) & 0xff;
	*((*list)++) = (d)  & 0xff;
}

void addword(uint8_t **list, uint32_t d) {
	*((*list)++) = (d >> 24) & 0xff;
	*((*list)++) = (d >> 16)  & 0xff;
	*((*list)++) = (d >> 8) & 0xff;
	*((*list)++) = (d >> 0) & 0xff;
}
*/
void addshort(uint8_t **list, uint16_t d) {
	*((*list)++) = (d) & 0xff;
	*((*list)++) = (d >> 8)  & 0xff;
}

void addword(uint8_t **list, uint32_t d) {
	*((*list)++) = (d) & 0xff;
	*((*list)++) = (d >> 8)  & 0xff;
	*((*list)++) = (d >> 16) & 0xff;
	*((*list)++) = (d >> 24) & 0xff;
}

void addfloat(uint8_t **list, float f) {
	uint32_t d = *((uint32_t *)&f);
	*((*list)++) = (d) & 0xff;
	*((*list)++) = (d >> 8)  & 0xff;
	*((*list)++) = (d >> 16) & 0xff;
	*((*list)++) = (d >> 24) & 0xff;
}


uint16_t EndianSwap16(uint16_t v)
{
	uint16_t x = 0;
	uint8_t* p = reinterpret_cast<uint8_t*>( &x );
	addshort( &p, v );
	return x;
}

uint32_t EndianSwap32(uint32_t v)
{
	uint32_t x = 0;
	uint8_t* p = reinterpret_cast<uint8_t*>( &x );
	addword( &p, v );
	return x;
}

uint32_t EndianSwapFloat(float v)
{
	uint32_t x = 0;
	uint8_t* p = reinterpret_cast<uint8_t*>( &x );
	addfloat( &p, v );
	return x;
}

unsigned int nextfree;

unsigned int next_coarse_offset ( unsigned int x )
{
	unsigned int mask;
	
	mask=(~0)<<(10-2);
	mask=~mask;
	while(x&mask) x++; //lazy brute force
	return(x);
}

//	https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=67970
typedef struct
{
	unsigned int read;		//	0
	unsigned int unused1;
	unsigned int unused2;
	unsigned int unused3;
	unsigned int poll;		//	16
	unsigned int sender;	//	20
	unsigned int status;	//	24
	unsigned int configuration;	//	28
	unsigned int write;		//	32
} Mailbox;

static volatile Mailbox* const MAILBOX0 = (Mailbox*)0x2000b880;


enum class TGpuThread : uint32_t;


class TDisplay : public TBlitter
{
public:
	TDisplay(int Width,int Height,bool EnableGpu);
	
	void		SetResolution(uint32_t Width,uint32_t Height);


	void		SetupGpu();
	template<typename LAMBDA>
	void		GpuExecute(size_t ProgramSizeAlloc,LAMBDA& SetupProgram,TGpuThread GpuThread);
	void		GpuNopTest();
	
	bool		SetupBinControl(void* ProgramMemory,TTileBin* TileBinMemory,size_t TileBinMemorySize,void* TileStateMemory);
	uint8_t*	SetupRenderControlProgram(uint8_t* Program,TTileBin* TileBinMemory);
	bool		SetupRenderControl(void* ProgramMemory,TTileBin* TileBinMemory);
	

	uint8_t		GetTileWidth() const	{	return (mWidth%64)/64;	}	//	round down so we don't overflow pixel buffer
	uint8_t		GetTileHeight() const	{	return (mHeight%64)/64;	}	//	round down so we don't overflow pixel buffer

	TCanvas<uint32_t>	LockCanvas();
	
public:
	uint32_t	mClearColour;
	uint32_t	mWidth;
	uint32_t	mHeight;
	uint32_t*	mScreenBuffer;
};

void DrawScreen(TDisplay& Display,int Tick);


class TGpuMemory;



class TGpuMemory
{
public:
	TGpuMemory(uint32_t Size,bool Lock);
	
	//	gr: making this explicit instead of in destructor as I can't debug to make sure any RValue copy is working correctly
	void 		Free();

	size_t		GetSize() const				{	return mSize;	}
	uint8_t*	GetCpuAddress() const		{	return TKernel::GetCpuAddress(mLockedAddress);	}
	uint8_t*	GetGpuAddress() const		{	return TKernel::GetGpuAddress(mLockedAddress);	}
	uint8_t*	GetAllocatedAddress() const	{	return mLockedAddress;	}
	
	uint8_t*	Lock();
	bool		Unlock();
	
public:
	uint32_t	mHandle;
	size_t		mSize;
	uint8_t*	mLockedAddress;
};


enum class TGpuMemFlags : uint32_t
{
	Discardable		= 1<<0,	//	can be resized to 0 at any time. Use for cached data
	Normal			= 0,	//	normal allocating alias. Don't use from ARM
	Direct			= 1<<2,	//	0xC alias uncached
	Coherent		= 1<<3,	//	same as 2<<2	0x8 alias. Non-allocating in L2 but coherent
	L1NonAllocating	= Direct | Coherent,	//	Allocating in L2
	ZeroMemory		= 1<<4,	//	initialise all zero
	NoInit			= 1<<5,	//	don't initialise (default is initialise to all ones)
	HintPermalock	= 1<<6,	//	Likely to be locked for long periods of time
};

constexpr enum TGpuMemFlags operator |(const enum TGpuMemFlags selfValue,const enum TGpuMemFlags inValue)
{
	return (enum TGpuMemFlags)(uint32_t(selfValue) | uint32_t(inValue));
}

enum class TGpuThread : uint32_t
{
	Thread0		= 0,
	Thread1		= 1,
};



namespace TMailbox
{
	enum class TChannel	: uint32_t;
	enum class TTag		: uint32_t;
	
	//	returns amount of data returned
	template<size_t PAYLOADSIZE>
	int			SetProperty(TTag Tag,TChannel Channel,uint32_t (& Payload)[PAYLOADSIZE],bool Read=true);
}



//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
enum class TMailbox::TChannel : uint32_t
{
	One		= 1,
	Gpu		= 8	//	Arm communicating with video controller (ArmToVc)
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

	SetClockRate	= 0x00038002
};
		


//	some more references for magic nmbers
//	https://www.raspberrypi.org/forums/viewtopic.php?f=29&t=65596
//	http://magicsmoke.co.za/?p=284
#define MAILBOX_STATUS_BUSY		0x80000000
#define MAILBOX_STATUS_EMPTY	0x40000000


void MailboxWrite(volatile void* Data,TMailbox::TChannel Channel)
{
	Data = TKernel::GetGpuAddress( Data );
	uint32_t BufferAddress = (uint32_t)Data;
	
	auto ChannelBitMask = (1<<4)-1;
	if ( BufferAddress & ChannelBitMask )
	{
		//	throw
		BufferAddress &= ~ChannelBitMask;
	}

	
	//	and bit 1 to say... we're writing it? or because it's the channel?..
	//	odd that it'll overwrite the display widht, but maybe that has to be aligned or something anyway
	//	4bits per channel
	//	gr: could be AllocateBuffer command; http://magicsmoke.co.za/?p=284
	//	gr: why arent we setting all these tags? http://magicsmoke.co.za/?p=284
	BufferAddress |= static_cast<uint32_t>(Channel);

    while(1)
    {
		unsigned int mailbox = 0x2000B880;
		//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
		//	0x80000000: request successful
		//	0x80000001: error parsing request buffer (partial response)
		uint32_t MailboxResponse = GET32(mailbox+24);
		//uint32_t MailboxResponse = MAILBOX0->sender;
		//uint32_t MailboxResponse = GET32( (uint32_t)&MAILBOX0->sender );
		
		//	break when we... have NO response waiting??
		bool StatusBusy = bool_cast(MailboxResponse & MAILBOX_STATUS_BUSY);
		if( !StatusBusy )
			break;
    }
	
	//	gr: where is
	
	//	gr: I bet this is configuration
	MAILBOX0->write = BufferAddress;
}

uint32_t MailboxRead(TMailbox::TChannel Channel)
{
	auto ChannelBitMask = (1<<4)-1;
    unsigned int mailbox = 0x2000B880;
    while(1)
    {
        while(1)
        {
			//	gr: sender, not status, suggests this struct may be off...
			//uint32_t MailboxResponse = GET32( (uint32_t)&MAILBOX0->sender );
			uint32_t MailboxResponse = GET32( mailbox+24 );
			bool StatusEmpty = bool_cast(MailboxResponse & MAILBOX_STATUS_EMPTY);
            if ( !StatusEmpty )
				break;
        }

		//	gr: switching this to MAILBOX0->read doesn't work :/
		auto Response = GET32( mailbox+0 );
		//auto Response = GET32( (uint32_t)&MAILBOX0->read );
		auto ResponseChannel = static_cast<TMailbox::TChannel>( Response & ChannelBitMask );
        if ( ResponseChannel == Channel )
		{
			Response &= ~ChannelBitMask;
			return Response;
		}
    }
	
	//	throw
	return -1;
}

bool MailboxEnableQpu(bool Enable=true)
{
	uint32_t Data[7] __attribute__ ((aligned(16)));
	static_assert( sizeof(Data) == 4*7, "sizeof");
	
	//	header
	Data[0] = sizeof(Data);
	Data[1] = 0x00000000;	//	process request (0 = writing mailbox)
	Data[2] = 0x00030012;	//	the tag id
	Data[3] = 4;			//	size of the buffer
	Data[4] = 4;			//	size of the data
	
	//	buffer here
	Data[5] = Enable ? 1 : 0;
	
	//	footer
	Data[6] = 0x00000000;	// end tag
	
	//	gr: channel 1 doesn't boot properly (screen resets?)
	//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
	//	8 is supposed to be ARM to VC
	auto Channel = TMailbox::TChannel::Gpu;
	MailboxWrite( Data, Channel );
	//	wait for it to finish
	MailboxRead( Channel );
	
	//mbox_property(file_desc, p);
	//return p[5];
	auto Enabled = (Data[5] == 1);
	return Enabled;
}

	
TCanvas<uint32_t> TDisplay::LockCanvas()
{
	TCanvas<uint32_t> Canvas( nullptr );
	Canvas.mPixels = mScreenBuffer;
	Canvas.mWidth = mWidth;
	Canvas.mHeight = mHeight;
	return Canvas;
}


void TDisplay::SetResolution(uint32_t Width,uint32_t Height)
{
	uint32_t Data[2];
	Data[0] = Width;
	Data[1] = Height;
	TMailbox::SetProperty( TMailbox::TTag::SetResolution, TMailbox::TChannel::Gpu, Data );
	/*
	uint32_t Data[8] __attribute__ ((aligned(16)));
	
	//	header
	Data[0] = sizeof(Data);
	Data[1] = 0x00000000;	//	process request (0 = writing mailbox)
	Data[2] = TAG_SETRESOLUTION;
	Data[3] = 8;			//	size of the buffer
	Data[4] = 8;			//	size of the data
	
	//	buffer here
	Data[5] = Width;
	Data[6] = Height;

	//	footer
	Data[7] = 0x00000000;	// end tag
	
	//	gr: channel 1 doesn't boot properly (screen resets?)
	//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
	//	8 is supposed to be ARM to VC
	auto Channel = TMailbox::TChannel::Gpu;
	
	MailboxWrite( Data, Channel );
	MailboxRead( Channel );
	 */
}

bool mmu_section(unsigned int add,unsigned int flags)
{
	
#if defined(SIMPLE_SECTION)
	unsigned int ra;
	unsigned int rb;
	unsigned int rc;
	
	//bits 31:20 index into the top level table
	ra=add>>20;
	rb=MMUTABLEBASE|(ra<<2);
	rc=(ra<<20)|flags|2;
	PUT32(rb,rc);
	return true;

#else

	//unsigned int add_one ( unsigned int add, unsigned int flags )
	unsigned int ra;
	unsigned int rb;
	unsigned int rc;
	
	ra=add>>20;
	rc=MMUTABLEBASE+(ra<<2);
	rb=GET32(rc);
	if(rb)
	{
		//printf("Address %08X already allocated\n",add);
		hexstring(add);
		hexstring(rc);
		hexstring(rb);
		hexstring(0xBADADD);
		return false;
	}
	add=ra<<20;
	
	rb=next_coarse_offset(nextfree);
	rc=rb+COARSE_TABLE_WORDS;
	if(rc>=MMUTABLESIZE)
	{
		//printf("Not enough room\n");
		hexstring(0xBAD);
		return false;
	}
	nextfree=rc;
	//use course page table pointer on top level table
	PUT32(MMUTABLEBASE+(ra<<2),(MMUTABLEBASE+(rb<<2))|0x00000001);
	//fill in the course page table. with small entries
	for(ra=0;ra<COARSE_TABLE_WORDS;ra++)
	{
		PUT32(MMUTABLEBASE+(rb<<2)+(ra<<2),(add+(ra<<12))|0x00000032|flags);
	}
	return true;
#endif
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


static_assert(sizeof(uint32_t*) == sizeof(uint32_t), "Expecting pointers to be 32bit for mailbox data");

struct TDisplayInfo
{
	//	https://github.com/raspberrypi/firmware/wiki/Mailbox-framebuffer-interface
	uint32_t	mFrameWidth;
	uint32_t	mFrameHeight;
	uint32_t	mVirtualWidth;
	uint32_t	mVirtualHeight;
	uint32_t	mPitch;			//	output only
	uint32_t	mBitDepth;
	uint32_t	mScrollX;
	uint32_t	mScrollY;
	uint32_t*	mPixelBuffer;		//	output only
	uint32_t	mPixelBufferSize;	//	output only (bytes?)
	
} __attribute__ ((aligned(16)));

TDisplayInfo DisplayInfo;

void Sleep(int Ms)
{
	//	random number, tis loop is basicaly "sleep for x ticks" so 250/1000mhz of nops is what we need?
	Ms *= 1 * 250 * 100;
	while ( Ms > 0 )
	{
		Ms--;
		asm ("nop");
	}
}


TDisplay::TDisplay(int Width,int Height,bool EnableGpu) :
	mScreenBuffer	( nullptr ),
	mWidth			( Width ),
	mHeight			( Height ),
	mClearColour	( RGBA( 255,0,255,255 ) ),
	TBlitter		( [this]{	return this->LockCanvas();	} )
{
	
	auto ScrollX = 0;
	auto ScrollY = 0;
	auto BitDepth = 32;
	auto GpuPitch = 0;

	
	DisplayInfo.mFrameWidth = mWidth;
	DisplayInfo.mFrameHeight = mHeight;
	DisplayInfo.mVirtualWidth = mWidth;
	DisplayInfo.mVirtualHeight = mHeight;
	DisplayInfo.mPitch = GpuPitch;
	DisplayInfo.mBitDepth = BitDepth;
	DisplayInfo.mScrollX = ScrollX;
	DisplayInfo.mScrollY = ScrollY;
	DisplayInfo.mPixelBuffer = nullptr;
	DisplayInfo.mPixelBufferSize = 0;
	
	
	//	send this data at this address to mailbox
	auto Channel = TMailbox::TChannel::One;
	MailboxWrite( &DisplayInfo,Channel);
	//	? block until ready
	MailboxRead( Channel );

	//	read new contents of struct
	mScreenBuffer = TKernel::GetCpuAddress( DisplayInfo.mPixelBuffer );
	//	todo: if addr=0, loop
	
	FillPixelsCheckerBoard(CHECKERBOARD_SIZE,CHECKERBOARD_COLOURA,CHECKERBOARD_COLOURB);
	
	//	testing
	//Sleep(2000);
	//SetResolution( 600, 600 );

	if ( EnableGpu )
	{
		Sleep(100);
	
	
		SetupGpu();
	
		Sleep(100);
		
		//	gr: without this... error... not sure why this affects things
		//	see if we have control again or if setup is stuck
		//DrawScreen( *this, 100 );
		
		Sleep(100);
	}
	
	//	all from kernel.asm
	//	lookup
	
	//	assembler has full struct info:
	//	size
	//	request/response code
	//	Set_Physical_Display <--tag
	/*
	 align 16
	 FB_STRUCT: ; Mailbox Property Interface Buffer Structure
  dw FB_STRUCT_END - FB_STRUCT ; Buffer Size In Bytes (Including The Header Values, The End Tag And Padding)
  dw $00000000 ; Buffer Request/Response Code
	 ; Request Codes: $00000000 Process Request Response Codes: $80000000 Request Successful, $80000001 Partial Response
	 ; Sequence Of Concatenated Tags
  dw Set_Physical_Display ; Tag Identifier
  dw $00000008 ; Value Buffer Size In Bytes
  dw $00000008 ; 1 bit (MSB) Request/Response Indicator (0=Request, 1=Response), 31 bits (LSB) Value Length In Bytes
  dw SCREEN_X ; Value Buffer
  dw SCREEN_Y ; Value Buffer
	 
  dw Set_Virtual_Buffer ; Tag Identifier
  dw $00000008 ; Value Buffer Size In Bytes
  dw $00000008 ; 1 bit (MSB) Request/Response Indicator (0=Request, 1=Response), 31 bits (LSB) Value Length In Bytes
  dw SCREEN_X ; Value Buffer
  dw SCREEN_Y ; Value Buffer
	 
  dw Set_Depth ; Tag Identifier
  dw $00000004 ; Value Buffer Size In Bytes
  dw $00000004 ; 1 bit (MSB) Request/Response Indicator (0=Request, 1=Response), 31 bits (LSB) Value Length In Bytes
  dw BITS_PER_PIXEL ; Value Buffer
	 
  dw Set_Virtual_Offset ; Tag Identifier
  dw $00000008 ; Value Buffer Size In Bytes
  dw $00000008 ; 1 bit (MSB) Request/Response Indicator (0=Request, 1=Response), 31 bits (LSB) Value Length In Bytes
	 FB_OFFSET_X:
  dw 0 ; Value Buffer
	 FB_OFFSET_Y:
  dw 0 ; Value Buffer
	 
  dw Allocate_Buffer ; Tag Identifier
  dw $00000008 ; Value Buffer Size In Bytes
  dw $00000008 ; 1 bit (MSB) Request/Response Indicator (0=Request, 1=Response), 31 bits (LSB) Value Length In Bytes
	 FB_POINTER:
  dw 0 ; Value Buffer
  dw 0 ; Value Buffer
	 
	 dw $00000000 ; $0 (End Tag)
	 FB_STRUCT_END:
	 
	 
	 */
	
	/*
	 ; Run Binning Control List (Thread 0)
	 imm32 r0,PERIPHERAL_BASE + V3D_BASE ; Load V3D Base Address
	 imm32 r1,CONTROL_LIST_BIN_STRUCT ; Store Control List Executor Binning Thread 0 Current Address
	 str r1,[r0,V3D_CT0CA]
	 imm32 r1,CONTROL_LIST_BIN_END ; Store Control List Executor Binning Thread 0 End Address
	 str r1,[r0,V3D_CT0EA] ; When End Address Is Stored Control List Thread Executes
	 */
	/*
	 uint32_t* v3dBase = PERPIPHERAL_BASE | V3D_BASE;
	 
	 
	 //	gr: this "struct" makes me think its a execute-code start & end address
	 //	gr: and it is!	https://github.com/phire/hackdriver/blob/master/v3d.h
	 v3dBase[V3D_CT0CA] = CONTROL_LIST_BIN_STRUCT;
	 v3dBase[V3D_CT0CE] = CONTROL_LIST_BIN_END;
	 /*
	 WaitBinControlList: ; Wait For Control List To Execute
  ldr r1,[r0,V3D_BFC] ; Load Flush Count
  tst r1,1 ; Test IF PTB Has Flushed All Tile Lists To Memory
  beq WaitBinControlList
	 */
	
	/*
	 
	 ; Run Rendering Control List (Thread 1)
	 imm32 r1,CONTROL_LIST_RENDER_STRUCT ; Store Control List Executor Rendering Thread 1 Current Address
	 str r1,[r0,V3D_CT1CA]
	 imm32 r1,CONTROL_LIST_RENDER_END ; Store Control List Executor Rendering Thread 1 End Address
	 str r1,[r0,V3D_CT1EA] ; When End Address Is Stored Control List Thread Executes
	 */
	/*
	 //	thread control start
	 v3dBase[V3D_CT1CA] = CONTROL_LIST_RENDER_STRUCT;
	 //	thread control end
	 v3dBase[V3D_CT1CE] = CONTROL_LIST_RENDER_END;
	 
	 /*
	 https://rpiplayground.wordpress.com/2014/05/03/hacking-the-gpu-for-fun-and-profit-pt-1/
	 Next we do some pointer arithmetic to set the structure fields to point to the proper VC addresses.  To execute a QPU program through the mailbox interface, we pass an array of message structures that contain a pointer to the uniforms to bind to the QPU program and then a pointer to the address of the QPU code to execute:
	 */
}

/*
align 4
CONTROL_LIST_BIN_STRUCT: ; Control List Of Concatenated Control Records & Data Structure (Binning Mode Thread 0)
Tile_Binning_Mode_Configuration BIN_ADDRESS, $2000, BIN_BASE, 10, 8, Auto_Initialise_Tile_State_Data_Array ; Tile Binning Mode Configuration (B) (Address, Size, Base Address, Tile Width, Tile Height, Data)
Start_Tile_Binning ; Start Tile Binning (Advances State Counter So That Initial State Items Actually Go Into Tile Lists) (B)

Clip_Window 0, 0, SCREEN_X, SCREEN_Y ; Clip Window
Configuration_Bits Enable_Forward_Facing_Primitive + Enable_Reverse_Facing_Primitive, Early_Z_Updates_Enable ; Configuration Bits
Viewport_Offset 0, 0 ; Viewport Offset
NV_Shader_State NV_SHADER_STATE_RECORD ; NV Shader State (No Vertex Shading)
Indexed_Primitive_List Mode_Triangles + Index_Type_8, 3, VERTEX_LIST, 2 ; Indexed Primitive List (OpenGL)
Flush ; Flush (Add Return-From-Sub-List To Tile Lists & Then Flush Tile Lists To Memory) (B)
CONTROL_LIST_BIN_END:

*/

#define VC_INSTRUCTION_NOP	1
#define VC_INSTRUCTION_HALT	0
	
void TDisplay::SetupGpu()
{
	//	set clock rate
	//	gr: asm seems wrong..
	//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
	{
		auto CLK_V3D_ID = 5;
		uint32_t Data[3];
		Data[0] = CLK_V3D_ID;
		Data[1] = 250*1000*1000;
		Data[2] = 1;	//	skip turbo
		TMailbox::SetProperty( TMailbox::TTag::SetClockRate, TMailbox::TChannel::Gpu, Data );
	}
	
	
	MailboxEnableQpu(true);
	
	auto Magic = ReadV3dReg(V3D_IDENT0);
	if ( Magic != V3D_IDENT0_MAGICNUMBER )
	{
		FillPixels( RGBA(255,255,255,255) );
		return;
		//throw;
	}
	
	
	
	//GpuNopTest();
/*
	if ( !SetupBinControl() )
	{
		FillPixels( RGBA(255,0,0,255) );
		return;
	}

	if ( !SetupRenderControl() )
	{
		//FillPixels( RGBA(255,0,255,255) );
		return;
	}
	*/
}

template<typename LAMBDA>
void TDisplay::GpuExecute(size_t ProgramSizeAlloc,LAMBDA& SetupProgram,TGpuThread GpuThread)
{
	TGpuMemory MemoryAlloc( ProgramSizeAlloc, true );
	auto* Memory = MemoryAlloc.GetCpuAddress();
	
	auto ExecuteSize = SetupProgram( Memory );
	
	uint32_t RegStarts[2] =	{	V3D_CT0CA,	V3D_CT1CA	};
	uint32_t RegEnds[2] = 	{	V3D_CT0EA,	V3D_CT1EA	};
	uint32_t RegStatuss[2] =	{	V3D_CT0CS,	V3D_CT1CS	};
	
	auto RegStart = RegStarts[static_cast<uint32_t>(GpuThread)];
	auto RegEnd = RegEnds[static_cast<uint32_t>(GpuThread)];
	auto RegStatus = RegStatuss[static_cast<uint32_t>(GpuThread)];
	
	//	tell thread0 to start at our instructions
	*GetV3dReg(RegStart) = TKernel::GetGpuAddress32(Memory);
	//	set end address, also starts execution
	*GetV3dReg(RegEnd) = TKernel::GetGpuAddress32(Memory) + ExecuteSize;
	

	//	Wait a second to be sure the contorl list execution has finished
	while(*GetV3dReg(RegStatus) & 0x20)
	{
	}
	
	
	MemoryAlloc.Free();
}


void TDisplay::GpuNopTest()
{
	auto Size = 256;
	auto ProgramSetup = [=] (uint8_t* Program)-> size_t
	{
		//	Now we construct our control list.
		//	255 nops, with a halt somewhere in the middle
		for ( auto i=0;	i<Size;	i++ )
		{
			Program[i] = VC_INSTRUCTION_NOP;
		}
		Program[0xbb] = VC_INSTRUCTION_HALT;
		return Size;
	};
	GpuExecute( Size, ProgramSetup, TGpuThread::Thread0 );
}
	
#define MAX_TILE_WIDTH		40
#define MAX_TILE_HEIGHT		40
#define TILE_STRUCT_SIZE	48
#define AUTO_INIT_TILE_STATE_CMD	(1<<2)


#define TILE_BIN_BLOCK_SIZE	(32)	//	gr; if not 32, there's flags for 64,128,256
	/*
uint32_t gTileBin[MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_BIN_BLOCK_SIZE]  __attribute__ ((aligned(16)));
uint8_t gTileState[MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_STRUCT_SIZE]  __attribute__ ((aligned(16)));
//static volatile uint32_t* TileBin = (uint32_t*)0x00400000;
//static volatile uint8_t* TileState = (uint8_t*)00500000;
uint8_t gProgram0[4096*4]  __attribute__ ((aligned(16)));
uint8_t gProgram1[4096*4]  __attribute__ ((aligned(16)));
	*/
//	128bit align
uint8_t NV_SHADER_STATE_RECORD[200]  __attribute__ ((aligned(16)));
uint32_t FRAGMENT_SHADER_CODE[12*4] __attribute__ ((aligned(16)))=
	{
		//	Fill Color Shader
		EndianSwap32( 0x009E7000 ),
		EndianSwap32( 0x100009E7 ),	//	nop; nop; nop
		
		EndianSwap32( 0xFFFFFFFF ),	//	RGBA White
		EndianSwap32( 0xE0020BA7 ),	//	ldi tlbc, $FFFFFFFF
		EndianSwap32( 0x009E7000 ),
		EndianSwap32( 0x500009E7 ),	//	nop; nop; sbdone
		EndianSwap32( 0x009E7000 ),
		EndianSwap32( 0x300009E7 ),	//	nop; nop; thrend
		
		EndianSwap32( 0x009E7000 ),
		EndianSwap32( 0x100009E7 ),	//	nop; nop; nop
		EndianSwap32( 0x009E7000 ),
		EndianSwap32( 0x100009E7 ),	//	nop; nop; nop
	};
	
struct TVertex
{
		uint16_t	x;	//	12.4 Fixed Point
		uint16_t	y;	//	12.4 Fixed Point
		uint32_t	z;	//	float
		uint32_t	w;	//	float
};
#define VERTEX_COUNT	3
TVertex VERTEX_DATA[VERTEX_COUNT] __attribute__ ((aligned(16))) =
{
	{	EndianSwap16(10 * 16),	EndianSwap16(10 * 16),	EndianSwapFloat(1.0),	EndianSwapFloat(1.0)	},
	{	EndianSwap16(32 * 16),	EndianSwap16(448 * 16),	EndianSwapFloat(1.0),	EndianSwapFloat(1.0)	},
	{	EndianSwap16(608 * 16),	EndianSwap16(448 * 16),	EndianSwapFloat(1.0),	EndianSwapFloat(1.0)	},
};
	
uint8_t VERTEX_INDEXES[VERTEX_COUNT] __attribute__ ((aligned(16))) =
	{
		0,1,2
	};
	
uint8_t* SetupVertexShaderState()
{
	uint8_t* ShaderState = NV_SHADER_STATE_RECORD;

	//	Flag Bits: 0 = Fragment Shader Is Single Threaded, 1 = Point Size Included In Shaded Vertex Data, 2 = Enable Clipping, 3 = Clip Coordinates Header Included In Shaded Vertex Data
	uint8_t Flags = 0;
	uint8_t VertexDataStride = 3 * 4;
	uint8_t UniformCount = 0;
	uint8_t VaryingsCount = 0;
	void* FragShaderUniforms = nullptr;
	void* VertexData = VERTEX_DATA;
	void* FragShader = FRAGMENT_SHADER_CODE;
	
	addbyte( &ShaderState, Flags );
	addbyte( &ShaderState, VertexDataStride );
	addbyte( &ShaderState, UniformCount );
	addbyte( &ShaderState, VaryingsCount );
	addword( &ShaderState, (uint32_t)FragShader );
	addword( &ShaderState, (uint32_t)FragShaderUniforms );
	addword( &ShaderState, (uint32_t)VertexData );
	
	return NV_SHADER_STATE_RECORD;
}

	
#define STATUS_ERROR		(1<<3)
#define STATUS_RUN			(1<<5)
#define STATUS_RUNSUBMODE	(1<<4)
	

enum ThreadState
{
	Running,
	Stalled,
	Finished,
	Error,
};
	
	
ThreadState GetThreadState(int ThreadIndex)
{
	auto StatusReg = (ThreadIndex==0) ? V3D_CT0CS : V3D_CT1CS;
	
	auto Status = ReadV3dReg(StatusReg);
	if ( bool_cast(Status & STATUS_RUN) )
	{
		if ( bool_cast(Status & STATUS_RUNSUBMODE) )
			return Stalled;
		else
			return Running;
	}
	
	if ( bool_cast(Status & STATUS_ERROR) )
	{
		return Error;
	}
	
	return Finished;
}
	
//	returns false on error
bool WaitForThread(int ThreadIndex)
{
	while ( true )
	{
		auto State = GetThreadState( ThreadIndex );
		if ( State == Running )
			continue;
		
		return State == Finished;
	}
}

	
bool TDisplay::SetupBinControl(void* ProgramMemory,TTileBin* TileBinMemory,size_t TileBinMemorySize,void* TileStateMemory)
{
//#define ENABLE_TRIANGLES
	
	//	CONTROL_LIST_BIN_STRUCT
	auto* p = TKernel::GetCpuAddress((uint8_t*)ProgramMemory);
	
	/*
	DrawString( GetConsoleX(), GetConsoleY(), "TileBin*  = ");
	DrawHex( GetConsoleX(false), GetConsoleY(), (uint32_t)TileBinMemory );
	
	DrawString( GetConsoleX(), GetConsoleY(), "TileState*= ");
	DrawHex( GetConsoleX(false), GetConsoleY(), (uint32_t)TileStateMemory );
	
	DrawString( GetConsoleX(), GetConsoleY(), "VERTEX_INDEXES = ");
	DrawNumber( GetConsoleX(false), GetConsoleY(), VERTEX_INDEXES[0] );
	DrawString( GetConsoleX(false), GetConsoleY(), ",");
	DrawNumber( GetConsoleX(false), GetConsoleY(), VERTEX_INDEXES[1] );
	DrawString( GetConsoleX(false), GetConsoleY(), ",");
	DrawNumber( GetConsoleX(false), GetConsoleY(), VERTEX_INDEXES[2] );
	
	*/
	
	//	Tile_Binning_Mode_Configuration
	addbyte(&p, 112);
	addword(&p, (uint32_t)TileBinMemory );
	addword(&p, TileBinMemorySize );
	addword(&p, (uint32_t)TileStateMemory );
	addbyte(&p, GetTileWidth() );
	addbyte(&p, GetTileHeight() );
	uint8_t Flags = AUTO_INIT_TILE_STATE_CMD;
	addbyte(&p, Flags);
	
	
	//Start_Tile_Binning
	addbyte(&p, 6);
	
	//	Clip_Window - left, bottom, width, height
	addbyte(&p, 102);
	addshort(&p, 0 );
	addshort(&p, 0 );
	addshort(&p, mWidth );
	addshort(&p, mHeight );
	
#define Enable_Forward_Facing_Primitive	0x01	//	Configuration_Bits: Enable Forward Facing Primitive
#define Enable_Reverse_Facing_Primitive	0x02	//	Configuration_Bits: Enable Reverse Facing Primitive
#define Early_Z_Updates_Enable			0x02
#define Mode_Triangles					0x04	//	Indexed_Primitive_List: Primitive Mode = Triangles
#define Index_Type_8 					0x00	//	Indexed_Primitive_List: Index Type = 8-Bit
#define Index_Type_16					0x10	//	Indexed_Primitive_List: Index Type = 16-Bit
#if defined(ENABLE_TRIANGLES)
	//	Configuration_Bits Enable_Forward_Facing_Primitive + Enable_Reverse_Facing_Primitive, Early_Z_Updates_Enable ; Configuration Bits
	uint8_t Config[3];
	Config[0] = Enable_Forward_Facing_Primitive | Enable_Reverse_Facing_Primitive;
	Config[1] = 0x0;
	Config[2] = Early_Z_Updates_Enable;
	addbyte(&p, 0x60);
	addbyte(&p, Config[0]);
	addbyte(&p, Config[1]);
	addbyte(&p, Config[2]);
#endif
	
	//	wont render without
	//	Viewport_Offset
	addbyte(&p, 0x67);
	addshort(&p, 0);
	addshort(&p, 0);
	
#if defined(ENABLE_TRIANGLES)
	//	NV_Shader_State NV_SHADER_STATE_RECORD ; NV Shader State (No Vertex Shading)
	addbyte(&p, 0x41);
	auto* VertexShaderState = SetupVertexShaderState();
	addword(&p, (uint32_t)VertexShaderState );
	
	//	macro Indexed_Primitive_List data, length, address, maxindex { ; Control ID Code: Indexed Primitive List (OpenGL)
	uint8_t Mode = Mode_Triangles | Index_Type_8;
	uint32_t IndexCount = VERTEX_COUNT;
	uint32_t MaxIndex = IndexCount - 1;
	addbyte(&p, 0x20);
	addbyte(&p, Mode);
	addword(&p, IndexCount);
	addword(&p, (uint32_t)VERTEX_INDEXES );
	addword(&p, MaxIndex);
#endif
	//	Flush
	addbyte(&p, 0x4);	//	flush
	//addbyte(&p, 0x5);	//	flush all state
	addbyte(&p, 1);	//	nop
	addbyte(&p, 0);	//	halt

	

	
	auto* ProgramMemoryEnd = p;
	
#if defined(RESET_THREAD0)
	{
		//	reset thread...
		auto Status = *GetV3dReg(V3D_CT0CS);
		Status |= 1<<15;
		*GetV3dReg(V3D_CT0CS) = Status;
	}
#endif
	
#if defined(STOP_THREAD0)
	{
		//	stop thread...
		auto Status = *GetV3dReg(V3D_CT0CS);
		Status |= (1<<5);
		*GetV3dReg(V3D_CT0CS) = Status;
	}
#endif

	if ( !WaitForThread(0) )
		return false;

	//	explicit stop
	//*GetV3dReg(V3D_CT0CS) = 0x20;

	*GetV3dReg(V3D_CT0CA) = TKernel::GetGpuAddress32(ProgramMemory);
	*GetV3dReg(V3D_CT0EA) = TKernel::GetGpuAddress32(ProgramMemoryEnd);
	
	if ( !WaitForThread(0) )
		return false;

	//	explicit stop
	//*GetV3dReg(V3D_CT0CS) = 0x20;
/*
	if ( InitialFlushCount != 0 )
	{
		return false;
		FillPixels( RGBA(0,255,0,255) );
	}
	
	//	wait for flush count to reach 1!
	while ( true )
	{
		auto FlushCount = ReadV3dReg(V3D_BFC);
		if ( FlushCount == InitialFlushCount+1 )
			break;
		Sleep(1);
	}
 */
	return true;
}

	
	

uint8_t* TDisplay::SetupRenderControlProgram(uint8_t* Program,TTileBin* TileBinMemory)
{
	//uint32_t ClearZMask = 0xffff;
	//uint32_t ClearVGMask = 0xff;
	//uint32_t ClearFlags0 = ClearZMask | (ClearVGMask<<24);
	uint32_t ClearFlags0 = 0;
	uint8_t ClearStencil = 0;
	//Clear_ZS      = $00FFFFFF ; Clear_Colors: Clear ZS (UINT24)
	//Clear_VG_Mask = $FF000000 ; Clear_Colors: Clear VG Mask (UINT8)
	//Clear_Stencil = $FF ; Clear_Colors: Clear Stencil (UINT8)

	addbyte( &Program, 114 );
	//	gr: gotta do colour twice, or RGBA16
	addword( &Program, mClearColour );
	addword( &Program, mClearColour );
	addword( &Program, ClearFlags0 );
	addbyte( &Program, ClearStencil );
	
	//	Tile_Rendering_Mode_Configuration
#define Frame_Buffer_Color_Format_RGBA8888 0x4
	addbyte( &Program, 113 );
	addword( &Program, TKernel::GetCpuAddress32(mScreenBuffer) );
	addshort( &Program, mWidth );	//	controls row stride
	addshort( &Program, mHeight );
	addshort( &Program, Frame_Buffer_Color_Format_RGBA8888 );

	//	Tile_Coordinates
	addbyte( &Program, 115 );
	addbyte( &Program, 0 );
	addbyte( &Program, 0 );
	
	//	Store_Tile_Buffer_General
	//	gr:this seems to be for a dump sysem.. like capture
	uint32_t Flags0_32 = 0;	//	disable double buffer in dump
	uint16_t Flags33_48 = 0;
	addbyte( &Program, 28 );
	addword( &Program, Flags0_32 );
	addshort( &Program, Flags33_48 );
	
	//db $1C ; Control ID Code Byte: ID Code #28
	//dh data16 ; Control ID Data Record Short: (Bit 0..15)
	//dw address + data32 ; Control ID Data Record Word: Memory Base Address Of Frame/Tile Dump Buffer (In Multiples Of 16 Bytes) (Bit 20..47), Data Record (Bit 16..19)

	auto TileWidth = GetTileWidth();
	auto TileHeight = GetTileHeight();
	
	for ( unsigned ty=0;	ty<TileHeight;	ty++ )
	{
		for ( unsigned tx=0;	tx<TileWidth;	tx++ )
		{
			//	set current tile
			addbyte( &Program, 115 );
			addbyte( &Program, tx );
			addbyte( &Program, ty );
			
			//	branch (BRANCH?? these bins must be generated instructions or something)
			//	Branch_To_Sub_List
			int TileIndex = tx + ( TileWidth * ty );
			auto* Address = &TileBinMemory[ TileIndex * TILE_BIN_BLOCK_SIZE ];
			addbyte( &Program, 17 );
			addword( &Program, TKernel::GetGpuAddress32(Address) );
			
			bool LastTile = (tx==TileWidth-1) && (ty==TileHeight-1);
			if ( LastTile )
			{
				addbyte( &Program, 25 );
			}
			else
			{
				//	Store_Multi_Sample Store Multi-Sample (Resolved Tile Color Buffer) (R)
				addbyte( &Program, 24 );
			}
		}
	}
	
	addbyte(&Program, 0x5);	//	flush all state
	
	addbyte(&Program, 1);	//	nop
	addbyte(&Program, 0);	//	halt

	return Program;
}


bool TDisplay::SetupRenderControl(void* ProgramMemory,TTileBin* TileBinMemory)
{
	auto* ProgramMemoryEnd = SetupRenderControlProgram( TKernel::GetCpuAddress((uint8_t*)ProgramMemory), TileBinMemory );
	
	auto Length = (int)ProgramMemoryEnd - (int)ProgramMemory;
	if ( Length == 0 )
	{
		DrawString( GetConsoleX(), GetConsoleY(), "thread 1 program length zero" );
		return false;
	}
	
#if defined(RESET_THREAD1)
	{
		//	reset thread...
		auto Status = *GetV3dReg(V3D_CT1CS);
		Status |= 1<<15;
		*GetV3dReg(V3D_CT1CS) = Status;
	}
#endif
	
#if defined(STOP_THREAD1)
	{
		//	stop thread...
		auto Status = *GetV3dReg(V3D_CT1CS);
		Status |= (1<<5);
		*GetV3dReg(V3D_CT1CS) = Status;
	}
#endif
	auto Thread = 1;
	
	
	if ( !WaitForThread(Thread) )
	{
		//	gr: none of these resets are getting rid of the error
		DrawString( GetConsoleX(), GetConsoleY(), "thread 1 pre wait failed" );
#if defined(RESET_THREAD_ON_ERROR)
		//	reset thread...
		auto Status = *GetV3dReg(V3D_CT1CS);
		Status = 1<<15;
		*GetV3dReg(V3D_CT1CS) = Status;
#endif
		/*
		{
			//	stop thread...
			auto Status = *GetV3dReg(V3D_CT1CS);
			Status |= (1<<5);
			*GetV3dReg(V3D_CT1CS) = Status;
		}
		return false;
		 */
	}
	
	
	*GetV3dReg(V3D_CT1CA) = TKernel::GetGpuAddress32(ProgramMemory);
	*GetV3dReg(V3D_CT1EA) = TKernel::GetGpuAddress32(ProgramMemoryEnd);
	
	while ( true )
	{
		auto State = GetThreadState(Thread);
		if ( State == Error )
		{
			auto ErrorStat = ReadV3dReg( V3D_ERRSTAT );
			
			DrawString( GetConsoleX(), GetConsoleY(), "Thread1 Error=" );
			DrawHex( GetConsoleX(false), GetConsoleY(), ErrorStat );
			
#if defined(RESET_THREAD_ON_ERROR)
			//	reset thread...
			auto Status = *GetV3dReg(V3D_CT1CS);
			Status |= 1<<15;
			*GetV3dReg(V3D_CT1CS) = Status;
#endif
			return false;
		}
		
		if ( State == Finished )
			break;
	}

	//	explicit stop
	//*GetV3dReg(V3D_CT1CS) = 0x20;

	return true;
}






class TFixed
{
public:
	const static	uint32_t	Precision = 8;
public:
	TFixed(int i) :
		mFixed	( i << Precision )
	{
	}
	
	uint32_t	GetInt() const		{	return mFixed >> Precision;	}
	
	TFixed&	Multiply(const TFixed& f)
	{
		auto a = this->mFixed;
		auto b = f.mFixed;
		
		//	8.8 * 8.8 = 16.16 so shift down.
		auto temp = (a * b);
		//	round
		auto r = temp + ((temp & 1<<(Precision-1))<<1);
		//	reduce
		r >>= Precision;
		mFixed = r;
		return *this;
	}
	
	TFixed&	Divide(const TFixed& f)
	{
		auto a = this->mFixed;
		auto b = f.mFixed;
		
		//	http://x86asm.net/articles/fixed-point-arithmetic-and-tricks/
		auto r = ( a << Precision ) / b;
		mFixed = r;
		return *this;
	}
	
	friend TFixed&	operator*=(TFixed& This,TFixed const& That)	{	return This.Multiply(That);	}
	friend TFixed&	operator/=(TFixed& This,TFixed const& That)	{	return This.Divide(That);	}
	
protected:
	uint32_t	mFixed;
};

namespace  Math
{
	template<typename T>
	T	Min(T a,T b)		{	return (a<b) ? a : b;	}

	template<typename T>
	T	Max(T a,T b)		{	return (a>b) ? a : b;	}
}

void DrawScreen(TDisplay& Display,int Tick)
{
	auto LastRow = Tick % Display.mHeight;
	auto NextRow = (Tick+1) % Display.mHeight;
	
	int Green = (Tick / 100) % 256;
	int Blue = (Tick / 1000) % 256;
	
	for ( unsigned y=0;	y<Display.mHeight;	y++ )
	{
		//	only draw new lines
		if ( y != LastRow && y != NextRow )
			continue;
		
#if defined(DRAW_RED)
		auto rgba = RGBA(255,Green,Blue,255);
#elif defined(DRAW_GREEN)
		auto rgba = RGBA(Green,255,Blue,255);
#elif defined(DRAW_BLUE)
		auto rgba = RGBA(Green,Blue,255,255);
#else
#error DRAW_XXX expected
#endif
	
		if ( y == NextRow )
			rgba = RGBA(0,0,0,255);
		Display.FillRow( y, rgba );
	}
	
	
	/*
	for ( int y=0;	y<Display.mHeight;	y++ )
	{
		TFixed yf( y );
		yf /= Display.mHeight;
		yf *= 256;
		
		for ( int x=0;	x<Display.mWidth;	x++ )
		{
			TFixed xf( x );
			xf /= Display.mWidth;
			xf *= 256;
			uint32_t rgba = RGBA( xf.GetInt(), yf.GetInt(), Tick%256, 255 );

			if ( Tick % Display.mHeight == y )
				rgba = RGBA(0,0,0,255);
			
			Display.SetPixel( x, y, rgba );
		}
	}
	 */
/*
	for ( int y)
	
	for ( auto i=0;	i<PixelCount;	i++ )
	{
		//auto f = i / static_cast<float>( PixelCount );
		//auto f = i / static_cast<float>( PixelCount );
		//float f = i / 100.0f;
		
		//921600
		//16777216
		auto ColourIndex = i + Tick;
		auto MaxRgb = 255*255*255;
		ColourIndex %= MaxRgb;
		uint32_t rgb = ColourIndex;
		
		uint32_t rgba = rgb;
		rgba |= BGRA(0,0,0,255);
		Display.SetPixel( i, rgba );
	}
	/*
	for ( int y=0;	y<Display.mHeight;	y++ )
{
	for ( int x=0;	x<ScreenWidth;	x++ )
	{
		uint32_t Table[] =
		{
			RGBA(	0,		0,		0,	0 ),
			RGBA(	255,	0,		0,	0 ),
			RGBA(	0,		255,	0,	0 ),
			RGBA(	255,	255,	0,	0 ),
			RGBA(	0,		0,		255,	0 ),
			RGBA(	255,	0,		255,	0 ),
			RGBA(	0,		255,	255,	0 ),
			RGBA(	255,	255,	255,	0 ),
			RGBA(	0,		0,		0,	255 ),
			RGBA(	255,	0,		0,	255 ),
			RGBA(	0,		255,	0,	255 ),
			RGBA(	255,	255,	0,	255 ),
			RGBA(	0,		0,	255,	255 ),
			RGBA(	255,	0,	255,	255 ),
			RGBA(	0,		255,	255,	255 ),
			RGBA(	255,	255,	255,	255 ),
		};
		
		int TableIndex = y / 50;
		if ( TableIndex > 16 )
			TableIndex = 16;
			
			uint32_t bgra = Table[ TableIndex ];
			if ( x < 10 )
				bgra = RGBA(255,255,255,255);
				
				uint32_t ScreenBufferIndex = x + (y*ScreenWidth);
				PUT32( rb + (ScreenBufferIndex*4), bgra );
				}
	 */
}
/*
	
 for(ry=0;ry<480;ry++)
 {
 for(rx=0;rx<480;rx++)
 {
 uint32_t argb = image_data[ra++];
 uint32_t r = (argb >> 0) & 0xff;
 uint32_t g = (argb >> 8) & 0xff;
 uint32_t b = (argb >> 16) & 0xff;
 uint32_t a = (argb >> 24) & 0xff;
 a = 0xff;
 
 r <<= 8;
 g <<= 16;
 b <<= 24;
 a <<= 0;
 
 uint32_t bgra = r | g | b | a;
 PUT32(rb,bgra);
 rb+=4;
 }
 for(;rx<640;rx++)
 {
 PUT32(rb,0);
 rb+=4;
 }
 }
 */

	
volatile uint32_t MailboxBuffer[1000*1000] __attribute__ ((aligned(16)));
int MailboxBufferPos = 1;
volatile uint32_t* AllocMailboxBuffer(int Size)
{
	//	align to 16 bytes
	int Align = 16/sizeof(uint32_t);
	if ( MailboxBufferPos % Align != 0 )
	{
		MailboxBufferPos += Align - (MailboxBufferPos % Align);
	}
	auto* Address = &MailboxBuffer[MailboxBufferPos];
	MailboxBufferPos += Size;
	return Address;
}

//	returns number of elements returned
template<size_t PAYLOADSIZE>
int TMailbox::SetProperty(TMailbox::TTag Tag,TMailbox::TChannel Channel,uint32_t (& Payload)[PAYLOADSIZE],bool ReadData)
{
	auto Tag32 = static_cast<uint32_t>( Tag );
	auto Size = PAYLOADSIZE + 6;
	//volatile static uint32_t Data[PAYLOADSIZE + 6] __attribute__ ((aligned(16)));
	auto* Data = AllocMailboxBuffer(Size+10);
	
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
	MailboxWrite( Data, Channel );
	
	if ( !ReadData )
		return 0;
	
	MailboxRead( Channel );
	
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


TGpuMemory::TGpuMemory(uint32_t Size,bool Lock) :
	mHandle			( 0 ),
	mSize			( Size ),
	mLockedAddress	( nullptr )
{
	//	gr: most things need to be aligned to 16 bytes
	auto AlignBytes = 0x1000;
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
	
	



CAPI int notmain ( void )
{
	TKernel Kernel;
	//TDisplay Display( 1280, 720, true );
	//TDisplay Display( 1920, 1080, true );
	TDisplay Display( 512, 512, true );
	
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Hello world!");
	
	//	gr: alphabet test seems okay
	/*
	char Alphabet[] = "abdefghijklmnopqrstuvwxyz0123456789():,.*!";
	for ( int i=0;	i<10000;	i++ )
	{
		auto Char = Alphabet[ i % sizeof(Alphabet)];
		char String[2] = { Char, 0 };
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), String );
	}
	*/
	
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "MailboxBufferPos" );
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), MailboxBufferPos );
	

	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Cpu base address: ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), TKernel::mCpuMemoryBase );
	Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), " size: ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), TKernel::mCpuMemorySize );
	
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Gpu base address: ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), TKernel::mGpuMemoryBase );
	Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), " size: ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), TKernel::mGpuMemorySize );
	
	
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Screen Buffer address = ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), (uint32_t)Display.mScreenBuffer );
	
	
	auto DebugAlloc = [&Display](TGpuMemory& Mem,const char* Name)
	{
		Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), Name );
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), " mem handle: ");
		Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), Mem.mHandle );
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), ", address: ");
		auto Addr =(uint32_t)Mem.GetAllocatedAddress();
		Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), Addr );
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), ", size: ");
		Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), Mem.GetSize() );
	};
	
	
	TGpuMemory TileBins( MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_BIN_BLOCK_SIZE * sizeof(TTileBin)*1000, true );
	DebugAlloc( TileBins, "Tile Bins" );
/*
	if ( !TileBins.Unlock() )
		Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Tile bin unlock failed" );
*/
	//	gr: something around here messes with the const strings
	//		moving code around does it
	
	int StateSize = MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_STRUCT_SIZE*1000;
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Allocating tile state bytes: " );
	Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), StateSize );
	
	TGpuMemory TileState( StateSize, true );
	DebugAlloc( TileState, "Tile State" );
	
	TGpuMemory Program0( 4096, true );
	DebugAlloc( Program0, "Program0" );
	
	TGpuMemory Program1( 4096, true );
	DebugAlloc( Program1, "Program1" );
	
	TGpuMemory Program2( 4096, true );
	DebugAlloc( Program2, "Program2" );
	
	TGpuMemory Program3( 4096, true );
	DebugAlloc( Program3, "Program3" );
	
	TGpuMemory Program4( 4096, true );
	DebugAlloc( Program4, "Program4" );

	
	uint32_t Tick = 0;
	while ( true )
	{
		Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Tick ");
		Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), Tick );
		
		Display.mClearColour = RGBA( (Tick&1) * 255, 0, 255, 255 );
		
		if ( !Display.SetupBinControl( Program0.GetGpuAddress(), reinterpret_cast<TTileBin*>(TileBins.GetGpuAddress()), TileBins.GetSize(), TileState.GetGpuAddress() ) )
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupBinControl failed");
			Sleep(10);
		}
		else
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupBinControl success");
		}

		//	gr this actually draws...
		if ( !Display.SetupRenderControl( Program1.GetGpuAddress(), reinterpret_cast<TTileBin*>(TileBins.GetGpuAddress()) ) )
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupRenderControl failed");
			Sleep(10);
		}
		else
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupRenderControl success");
		}
		
		Tick++;
		Sleep(100);
	}


    return(0);
}



