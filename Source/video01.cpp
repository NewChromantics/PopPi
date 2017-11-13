
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
#if defined(TARGET_CPP)
}
#endif

//	display is BGRA
#define BGRA(r,g,b,a)		( ((r)<<0) | ((g)<<8) | ((b)<<16) | ((a)<<24) )
#define RGBA(r,g,b,a)		( BGRA(b,g,r,a) )

typedef unsigned int uint32_t;


CAPI unsigned int MailboxWrite ( unsigned int fbinfo_addr, unsigned int channel )
{
    unsigned int mailbox;

    mailbox=0x2000B880;
    while(1)
    {
        if((GET32(mailbox+0x18)&0x80000000)==0) break;
    }
    PUT32(mailbox+0x20,fbinfo_addr+channel);
    return(0);
}

CAPI unsigned int MailboxRead ( unsigned int channel )
{
    unsigned int ra;
    unsigned int mailbox;

    mailbox=0x2000B880;
    while(1)
    {
        while(1)
        {
            ra=GET32(mailbox+0x18);
            if((ra&0x40000000)==0) break;
        }
        //hexstrings(ra);
        ra=GET32(mailbox+0x00);
        //hexstring(ra);
        if((ra&0xF)==channel) break;
    }
    return(ra);
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
	
public:
	uint32_t	mWidth;
	uint32_t	mHeight;
	uint32_t	mScreenBufferAddress;
};



TKernel::TKernel()
{
	//	terminal debug
	uart_init();
	hexstring(0x12345678);
	hexstring(GETPC());
	
	//	set clock speed
	timer_init();
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
	
	PUT32(0x40040000, mWidth); /* #0 Physical Width */
	PUT32(0x40040004, mHeight); /* #4 Physical Height */
	PUT32(0x40040008, mWidth); /* #8 Virtual Width */
	PUT32(0x4004000C, mHeight); /* #12 Virtual Height */
	PUT32(0x40040010, GpuPitch); /* #16 GPU - Pitch */
	PUT32(0x40040014, BitDepth); /* #20 Bit Depth */
	PUT32(0x40040018, ScrollX); /* #24 X */
	PUT32(0x4004001C, ScrollY); /* #28 Y */
	PUT32(0x40040020, 0); /* #32 GPU - Pointer */
	PUT32(0x40040024, 0); /* #36 GPU - Size */
	
	//	? commit ram
	hexstring(MailboxWrite(0x40040000,1));
	//	? block until ready
	hexstring(MailboxRead(1));

	
	//	read/verify meta?
	/*
	uint32_t rb=0x40040000;
	for( auto ra=0;ra<10;ra++)
	{
		hexstrings(rb);
		hexstring(GET32(rb));
		rb+=4;
	}
	*/
	
	//	init screen planes?
	/*
	rb=GET32(0x40040020);
	hexstring(rb);
	for( auto ra=0;ra<10000;ra++)
	{
		PUT32(rb,~((ra&0xFF)<<0));
		rb+=4;
	}
	for(auto ra=0;ra<10000;ra++)
	{
		PUT32(rb,~((ra&0xFF)<<8));
		rb+=4;
	}
	for(auto ra=0;ra<10000;ra++)
	{
		PUT32(rb,~((ra&0xFF)<<16));
		rb+=4;
	}
	for(auto ra=0;ra<10000;ra++)
	{
		PUT32(rb,~((ra&0xFF)<<24));
		rb+=4;
	}
	*/
	mScreenBufferAddress = GET32(0x40040020);
}


void TDisplay::SetPixel(int Index,uint32_t Colour)
{
	auto Address = Index;
	Address *= 4;
	Address += mScreenBufferAddress;
	
	PUT32( Address, Colour );
}


void TDisplay::SetPixel(int x,int y,uint32_t Colour)
{
	auto Address = x + (y*mWidth);
	SetPixel( Address, Colour );
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


void DrawScreen(TDisplay& Display,int Tick)
{
	auto PixelCount = Display.mHeight * Display.mWidth;
	Tick %= Display.mHeight;
	
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
			uint32_t rgba = RGBA( xf.GetInt(), yf.GetInt(), 0, 255 );

			if ( Tick == y )
				rgba = RGBA(0,0,0,255);
			
			Display.SetPixel( x, y, rgba );
		}
	}
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



