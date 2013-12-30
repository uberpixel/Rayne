//
//  RNException.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNException.h"
#include "RNThread.h"

#if RN_PLATFORM_POSIX
#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <dlfcn.h>
#endif

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
		
#if RN_PLATFORM_POSIX
		size = backtrace(symbols, kRNExceptionMaxSymbols);
		std::string unknwonSymbol = std::string("<???>");
		
		for(size_t i = 1; i < size; i ++)
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
		size = ::CaptureStackBackTrace(0, kRNExceptionMaxSymbols, symbols, nullptr);
		std::string unknwonSymbol = std::string("<???>");
		
		for(size_t i = 1; i < size; i ++)
		{
			_callStack.push_back(std::pair<uintptr_t, std::string>((uintptr_t)symbols[i], unknwonSymbol));
		}
#endif
		
		_thread = Thread::GetCurrentThread();
	}
	
	
#define CaseType(type) \
	case type: \
		return #type ; \
		break;
	
	const char *Exception::GetStringifiedType() const
	{
		switch(_type)
		{
			CaseType(Type::GenericException)
			CaseType(Type::InvalidArgumentException)
			CaseType(Type::RangeException)
			CaseType(Type::InconsistencyException)
			CaseType(Type::DowncastException)
				
			CaseType(Type::ApplicationNotFoundException)
			CaseType(Type::NoCPUException)
			CaseType(Type::NoGPUException)
			CaseType(Type::NoContextException)
				
			CaseType(Type::ModuleNotFoundException)
			CaseType(Type::ModuleUnsupportedABIException)
			CaseType(Type::ModuleConstructFailedException)
				
			CaseType(Type::TextureFormatUnsupportedException)
			
			CaseType(Type::ShaderUnsupportedException)
			CaseType(Type::ShaderCompilationFailedException)
			CaseType(Type::ShaderLinkingFailedException)
				
			CaseType(Type::FramebufferException)
			CaseType(Type::FramebufferUnsupportedException)
			CaseType(Type::FramebufferIncompleteAttachmentException)
			CaseType(Type::FramebufferIncompleteMissingAttachmentException)
			CaseType(Type::FramebufferIncompleteDrawbufferException)
			CaseType(Type::FramebufferIncompleteDimensionsException)
			CaseType(Type::FramebufferIncompleteLayerException)
			CaseType(Type::FramebufferIncompleteMultisampleException)
		}
		
		return nullptr;
	}
}
