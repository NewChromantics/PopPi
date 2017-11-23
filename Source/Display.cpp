#include "Display.h"
#include "Kernel.h"
#include "Mailbox.h"

#define ENABLE_TRIANGLES

//	gr: if I don't stop thread 0, it hangs after 22 iterations
//#define RESET_THREAD0
//#define STOP_THREAD0
//#define RESET_THREAD1
//#define STOP_THREAD1
#define RESET_THREAD_ON_ERROR
#define STOP_THREAD_ON_ERROR

//#define BINTHREAD_FLUSH
#define BINTHREAD_FLUSHALL		//	wont render without
//#define BINTHREAD_NOPHALT		//	adding causes thread1 error
//#define RENDERTHREAD_FLUSH		//	adding causes thread1 error
//#define RENDERTHREAD_FLUSHALL	//	adding causes thread1 error
//#define RENDERTHREAD_NOPHALT

#define GetTHREADAddress32			GetUnmodifiedAddress32
#define GetSCREENBUFFERAddress32	GetBusAddress32
#define GetTILEBINAddress32			GetUnmodifiedAddress32
#define GetPROGRAMMEMAddress		GetUnmodifiedAddress


#define ALLOC_ALIGNMENT				0x1000


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



TDisplay::TDisplay(int Width,int Height) :
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
	TMailbox::Write( &DisplayInfo,Channel);
	//	? block until ready
	TMailbox::Read( Channel );
	
	//	read new contents of struct
	mScreenBuffer = DisplayInfo.mPixelBuffer;
	//	todo: if addr=0, loop
	
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
	
	
	TMailbox::EnableQpu(true);
	
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
	*GetV3dReg(RegStart) = TKernel::GetTHREADAddress32(Memory);
	//	set end address, also starts execution
	*GetV3dReg(RegEnd) = TKernel::GetTHREADAddress32(Memory) + ExecuteSize;
	
	
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

#define AUTO_INIT_TILE_STATE_CMD	(1<<2)

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
uint32_t FRAGMENT_SHADER_CODE[] __attribute__ ((aligned(16)))=
{
	/*
	//	hackdriver
	0x958e0dbf,
	0xd1724823,// mov r0, vary; mov r3.8d, 1.0
	0x818e7176,
	0x40024821, // fadd r0, r0, r5; mov r1, vary
	0x818e7376,
	0x10024862, // fadd r1, r1, r5; mov r2, vary
	0x819e7540,
	0x114248a3, // fadd r2, r2, r5; mov r3.8a, r0
	0x809e7009,
	0x115049e3, // nop; mov r3.8b, r1
	0x809e7012,
	0x116049e3, // nop; mov r3.8c, r2
	0x159e76c0,
	0x30020ba7, // mov tlbc, r3; nop; thrend
	0x009e7000,
	0x100009e7, // nop; nop; nop
	0x009e7000,
	0x500009e7, // nop; nop; sbdone
*/
	
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
	float		z;	//	float
	float		w;	//	float
	float		r;
	float		g;
	float		b;
};
static_assert( sizeof(TVertex) == 6*4, "Vertex is unexpected size");
#define VERTEX_COUNT	3
TVertex VERTEX_DATA[VERTEX_COUNT] __attribute__ ((aligned(16))) =
{
	{	EndianSwap16(1 * 16),	EndianSwap16(1 * 16),	1,1, 	1,0,0	},
	{	EndianSwap16(100 * 16),	EndianSwap16(1 * 16),	1,1, 	0,1,0	},
	{	EndianSwap16(100 * 16),	EndianSwap16(100 * 16),	1,1, 	0,0,1	},
};

uint8_t VERTEX_INDEXES[VERTEX_COUNT] __attribute__ ((aligned(16))) =
{
	0,1,2
};

uint8_t* SetupVertexShaderState()
{
	uint8_t* ShaderState = NV_SHADER_STATE_RECORD;
	
	//	Flag Bits: 0 = Fragment Shader Is Single Threaded,
	//	1 = Point Size Included In Shaded Vertex Data,
	//	2 = Enable Clipping,
	//	3 = Clip Coordinates Header Included In Shaded Vertex Data
	uint8_t Flags = 0;
	uint8_t VertexDataStride = sizeof(TVertex);
	uint8_t UniformCount = 0;	//	docs: currently unused
	uint8_t VaryingsCount = 3;
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
	//	CONTROL_LIST_BIN_STRUCT
	auto* p = TKernel::GetPROGRAMMEMAddress((uint8_t*)ProgramMemory);
	
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
	
	
#define Enable_Forward_Facing_Primitive	0x01	//	Configuration_Bits: Enable Forward Facing Primitive
#define Enable_Reverse_Facing_Primitive	0x02	//	Configuration_Bits: Enable Reverse Facing Primitive
#define EarlyDepthWrite			0x02
#define Mode_Triangles					0x04	//	Indexed_Primitive_List: Primitive Mode = Triangles
#define Index_Type_8 					0x00	//	Indexed_Primitive_List: Index Type = 8-Bit
#define Index_Type_16					0x10	//	Indexed_Primitive_List: Index Type = 16-Bit
#define DepthTestDisabled				0x0
	
	//	primitive list
	enum PrimitiveType : uint8_t
	{
		Points		= 0,
		Lines		= 1,
		Triangles	= 2,
		RHT			= 3,
	};
	enum PrimitiveSize : uint8_t
	{
		Index16		= 1 << 4,	//	LdB-ECM says 16bit
		XY32		= 3 << 4,	//	hackdriver says 16bit
	};

	
	//	state
	//	Configuration_Bits Enable_Forward_Facing_Primitive + Enable_Reverse_Facing_Primitive, Early_Z_Updates_Enable ; Configuration Bits
	uint8_t Config[3];
	Config[0] = Enable_Forward_Facing_Primitive | Enable_Reverse_Facing_Primitive;
	Config[1] = DepthTestDisabled;
	Config[2] = EarlyDepthWrite;
	addbyte(&p, 96);
	addbyte(&p, Config[0]);
	addbyte(&p, Config[1]);
	addbyte(&p, Config[2]);
	
	//	wont render without
	//	Viewport_Offset
	addbyte(&p, 103);
	addshort(&p, 0);
	addshort(&p, 0);

	
	
	
	//	Clip_Window - left, bottom, width, height
	addbyte(&p, 102);
	addshort(&p, 0 );
	addshort(&p, 0 );
	addshort(&p, mWidth );
	addshort(&p, mHeight );
	
	uint8_t DataType = PrimitiveType::Triangles | (3<<4);
	addbyte(&p, 56);
	addbyte(&p, DataType);

	//	primitive instance
	//	No vertex shader (pre transformed verts)
	//	NV_Shader_State NV_SHADER_STATE_RECORD ; NV Shader State (No Vertex Shading)
	addbyte(&p, 65);
	auto* VertexShaderState = SetupVertexShaderState();
	addword(&p, (uint32_t)VertexShaderState );
	
	/*
	//	macro Indexed_Primitive_List data, length, address, maxindex { ; Control ID Code: Indexed Primitive List (OpenGL)
	uint8_t Mode = Mode_Triangles | Index_Type_8;
	uint32_t IndexCount = VERTEX_COUNT;
	uint32_t MaxIndex = IndexCount - 1;
	addbyte(&p, 32);
	addbyte(&p, Mode);
	addword(&p, IndexCount);
	addword(&p, (uint32_t)VERTEX_INDEXES );
	addword(&p, MaxIndex);
	 */
	//	vertex array (no indexes)
#define PRIM_TRIANGLE	4
	uint32_t VertexCount = 3;
	uint32_t FirstIndex = 0;
	addbyte(&p, 33);
	addbyte(&p, PRIM_TRIANGLE);
	addword(&p, VertexCount );
	addword(&p, FirstIndex );
	

	
#if defined(BINTHREAD_FLUSH)
	addbyte(&p, 0x4);	//	flush
#endif
#if defined(BINTHREAD_FLUSHALL)
	addbyte(&p, 0x5);	//	flush
#endif
#if defined(BINTHREAD_NOPHALT)
	addbyte(&p, 1);	//	nop
	addbyte(&p, 0);	//	halt
#endif
	
	
	
	
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
	

	
	*GetV3dReg(V3D_CT0CA) = TKernel::GetTHREADAddress32(ProgramMemory);
	*GetV3dReg(V3D_CT0EA) = TKernel::GetTHREADAddress32(ProgramMemoryEnd);
	
	if ( !WaitForThread(0) )
		return false;
	

	
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
	addword( &Program, TKernel::GetSCREENBUFFERAddress32(mScreenBuffer) );
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
			addword( &Program, TKernel::GetTILEBINAddress32(Address) );
			
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
	
#if defined(RENDERTHREAD_FLUSH)
	addbyte(&Program, 0x4);	//	flush
#endif
#if defined(RENDERTHREAD_FLUSHALL)
	addbyte(&Program, 0x5);	//	flush
#endif
#if defined(RENDERTHREAD_NOPHALT)
	addbyte(&Program, 1);	//	nop
	addbyte(&Program, 0);	//	halt
#endif
	
	
	
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
		auto ErrorStat = ReadV3dReg( V3D_ERRSTAT );
		DrawString( GetConsoleX(), GetConsoleY(), "Thread1 Error=" );
		DrawHex( GetConsoleX(false), GetConsoleY(), ErrorStat );

		
#if defined(STOP_THREAD_ON_ERROR)
		{
			//	stop thread...
			auto Status = *GetV3dReg(V3D_CT1CS);
			Status |= (1<<5);
			*GetV3dReg(V3D_CT1CS) = Status;
		 }
#endif
#if defined(RESET_THREAD_ON_ERROR)
		{
			//	reset thread...
			auto Status = *GetV3dReg(V3D_CT1CS);
			Status = 1<<15;
			*GetV3dReg(V3D_CT1CS) = Status;
		}
#endif
		 return false;
	}
	

	*GetV3dReg(V3D_CT1CA) = TKernel::GetTHREADAddress32(ProgramMemory);
	*GetV3dReg(V3D_CT1EA) = TKernel::GetTHREADAddress32(ProgramMemoryEnd);
	
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
			Status = 1<<15;
			*GetV3dReg(V3D_CT1CS) = Status;
#endif
			return false;
		}
		
		if ( State == Finished )
			break;
		
		if ( State == Stalled )
		{
			DrawString( GetConsoleX(), GetConsoleY(), "thread 1 stalled" );
		}
		else
		{
			//DrawString( GetConsoleX(), GetConsoleY(), "waiting for thread 1" );
		}
		TKernel::Sleep(100);
	}
	
	//	explicit stop
	//*GetV3dReg(V3D_CT1CS) = 0x20;
	
	return true;
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
