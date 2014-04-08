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

#if RN_PLATFORM_WINDOWS
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
#endif

#define kRNExceptionMaxSymbols 64

namespace RN
{
	Exception::Exception(Type type, const std::string& reason) :
		_type(type),
		_reason(reason)
	{
		GatherInfo();
	}
	
	Exception::Exception(Type type, const char *format, ...) :
		_type(type)
	{
		char buffer[1024];
		va_list args;
		
		va_start(args, format);
		vsnprintf(buffer, 1024, format, args);
		va_end(args);
		
		buffer[1023] = '\0';
		
		_reason = buffer;
		
		GatherInfo();
	}
	
	RN_NOINLINE void Exception::GatherInfo()
	{
		void *symbols[kRNExceptionMaxSymbols];
		size_t size;
		
		std::string unknwonSymbol = std::string("<???>");
		
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
