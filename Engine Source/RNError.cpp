//
//  RNError.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNError.h"
#include "RNThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <dlfcn.h>

#define kRNErrorExceptionMaxSymbols 30

namespace RN
{
	ErrorException::ErrorException(ErrorCode error, const std::string& description, const std::string& detail) :
		_description(description),
		_additionalDetail(detail)
	{
		_error = error;
		GatherInfo();
	}
	
	ErrorException::ErrorException(uint32 group, uint32 subgroup, uint32 code, const std::string& description, const std::string& detail) :
		_description(description),
		_additionalDetail(detail)
	{
		_error = RNErrorGroup(group) | RNErrorSubgroup(subgroup) | code;
		GatherInfo();
	}
	
	
	void ErrorException::GatherInfo()
	{
		void *symbols[kRNErrorExceptionMaxSymbols];
		size_t size;
		
		size = backtrace(symbols, kRNErrorExceptionMaxSymbols);
		std::string unknwonSymbol = std::string("<???>");
		
		for(size_t i=0; i<size; i++)
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
		
		_thread = Thread::CurrentThread();
	}
}
