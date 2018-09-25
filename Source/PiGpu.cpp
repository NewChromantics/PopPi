#include "PiGpu.h"

/*
#define TILE_BIN_PADDING	0

uint8_t gTileBin[MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_BIN_BLOCK_SIZE * sizeof(TTileBin) + TILE_BIN_PADDING]  __attribute__ ((aligned(16)));
uint8_t gTileState[MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_STRUCT_SIZE]  __attribute__ ((aligned(16)));
//static volatile uint32_t* TileBin = (uint32_t*)0x00400000;
//static volatile uint8_t* TileState = (uint8_t*)00500000;
uint8_t gProgram0[0x1000]  __attribute__ ((aligned(4)));
uint8_t gProgram1[0x1000]  __attribute__ ((aligned(4)));


void GpuLoop(TDisplay& Display)
{
	Display.SetupGpu();

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
	TALLOCMemory TileBins( MAX_TILE_WIDTH*MAX_TILE_HEIGHT*TILE_BIN_BLOCK_SIZE * sizeof(TTileBin) + TILE_BIN_PADDING, !USE_BIG_ALLOCATION );
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

	int DrawTickFrequency = 100;
	
	
	
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
	
	auto* Program0End = Display.SetupBinControl( Program0Mem, TileBinMem, TileBins.GetSize(), TileStateMem );
	auto* Program1End = Display.SetupRenderControl( Program1Mem, TileBinMem );

	
	uint32_t Tick = 0;
	while ( true )
	{
		
		if ( DrawTickFrequency > 0 )
		{
			if ( Tick % DrawTickFrequency == 0  )
			{
				//Display.DrawString( Display.GetConsoleX(), Display.mHeight-9, "beep! tick x ");
				//Display.DrawNumber( Display.GetConsoleX(false), Display.GetConsoleY(), Tick );
				//Context.Execute("print(\"hello!\");");
			}
		}
		
		//Display.mClearColour = RGBA( (Tick&1) * 255, 0, 255, 255 );
		Display.mClearColour = RGBA( Tick%255, (Tick/10)%255, (Tick/100)%255, 255 );
		
		Program1End = Display.SetupRenderControl( Program1Mem, TileBinMem );

		
		if ( !Display.ExecuteThread( Program0Mem, Program0End, 0 ) )
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Bin thread failed");
			TKernel::Sleep(10);
		}

		if ( !Display.ExecuteThread( Program1Mem, Program1End, 1 ) )
		{
			Display.DrawString( Display.GetConsoleX(), Display.GetConsoleY(), "Render thread failed");
			TKernel::Sleep(10);
		}

		
		Tick++;
	}


}
*/


