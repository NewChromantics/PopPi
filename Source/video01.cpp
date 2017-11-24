#include "Kernel.h"
#include "Sprites.h"
#include "Blitter.h"
#include "Display.h"
#include "Mailbox.h"
#include "Memory.h"


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



#define CHECKERBOARD_SIZE		16
#define CHECKERBOARD_COLOURA	RGBA( 247,191,9,255 )
#define CHECKERBOARD_COLOURB	RGBA( 249,221,127,255 )
// = RGBA( 56,185,255,255 );
//Colours[1] = RGBA( 199,214,221,255 );


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


void DrawScreen(TDisplay& Display,int Tick);




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


uint8_t gTileBin[MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_BIN_BLOCK_SIZE * sizeof(TTileBin)]  __attribute__ ((aligned(16)));
uint8_t gTileState[MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_STRUCT_SIZE]  __attribute__ ((aligned(16)));
//static volatile uint32_t* TileBin = (uint32_t*)0x00400000;
//static volatile uint8_t* TileState = (uint8_t*)00500000;
uint8_t gProgram0[0x1000]  __attribute__ ((aligned(4)));
uint8_t gProgram1[0x1000]  __attribute__ ((aligned(4)));



CAPI int notmain ( void )
{
	TKernel Kernel;
	//TDisplay Display( 1280, 720 );
	//TDisplay Display( 1920, 1080 );
	TDisplay Display( 640, 522 );
	
	Display.FillPixelsCheckerBoard(CHECKERBOARD_SIZE,CHECKERBOARD_COLOURA,CHECKERBOARD_COLOURB);

	Display.SetupGpu();

	
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
	
	
//#define USE_MAPPED_ALLOCATION

#if defined(USE_MAPPED_ALLOCATION)
	#define TALLOCMemory	TMappedMemory
	#define USE_BIG_ALLOCATION	false
#else
	#define USE_BIG_ALLOCATION	false
	#define TALLOCMemory	TGpuMemory
#endif
	
#define GetBUFFERAddress	GetAllocAddress

	auto DebugAlloc = [&Display](TALLOCMemory& Mem,const char* Name)
	{
		Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), Name );
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), " mem handle: ");
		Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), Mem.mHandle );
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), ", alloc address: ");
		auto Addr =(uint32_t)Mem.GetAllocAddress();
		Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), Addr );
		Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), ", size: ");
		Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), Mem.GetSize() );
	};
	
	
	auto DebugMemoryDump = [](TALLOCMemory& Memory,const char* Name,TBlitter& Blitter,int ChunkSize)
	{
		return;
		auto* Mem = Memory.GetBUFFERAddress();
		
		Blitter.DrawString( Blitter.GetConsoleX(), Blitter.GetConsoleY(), "Memory dump of ");
		Blitter.DrawString( Blitter.GetConsoleX(false), Blitter.GetConsoleY(), Name );

		if ( !Mem )
		{
			Blitter.DrawString( Blitter.GetConsoleX(), Blitter.GetConsoleY(), "(null)" );
			return;
		}
		
		auto ChunkCount = Memory.GetSize() / ChunkSize;
		
		ChunkCount=3;
		
		char HexByteString[4];
		auto GetHexByteString = [&](uint8_t Byte)
		{
			uint8_t a = (Byte>>4) & 0x0f;
			uint8_t b = (Byte>>0) & 0x0f;
	
			HexByteString[0] = ( a >= 10 ) ? ((a-10) + 'a') : a + '0';
			HexByteString[1] = ( b >= 10 ) ? ((b-10) + 'a') : b + '0';
			HexByteString[2] = ' ';
			HexByteString[3] = '\0';
			
			return HexByteString;
		};
		
		for ( unsigned Chunk=0;	Chunk<ChunkCount;	Chunk++ )
		{
			Blitter.DrawString( Blitter.GetConsoleX(), Blitter.GetConsoleY(), "Chunk #");
			Blitter.DrawNumber( Blitter.GetConsoleX(false), Blitter.GetConsoleY(), Chunk);
			
			auto* Addr = &Mem[Chunk * ChunkSize];
			for ( int i=0;	i<ChunkSize;	i++ )
			{
				auto* String = GetHexByteString( Addr[i] );
				Blitter.DrawString( Blitter.GetConsoleX( i==0 ), Blitter.GetConsoleY(), String);
				//Blitter.DrawNumber( Blitter.GetConsoleX( i==0 ), Blitter.GetConsoleY(), Addr[i] );
			}
		}
	};
	
#define BIN_PAD_SCALAR	1
	
	
#if defined(USE_MAPPED_ALLOCATION)
	
	TMappedMemory TileBins( gTileBin );
	DebugAlloc( TileBins, "Tile Bins" );
	
	TMappedMemory TileState( gTileState );
	DebugAlloc( TileState, "Tile State" );
	
	TMappedMemory Program0( gProgram0 );
	DebugAlloc( Program0, "Program0" );
	
	TMappedMemory Program1( gProgram1 );
	DebugAlloc( Program1, "Program1" );
	
	
#else
	TALLOCMemory TileBins( MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_BIN_BLOCK_SIZE * sizeof(TTileBin), !USE_BIG_ALLOCATION );
	DebugAlloc( TileBins, "Tile Bins" );

	TALLOCMemory TileState( MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_STRUCT_SIZE, !USE_BIG_ALLOCATION );
	DebugAlloc( TileState, "Tile State" );
	
	TALLOCMemory Program0( 4096, !USE_BIG_ALLOCATION );
	DebugAlloc( Program0, "Program0" );
	
	TALLOCMemory Program1( 4096, !USE_BIG_ALLOCATION );
	DebugAlloc( Program1, "Program1" );

	
#if USE_BIG_ALLOCATION==true
	TALLOCMemory BigAlloc( TileBins.GetSize() + TileState.GetSize() +  Program0.GetSize() + Program1.GetSize(), true );
	DebugAlloc( BigAlloc, "BigAlloc" );
#endif
#endif

	bool DrawTick = true;
	
	uint32_t Tick = 0;
	while ( true )
	{
#if USE_BIG_ALLOCATION==true
		auto* TileBinMem = reinterpret_cast<TTileBin*>( BigAlloc.GetBUFFERAddress() );
		auto* TileStateMem = BigAlloc.GetBUFFERAddress() + TileBins.GetSize();
		auto* Program0Mem = TileStateMem + TileState.GetSize();
		auto* Program1Mem = Program0Mem + Program0.GetSize();
#else
		auto* TileBinMem = reinterpret_cast<TTileBin*>( TileBins.GetBUFFERAddress() );
		auto* TileStateMem = TileState.GetBUFFERAddress();
		auto* Program0Mem = Program0.GetBUFFERAddress();
		auto* Program1Mem = Program1.GetBUFFERAddress();
#endif
		if ( DrawTick )
		{
			Display.DrawString( Display.GetConsoleX(), Display.mHeight-9, "Tick ");
			Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), Tick );
		}
		
		//Display.mClearColour = RGBA( (Tick&1) * 255, 0, 255, 255 );
		Display.mClearColour = RGBA( Tick%255, (Tick/10)%255, (Tick/100)%255, 255 );
	

		if ( !Display.SetupBinControl( Program0Mem, TileBinMem, TileBins.GetSize(), TileStateMem ) )
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupBinControl failed");
			TKernel::Sleep(10);
		}
		else
		{
			//Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupBinControl success");
		}

		DebugMemoryDump( TileBins, "Tile bins", Display, TILE_BIN_BLOCK_SIZE*sizeof(TTileBin) );
		DebugMemoryDump( TileState, "Tile State", Display, TILE_STRUCT_SIZE );
#if USE_BIG_ALLOCATION==true
		DebugMemoryDump( BigAlloc, "BigAlloc", Display, TILE_BIN_BLOCK_SIZE*sizeof(TTileBin) );
#endif
		
		bool Abort = false;
		//	gr this actually draws...
		if ( !Display.SetupRenderControl( Program1Mem, TileBinMem) )
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupRenderControl failed");
			TKernel::Sleep(10);
			Abort = true;
		}
		else
		{
			//Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "SetupRenderControl success");
		}
		DebugMemoryDump( TileBins, "Tile bins post render", Display, TILE_BIN_BLOCK_SIZE*sizeof(TTileBin) );
		DebugMemoryDump( TileState, "Tile State post render", Display, TILE_STRUCT_SIZE );
	
		if ( Abort )
			return 1;

		//if ( Tick == 0 )
		//	TKernel::Sleep(500);

		Tick++;
	}


    return(0);
}



