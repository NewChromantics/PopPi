
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
/*
CAPI void __aeabi_unwind_cpp_pr0(void);
CAPI void __aeabi_unwind_cpp_pr1(void);
CAPI void __aeabi_unwind_cpp_pr2(void);

void
__aeabi_unwind_cpp_pr0(void)
{
	//panic("__aeabi_unwind_cpp_pr0");
}

void
__aeabi_unwind_cpp_pr1(void)
{
	//panic("__aeabi_unwind_cpp_pr1");
}

void
__aeabi_unwind_cpp_pr2(void)
{
	//panic("__aeabi_unwind_cpp_pr2");
}
*/

//------------------------------------------------------------------------
CAPI int notmain ( void )
{
    unsigned int ra,rb;
    unsigned int ry,rx;

    uart_init();
    hexstring(0x12345678);
    hexstring(GETPC());
    timer_init();
	
	uint32_t ScreenWidth = 1280;
	uint32_t ScreenHeight = 720;

    PUT32(0x40040000, ScreenWidth); /* #0 Physical Width */
    PUT32(0x40040004, ScreenHeight); /* #4 Physical Height */
    PUT32(0x40040008, ScreenWidth); /* #8 Virtual Width */
    PUT32(0x4004000C, ScreenHeight); /* #12 Virtual Height */
    PUT32(0x40040010, 0); /* #16 GPU - Pitch */
    PUT32(0x40040014, 32); /* #20 Bit Depth */
    PUT32(0x40040018, 0); /* #24 X */
    PUT32(0x4004001C, 0); /* #28 Y */
    PUT32(0x40040020, 0); /* #32 GPU - Pointer */
    PUT32(0x40040024, 0); /* #36 GPU - Size */


    hexstring(MailboxWrite(0x40040000,1));
    hexstring(MailboxRead(1));
    rb=0x40040000;
    for(ra=0;ra<10;ra++)
    {
        hexstrings(rb); hexstring(GET32(rb));
        rb+=4;
    }

    rb=GET32(0x40040020);
    hexstring(rb);
    for(ra=0;ra<10000;ra++)
    {
        PUT32(rb,~((ra&0xFF)<<0));
        rb+=4;
    }
    for(ra=0;ra<10000;ra++)
    {
        PUT32(rb,~((ra&0xFF)<<8));
        rb+=4;
    }
    for(ra=0;ra<10000;ra++)
    {
        PUT32(rb,~((ra&0xFF)<<16));
        rb+=4;
    }
    for(ra=0;ra<10000;ra++)
    {
        PUT32(rb,~((ra&0xFF)<<24));
        rb+=4;
    }
    rb=GET32(0x40040020);
    hexstring(rb);
    ra=0;
	
	
	//	display is BGRA
#define BGRA(r,g,b,a)		( ((r)<<0) | ((g)<<8) | ((b)<<16) | ((a)<<24) )
#define RGBA(r,g,b,a)		( BGRA(b,g,r,a) )
	
	for ( int y=0;	y<ScreenHeight;	y++ )
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

    return(0);
}



