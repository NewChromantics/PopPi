#pragma once

#include "Kernel.h"


#define BGRA(r,g,b,a)		( (uint32_t(r)<<0) | (uint32_t(g)<<8) | (uint32_t(b)<<16) | (uint32_t(a)<<24) )
#define RGBA(r,g,b,a)		( BGRA(b,g,r,a) )

//	template this later
class TColour32
{
	typedef uint8_t COMPONENT;
	typedef uint32_t STORAGE;
	const static COMPONENT MAX = 255;

public:
	static const TColour32	Transparent;
	static const TColour32	White;
	static const TColour32	Black;

public:
	constexpr TColour32(COMPONENT Red,COMPONENT Green,COMPONENT Blue,COMPONENT Alpha=MAX) :
		bgra	( RGBA(Red,Green,Blue,Alpha) )
	{
	}
	
	bool		IsOpaque() const	{	return bool_cast(bgra & RGBA(0,0,0,255));	}
	
public:
	STORAGE	bgra;
};

static_assert( sizeof(TColour32) == sizeof(uint32_t), "Colour is not 32 bit in memory");



namespace PopSprite
{
	const TColour32		GetPaletteColour(uint8_t PaletteIndex);
	const uint8_t*			GetSprite(char Char,int& Width,int& Height);
}
