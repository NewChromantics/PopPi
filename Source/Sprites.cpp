#include "Sprites.h"


constexpr TColour32 TColour32::Transparent	( 0, 255, 0, 0 );
constexpr TColour32 TColour32::White		( 255, 255, 255, 255 );
constexpr TColour32 TColour32::Black		( 0, 0, 0, 255 );


constexpr TColour32 Palette[3] =
{
	TColour32::Transparent,
	TColour32::White,
	TColour32::Black,
};



#define SPRITE_WIDTH		5
#define SPRITE_HEIGHT		7

//	size + linefeed + ident
#define SPRITE_ATLAS_LFSIZE	1
#define SPRITE_ATLAS_PREFIX_SIZE	(SPRITE_ATLAS_LFSIZE)	//	ident+lf
#define SPRITE_ATLAS_BLOCK_PREFIX_SIZE	(1+SPRITE_ATLAS_LFSIZE)	//	ident+lf
#define SPRITE_ATLAS_BLOCK_SIZE	(SPRITE_ATLAS_BLOCK_PREFIX_SIZE+(SPRITE_WIDTH*SPRITE_HEIGHT)+(SPRITE_ATLAS_LFSIZE*SPRITE_HEIGHT))

constexpr const char RawSpriteAtlas[] =
#include "SpriteCharAtlas.inc"
;
static_assert( RawSpriteAtlas[0]=='\n', "0" );
static_assert( RawSpriteAtlas[SPRITE_ATLAS_PREFIX_SIZE]=='0', "0" );
static_assert( RawSpriteAtlas[SPRITE_ATLAS_PREFIX_SIZE+SPRITE_ATLAS_BLOCK_PREFIX_SIZE]=='_', "_");
static_assert( RawSpriteAtlas[3]=='_', "_");


template<size_t N>
constexpr size_t length(char const (&)[N])
{
	return N-1;
}

constexpr int GetSpriteAtlasSize()
{
	return length(RawSpriteAtlas);
}

const uint8_t* SpriteLookup[256];
char TranslateAtlas(char AtlasChar)
{
	switch(AtlasChar)
	{
		case '_':	return 2;
		case 'X':	return 1;
		case 'O':	return 0;
		default:
			return 0;
	}
}


void ParseSpriteAtlas(char* Atlas)
{
	auto GetRawAtlasChar = [Atlas](int x,int y) -> char
	{
		int i = x;
		i += y * (SPRITE_WIDTH+SPRITE_ATLAS_LFSIZE);
		return Atlas[i];
	};

	for ( int y=0;	y<SPRITE_HEIGHT;	y++ )
	{
		for ( int x=0;	x<SPRITE_WIDTH;	x++ )
		{
			auto AtlasChar = GetRawAtlasChar(x,y);
			AtlasChar = TranslateAtlas(AtlasChar);
			int i = x + (y*SPRITE_WIDTH);
			Atlas[i] = AtlasChar;
		}
	}
}

uint8_t Dummy[SPRITE_WIDTH*SPRITE_HEIGHT] =
{
	2,2,2,2,2,
	2,1,2,1,2,
	2,2,1,2,2,
	2,1,2,1,2,
	2,2,1,2,2,
	2,1,2,1,2,
	2,2,2,2,2,
};

//	gr: this isn't initialising to 0
int SpriteLookupInitialised = 0;

void ParseSpriteAtlas()
{
	const int MAGIC = 65154;

	if ( SpriteLookupInitialised == MAGIC )
		return;
	
	//	init
	for ( int i=0;	i<256;	i++ )
		SpriteLookup[i] = nullptr;
	SpriteLookup['?'] = Dummy;
	
	for ( int a=0;	a<12;	a++ )
	{
		auto* Atlas = (char*)RawSpriteAtlas;
		Atlas += SPRITE_ATLAS_PREFIX_SIZE;
		Atlas += a * SPRITE_ATLAS_BLOCK_SIZE;

		auto AtlasSpriteChar = *Atlas;
		auto* AtlasIndexStart = Atlas + SPRITE_ATLAS_BLOCK_PREFIX_SIZE;
		ParseSpriteAtlas( AtlasIndexStart );
		SpriteLookup[(int)AtlasSpriteChar] = (const uint8_t*)AtlasIndexStart;
		
		if ( AtlasSpriteChar >= 'A' && AtlasSpriteChar <= 'Z' )
			SpriteLookup[(int)AtlasSpriteChar - 'A'] = (const uint8_t*)AtlasIndexStart;
		if ( AtlasSpriteChar >= 'a' && AtlasSpriteChar <= 'z' )
			SpriteLookup[(int)AtlasSpriteChar - 'a'] = (const uint8_t*)AtlasIndexStart;
		
	}
	
	for ( int i=0;	i<256;	i++ )
	{
		if ( !SpriteLookup[i] )
			SpriteLookup[i] = SpriteLookup['?'];
	}
	
	SpriteLookupInitialised = MAGIC;
}



const TColour32 PopSprite::GetPaletteColour(uint8_t PaletteIndex)
{
	return Palette[PaletteIndex];
}


const uint8_t* PopSprite::GetSprite(char Char,int& Width,int& Height)
{
	ParseSpriteAtlas();
	auto* Sprite = SpriteLookup[(int)Char];
	Width = SPRITE_WIDTH;
	Height = SPRITE_HEIGHT;
	return (uint8_t*)Sprite;
}
