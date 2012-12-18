//
//  RNContextOGL.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONTEXTOGL_H__
#define __RAYNE_CONTEXTOGL_H__

#include "RNBase.h"
#include "RNContext.h"

namespace RN
{
	class ContextOGL : public Context
	{
	public:
		ContextOGL(ContextFlags flags, ContextOGL *shared=0);
        virtual ~ContextOGL();
		
		void Flush();
		
	protected:
		virtual void Activate();
		virtual void Deactivate();
		
	private:
		int32 _glsl;
		
#if RN_PLATFORM_MAC_OS
        ContextOGL *_shared;
        NSOpenGLContext *_oglContext;
#endif
	};
}

#endif /* __RAYNE_CONTEXTOGL_H__ */
