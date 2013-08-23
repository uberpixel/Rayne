//
//  RNException.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "RNException.h"
#include "RNThread.h"

#define kRNExceptionMaxSymbols 32

namespace RN
{
	Exception::Exception(Type type, const std::string& reason) :
		_type(type),
		_reason(reason)
	{
		GatherInfo();
	}
	
	void Exception::GatherInfo()
	{
		void *symbols[kRNExceptionMaxSymbols];
		size_t size;
		
		size = backtrace(symbols, kRNExceptionMaxSymbols);
		std::string unknwonSymbol = std::string("<???>");
		
		for(size_t i=1; i<size; i++)
		{
			Dl_info info;
			int status = dladdr(symbols[i], &info);
			
			if(status != 0)
			{
				const char *symbol = abi::__cxa_demangle((char *)info.dli_sname, 0, 0, 0);
				std::string symbolname = symbol ? std::string(symbol) : unknwonSymbol;
				
				_callStack.push_back(std::pair<uintptr_t, std::string>((uintptr_t)symbols[i], symbolname));
				
				if(symbol)
					free((char *)symbol);
			}
			else
			{
				_callStack.push_back(std::pair<uintptr_t, std::string>((uintptr_t)symbols[i], unknwonSymbol));
			}
			
		}
		
		_thread = Thread::GetCurrentThread();
	}
}
