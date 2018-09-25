#pragma once

#define DUK_ASSERT(x)	do{ if(!x) Js::OnAssert();	}while(0)
#include "duktape-2.3.0/src/duktape.h"
#include <functional>

#include "Heap.h"

namespace Js
{
	class TContext;
	void	OnAssert();
}


class Js::TContext
{
public:
	TContext(THeap& Heap,std::function<void(const char*)> OnError);
	~TContext();
	
	void		Execute(const char* Script);
	
	void		BindPrint();
	
	THeap&		GetHeap()	{	return mHeap;	}
	
public:
	static std::function<void(const char*)>*	mPrint;
	duk_context*	mContext;
	std::function<void(const char*)>	mOnError;
	THeap&			mHeap;
};

