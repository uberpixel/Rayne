//
//  RNContext.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONTEXT_H__
#define __RAYNE_CONTEXT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThread.h"
#include "RNColor.h"

namespace RN
{
	class Window;
	struct ContextInternals;
	
	class Context : public Object
	{
	public:
		friend class Window;
		friend class Kernel;
		friend class Thread;
		
#if RN_PLATFORM_WINDOWS
		RNAPI Context(Context *shared, HWND window = nullptr);
#else
		RNAPI Context(Context *shared);
#endif
		RNAPI Context(gl::Version version);
		
		RNAPI ~Context() override;

		RNAPI void MakeActiveContext();
		RNAPI void DeactivateContext();
		
		RNAPI void SetDepthClear(GLfloat depth);
		RNAPI void SetStencilClear(GLint stencil);
		RNAPI void SetClearColor(const Color& color);
		
		gl::Version GetVersion() const { return _version; }
		RNAPI static Context *GetActiveContext();

	protected:
		RNAPI void Activate();
		RNAPI void Deactivate();

	private:
		void Initialize(gl::Version version);
		void ForceDeactivate();
		
		gl::Version _version;
		
		bool _active;
		Thread *_thread;
		Context *_shared;
		bool _firstActivation;
		
		GLfloat _depthClear;
		GLint _stencilClear;
		Color _clearColor;

		PIMPL<ContextInternals> _internals;
		
		RNDefineMeta(Context, Object)
	};
}

#endif /* __RAYNE_CONTEXT_H__ */
