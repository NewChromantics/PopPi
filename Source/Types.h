#pragma once


#include <stdint.h>
#include <functional>


//	display is BGRA
#define BGRA(r,g,b,a)		( (uint32_t(r)<<0) | (uint32_t(g)<<8) | (uint32_t(b)<<16) | (uint32_t(a)<<24) )
#define RGBA(r,g,b,a)		( BGRA(b,g,r,a) )



template<typename T>
inline constexpr bool bool_cast(const T& v)
{
	return v != 0;
}

namespace  Math
{
	template<typename T>
	T	Min(T a,T b)		{	return (a<b) ? a : b;	}
	
	template<typename T>
	T	Max(T a,T b)		{	return (a>b) ? a : b;	}
}


template<typename TYPE>
struct TVector2
{
public:
	TVector2(TYPE x,TYPE y) :
		x	(x),
		y	(y)
	{
	}
	TVector2() :
		x	(0),
		y	(0)
	{
	}
	
	TYPE	x;
	TYPE	y;
};

typedef TVector2<uint32_t> uint32_2;
typedef TVector2<uint16_t> uint16_2;

#define MaskBits(Value,Size)			( (Value) & ((1<<(Size))-1) )
//#define Fixed12_4( Integer, Fraction )	( (MaskBits(Integer,12)<<4) | MaskBits(Fraction,4) )
#define Fixed12_4( Integer, Fraction )	( Integer*16 )


#if !defined(__EXCEPTIONS)
namespace std
{
	inline void __throw_bad_function_call() { while(1); };
}
#endif




inline void addbyte(uint8_t **list, uint8_t d)
{
	*((*list)++) = d;
}
/*
inline void addshort(uint8_t **list, uint16_t d) {
	*((*list)++) = (d >> 8) & 0xff;
	*((*list)++) = (d)  & 0xff;
 }
 
inline void addword(uint8_t **list, uint32_t d) {
	*((*list)++) = (d >> 24) & 0xff;
	*((*list)++) = (d >> 16)  & 0xff;
	*((*list)++) = (d >> 8) & 0xff;
	*((*list)++) = (d >> 0) & 0xff;
 }
 */
inline void addshort(uint8_t **list, uint16_t d)
{
	*((*list)++) = (d) & 0xff;
	*((*list)++) = (d >> 8)  & 0xff;
}

inline void addword(uint8_t **list, uint32_t d)
{
	*((*list)++) = (d) & 0xff;
	*((*list)++) = (d >> 8)  & 0xff;
	*((*list)++) = (d >> 16) & 0xff;
	*((*list)++) = (d >> 24) & 0xff;
}

inline void addfloat(uint8_t **list, float f)
{
	uint32_t d = *((uint32_t *)&f);
	*((*list)++) = (d) & 0xff;
	*((*list)++) = (d >> 8)  & 0xff;
	*((*list)++) = (d >> 16) & 0xff;
	*((*list)++) = (d >> 24) & 0xff;
}


inline uint16_t EndianSwap16(uint16_t v)
{
	return v;
	uint16_t x = 0;
	uint8_t* p = reinterpret_cast<uint8_t*>( &x );
	addshort( &p, v );
	return x;
}

inline uint32_t EndianSwap32(uint32_t v)
{
	return v;
	uint32_t x = 0;
	uint8_t* p = reinterpret_cast<uint8_t*>( &x );
	addword( &p, v );
	return x;
}

inline uint32_t EndianSwapFloat(float v)
{
	static_assert( sizeof(float) == sizeof(uint32_t), "Expecting 32bit float");
	uint32_t* f32 = reinterpret_cast<uint32_t*>(&v);
	return *f32;
	uint32_t x = 0;
	uint8_t* p = reinterpret_cast<uint8_t*>( &x );
	addfloat( &p, v );
	return x;
}

