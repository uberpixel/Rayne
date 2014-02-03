//
//  RNException.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EXCEPTION_H__
#define __RAYNE_EXCEPTION_H__

#include <string>
#include <vector>

#include "RNDefines.h"

namespace RN
{
	class Thread;
	class Exception
	{
	public:
		enum class Type
		{
			GenericException,
			InvalidArgumentException,
			RangeException,
			InconsistencyException,
			DowncastException,
			
			ApplicationNotFoundException,
			NoCPUException,
			NoGPUException,
			NoContextException,
			
			ModuleNotFoundException,
			ModuleUnsupportedABIException,
			ModuleConstructFailedException,
			
			TextureFormatUnsupportedException,
			
			ShaderUnsupportedException,
			ShaderCompilationFailedException,
			ShaderLinkingFailedException,
			
			FramebufferException,
			FramebufferUnsupportedException,
			FramebufferIncompleteAttachmentException,
			FramebufferIncompleteMissingAttachmentException,
			FramebufferIncompleteDrawbufferException,
			FramebufferIncompleteMultisampleException,
			FramebufferIncompleteLayerException,
			FramebufferIncompleteDimensionsException
		};
		
		RNAPI Exception(Type type, const std::string& reason);
		RNAPI Exception(Type type, const char *format, ...);
		
		Type GetType() const { return _type; }
		Thread *GetThread() const { return _thread; }
		
		RNAPI const char *GetStringifiedType() const;
		const std::string& GetReason() const { return _reason; }
		const std::vector<std::pair<uintptr_t, std::string>>& GetCallStack() const { return _callStack; }
		
	private:
		void GatherInfo();
		
		Type _type;
		std::string _reason;
		
		Thread *_thread;
		std::vector<std::pair<uintptr_t, std::string>> _callStack;
	};
}


#endif /* __RAYNE_EXCEPTION_H__ */
