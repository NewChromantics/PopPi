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



#if !defined(__EXCEPTIONS)
namespace std
{
	inline void __throw_bad_function_call() { while(1); };
}
#endif

