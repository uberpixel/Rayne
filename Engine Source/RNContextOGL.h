//
//  RNContextOGL.h
//  Rayne
//
//  Created by Sidney Just on 13.12.12.
//  Copyright (c) 2012 Sidney Just. All rights reserved.
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
