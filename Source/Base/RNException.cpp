//
//  RNException.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNException.h"
#include "../Threads/RNThread.h"

#if RN_PLATFORM_POSIX
#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <dlfcn.h>
#endif

#if RN_PLATFORM_WINDOWS
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
#endif

#define kRNExceptionMaxSymbols 64

namespace RN
{
	Exception::Exception(const std::string &reason) :
		_reason(reason)
	{
		GatherInfo();
	}
	
	Exception::Exception(const String *reason) :
		_reason(reason->GetUTF8String())
	{
		GatherInfo();
	}


	
	RN_NOINLINE void Exception::GatherInfo()
	{
		void *symbols[kRNExceptionMaxSymbols];
		size_t size;
		
		std::string unknwonSymbol("<..>");
		
#if RN_PLATFORM_POSIX
		size = backtrace(symbols, kRNExceptionMaxSymbols);
		
		for(size_t i = 2; i < size; i ++)
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
#endif
		
#if RN_PLATFORM_WINDOWS
		HANDLE process = ::GetCurrentProcess();
		
		static std::once_flag flag;
		std::call_once(flag, [&]() {
			::SymInitialize(process, nullptr, true);
		});
		
		size = ::CaptureStackBackTrace(0, kRNExceptionMaxSymbols, symbols, nullptr);
		
		SYMBOL_INFO *symbol  = reinterpret_cast<SYMBOL_INFO *>(calloc(sizeof(SYMBOL_INFO) + 256, 1));
		symbol->MaxNameLen   = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		
		for(size_t i = 2; i < size; i ++)
		{
			::SymFromAddr(process, reinterpret_cast<DWORD64>(symbols[i]), 0, symbol);
			
			std::string name = (symbol->NameLen == 0) ? unknwonSymbol : std::string(symbol->Name);
			_callStack.push_back(std::pair<uintptr_t, std::string>((uintptr_t)symbols[i], std::move(name)));
		}
#endif
		
		_thread = Thread::GetCurrentThread();
		
		if(_reason.empty())
			_reason = "Jabberwock is killing user";
	}
}
