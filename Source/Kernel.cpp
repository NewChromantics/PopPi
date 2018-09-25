#include "Kernel.h"
#include "Mailbox.h"

std::function<void(const char*)>* gPrintFunc = nullptr;

void TKernel::DebugLog(const char* String)
{
	if ( !gPrintFunc )
		return;

	(*gPrintFunc)( String );
}

