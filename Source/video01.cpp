
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

//	display is BGRA
#define BGRA(r,g,b,a)		( ((r)<<0) | ((g)<<8) | ((b)<<16) | ((a)<<24) )
#define RGBA(r,g,b,a)		( BGRA(b,g,r,a) )

typedef unsigned int uint32_t;



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


//	some more references for magic nmbers
//	https://www.raspberrypi.org/forums/viewtopic.php?f=29&t=65596
//	http://magicsmoke.co.za/?p=284
#define MAILBOX_STATUS_BUSY 0x80000000
#define MAILBOX_STATUS_EMPTY 0x40000000


void MailboxWrite(void* Data,unsigned int Channel)
{
    unsigned int mailbox = 0x2000B880;
	
	uint32_t BufferAddress = (uint32_t)Data;
	
	auto ChannelBitMask = (1<<4)-1;
	if ( BufferAddress & ChannelBitMask )
	{
		//	throw
		BufferAddress &= ~ChannelBitMask;
	}
	
	//	map memory address to physical address... so... the cached/physical memory/virtual memory address over the bus
	//	gr: note; not l1cache!
	//	L2cache enabled
	BufferAddress |= 0x40000000;
	//	L2cache disabled
	//MailBoxAddress |= 0xC0000000;
	
	//	and bit 1 to say... we're writing it? or because it's the channel?..
	//	odd that it'll overwrite the display widht, but maybe that has to be aligned or something anyway
	//	4bits per channel
	//	gr: could be AllocateBuffer command; http://magicsmoke.co.za/?p=284
	//	gr: why arent we setting all these tags? http://magicsmoke.co.za/?p=284
	BufferAddress |= Channel;

    while(1)
    {
		//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
		//	0x80000000: request successful
		//	0x80000001: error parsing request buffer (partial response)
		//uint32_t MailboxResponse = GET32(mailbox+20);
		uint32_t MailboxResponse = MAILBOX0->sender;
		//	break when we... have NO response waiting??
		bool StatusBusy = (MailboxResponse & MAILBOX_STATUS_BUSY) == MAILBOX_STATUS_BUSY;
		if( !StatusBusy )
			break;
    }
	
	//	gr: where is
	
	//	gr: I bet this is configuration
	MAILBOX0->write = BufferAddress;
}

uint32_t MailboxRead(unsigned int Channel)
{
	auto ChannelBitMask = (1<<4)-1;
    unsigned int mailbox = 0x2000B880;
    while(1)
    {
        while(1)
        {
			//	gr: sender, not status, suggests this struct may be off...
			uint32_t MailboxResponse = MAILBOX0->sender;
			bool StatusEmpty = (MailboxResponse & MAILBOX_STATUS_EMPTY) == MAILBOX_STATUS_EMPTY;
            if ( !StatusEmpty )
				break;
        }

		//	gr: switching this to MAILBOX0->read doesn't work :/
		auto Response = GET32( mailbox+0x00 );
		auto CurrentChannelResponse = Response & ChannelBitMask;
        if ( CurrentChannelResponse == Channel )
		{
			Response &= ~ChannelBitMask;
			return Response;
		}
    }
	
	//	throw
	return -1;
}

bool MailboxEnableQpu(int Channel,bool Enable=true)
{
	uint32_t Data[7] __attribute__ ((aligned(16)));
	static_assert( sizeof(Data) == 4*7, "sizeof");
	
	//	header
	Data[0] = sizeof(Data);
	Data[1] = 0x00000000;	//	process request (0 = writing mailbox)
	Data[2] = 0x30012;		//	the tag id
	Data[3] = 4;			//	size of the buffer
	Data[4] = 4;			//	size of the data
	
	//	buffer here
	Data[5] = Enable ? 1 : 0;
	
	//	footer
	Data[6] = 0x00000000;	// end tag
	
	//	gr: switched to channel 0 and it booted okay..
	Channel = 0;
	
	MailboxWrite( Data, Channel );
	//	wait for it to finish
	MailboxRead( Channel );
	
	//mbox_property(file_desc, p);
	//return p[5];
	auto Enabled = (Data[5] == 1);
	return Enabled;
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


class TKernel
{
public:
	TKernel();
};

class TDisplay
{
public:
	TDisplay(int Width,int Height);

	void		SetPixel(int x,int y,uint32_t Colour);
	void		SetPixel(int Index,uint32_t Colour);
	void		SetRow(int y,uint32_t Colour);
	void		FillPixelsGradient();
	void		FillPixelsCheckerBoard(int SquareSize);
	
public:
	uint32_t	mWidth;
	uint32_t	mHeight;
	uint32_t	mScreenBufferAddress;
};

void exit(int Error)
{
	
}

TKernel::TKernel()
{
	//	terminal debug
	uart_init();
	hexstring(0x12345678);
	hexstring(GETPC());
	
	//	set clock speed
	timer_init();
	
	//	enable level 1 cache (faster!)
	//	https://www.raspberrypi.org/forums/viewtopic.php?t=16851
	start_l1cache();
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


TDisplay::TDisplay(int Width,int Height) :
	mScreenBufferAddress	( 0 ),
	mWidth					( Width ),
	mHeight					( Height )
{
	auto ScrollX = 0;
	auto ScrollY = 0;
	auto BitDepth = 32;
	auto GpuPitch = 0;

//	PERIPHERAL_BASE                = $3F000000 ; Peripheral Base Address
	//	gr: is this a general ram address?
	//	https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
	//	if L2 cache is enabled, this address needs to start at 0x40000000
	
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
	int Channel = 1;
	MailboxWrite( &DisplayInfo,Channel);
	//	? block until ready
	MailboxRead(Channel);

	//	read new contents of struct
	mScreenBufferAddress = (uint32_t)DisplayInfo.mPixelBuffer;
	//	todo: if addr=0, loop
	
	FillPixelsCheckerBoard(10);
	Sleep(1000);
	
	MailboxEnableQpu(Channel,true);
	
	//	gr: no speed difference
	//	https://github.com/PeterLemon/RaspberryPi/blob/master/Input/NES/Controller/GFXDemo/kernel.asm
	//and r0,$3FFFFFFF ; Convert Mail Box Frame Buffer Pointer From BUS Address To Physical Address ($CXXXXXXX -> $3XXXXXXX)
	//mScreenBufferAddress &= 0x3FFFFFFF;
	
	
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
	
#define PERPIPHERAL_BASE	0x3F000000
#define V3D_BASE	0xC00000	//	; V3D Base Address ($20C00000 PHYSICAL, $7EC00000 BUS)
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


void TDisplay::SetPixel(int Index,uint32_t Colour)
{
	auto Address = Index;
	Address *= 4;
	Address += mScreenBufferAddress;
	
	//	gr: no perf improvement
	//uint32_t* Buffer = (uint32_t*)Address;
	//*Buffer = Colour;
	PUT32( Address, Colour );
}


void TDisplay::SetPixel(int x,int y,uint32_t Colour)
{
	auto Address = x + (y*mWidth);
	SetPixel( Address, Colour );
}


void TDisplay::SetRow(int y,uint32_t Colour)
{
	auto Start = 0 + ( y * mWidth );
	auto End = mWidth + ( y * mWidth );
	auto Address = mScreenBufferAddress + (Start * 4);
	auto EndAddress = mScreenBufferAddress + (End * 4);
	
	for ( ;	Address<EndAddress;	Address+=4 )
	{
		PUT32( Address, Colour );
	}
}


void TDisplay::FillPixelsGradient()
{
	auto PixelCount = mHeight * mWidth;
	
	uint32_t rgba = RGBA( 0,0,0,255 );
	auto Address = mScreenBufferAddress;
	//uint32_t* Buffer = (uint32_t*)Address;
	
	for ( int i=0;	i<PixelCount;	i++,Address+=4)//,Buffer++ )
	{
		rgba = RGBA(i % 256,255,0,255);
		PUT32( Address, rgba );
		//*Buffer = rgba;
	}
	
}


void TDisplay::FillPixelsCheckerBoard(int SquareSize)
{
	uint32_t Colours[2];
	Colours[0] = RGBA( 56,185,255,255 );
	Colours[1] = RGBA( 199,214,221,255 );
	
	auto* Pixels = (uint32_t*)mScreenBufferAddress;
	for ( unsigned y=0;	y<mHeight;	y++ )
	{
		auto yodd = (y/SquareSize) & 1;
		for ( unsigned x=0;	x<mWidth;	x++ )
		{
			auto xodd = (x/SquareSize) & 1;
			if ( yodd )
				xodd = !xodd;
			auto ColourIndex = xodd;
			int p = x + (y * mWidth);
			Pixels[p] = Colours[ColourIndex];
		}
	}
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
		Display.SetRow( y, rgba );
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


CAPI int notmain ( void )
{
	TKernel Kernel;
	TDisplay Display( 1280, 720 );
	
	uint32_t Tick = 0;
	while ( true )
	{
		DrawScreen( Display, Tick );
		Tick++;
	}


    return(0);
}



