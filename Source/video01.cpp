#include "Kernel.h"
#include "Sprites.h"
#include "Blitter.h"
#include "Display.h"
#include "Memory.h"
#include "Js_Duktape.h"



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



void DrawScreen(TDisplay& Display,int Tick);





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

extern void SetTick(uint32_t Tick);

namespace Js
{
	void Assert()
	{
		(*Js::TContext::mPrint)("JS assert");
		int x = 10000;
		while( x-- ){}
	}
}

CAPI int notmain()
{
	TKernel Kernel;
	//TDisplay Display( 1280, 720 );
	//TDisplay Display( 1920, 1080 );
	TDisplay Display( 320, 240, true );
	
	Display.FillPixelsCheckerBoard(CHECKERBOARD_SIZE,CHECKERBOARD_COLOURA,CHECKERBOARD_COLOURB);
	
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Hello world!");

#if !defined(TARGET_OSX)
	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Cpu base address: ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), TKernel::mCpuMemoryBase );
	Display.DrawString( Display.GetConsoleX(false), Display.GetConsoleY(), " size: ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), TKernel::mCpuMemorySize );
#endif

	Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Screen Buffer address = ");
	Display.DrawHex( Display.GetConsoleX(false), Display.GetConsoleY(), (uint32_t)(intptr_t)Display.mScreenBuffer );

	
	std::function<void(const char*)> Print = [&](const char* String)
	{
		Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), String );
	};
	auto PrintNum = [&](int Number)
	{
		Display.DrawNumber( Display.GetConsoleX(), Display.GetConsoleY(), Number );
	};
	Js::TContext::mPrint = &Print;

	(*Js::TContext::mPrint)("pad");
	
	(*Js::TContext::mPrint)("One");
	Js::TContext Context( Kernel.mHeap, Print );
	(*Js::TContext::mPrint)("Two");

	uint32_t Tick = 0;
	while ( ++Tick )
	{
		Print("Tick x");
		PrintNum(Tick);
		//Display.DrawString( Display.GetConsoleX(), Display.mHeight-9, "beep! tick #");
		//Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), Tick );
		
		Debug::Print("I am a Debug::Print.");
		
		Kernel.mDebugLogger.Pop( Print );
		
		TKernel::Sleep(1000);
	}

    return(0);
}



