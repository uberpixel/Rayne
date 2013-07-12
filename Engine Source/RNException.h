//
//  RNException.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EXCEPTION_H__
#define __RAYNE_EXCEPTION_H__

#include <string>
#include <vector>

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
			ModuleNotFoundException,
			NoCPUException,
			NoGPUException,
			NoContextException,
			
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
		
		Exception(Type type, const std::string& reason);
		
		Type ExceptionType() const { return _type; }
		Thread *ExceptionThread() const { return _thread; }
		
		const std::string& Reason() const { return _reason; }
		const std::vector<std::pair<uintptr_t, std::string>>& CallStack() const { return _callStack; }
		
	private:
		void GatherInfo();
		
		Type _type;
		std::string _reason;
		
		Thread *_thread;
		std::vector<std::pair<uintptr_t, std::string>> _callStack;
	};
}


#endif /* __RAYNE_EXCEPTION_H__ */