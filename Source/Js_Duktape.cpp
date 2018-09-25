#include "Js_Duktape.h"
#include "Debug.h"

extern "C" void	Js_Debug(int level,const char* Filename,int Line, const char* Func,const char* Message)
{
	//Debug::Print( Func );
	//Debug::Print( Message );
}


std::function<void(const char*)>* Js::TContext::mPrint = nullptr;


void OnError(void* pContext,const char* Error)
{
	if ( !pContext )
		return;
	auto& Context = *reinterpret_cast<Js::TContext*>(pContext);
	Context.mOnError( Error );
}


void* Alloc(void* pContext,duk_size_t Size)
{
	auto& Context = *reinterpret_cast<Js::TContext*>(pContext);
	auto& Heap = Context.GetHeap();
	return Heap.Alloc( Size );
}

void* Realloc(void* pContext,void* OldData,duk_size_t Size)
{
	return Alloc( pContext, Size );
}


void Free(void* pContext,void* Data)
{
	auto& Context = *reinterpret_cast<Js::TContext*>(pContext);
	auto& Heap = Context.GetHeap();
	return Heap.Free( reinterpret_cast<uint8_t*>(Data) );
}


Js::TContext::TContext(THeap& Heap,std::function<void(const char*)> OnError) :
	mContext	( nullptr ),
	mOnError	( OnError ),
	mHeap		( Heap )
{
	if ( mOnError == nullptr )
		mOnError = [](const char* Error)	{};

	
	
	//mContext = duk_create_heap_default();
	mContext = duk_create_heap( Alloc, Realloc, Free, this, ::OnError );
	
	/*
	auto Print = [](duk_context* Context) ->duk_ret_t
	{
		auto& PrintFunc = *Js::TContext::mPrint;
		PrintFunc( "I am Print()!");
		return 0;
	};
	 
	duk_push_c_function( mContext, Print, 3);
	duk_put_global_string( mContext, "print");
	*/
	OnError("Context initialised");
}

Js::TContext::~TContext()
{
	duk_destroy_heap( mContext );
}

void Js::TContext::Execute(const char* Script)
{
	duk_eval_string( mContext, Script );
	duk_pop( mContext );	//	pop eval result
}


