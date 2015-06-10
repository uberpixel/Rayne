//
//  RNRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERER_H_
#define __RAYNE_RENDERER_H_

#include "../Base/RNBase.h"
#include "../System/RNScreen.h"
#include "RNWindow.h"
#include "RNGPUBuffer.h"

namespace RN
{
	class Renderer : public Object
	{
	public:
		RNAPI static Renderer *GetActiveRenderer();

		RNAPI virtual Window *CreateWindow(const Rect &frame, Screen *screen) = 0;
		RNAPI virtual Window *GetMainWindow() = 0;

		RNAPI void Activate();

		RNAPI virtual void BeginWindow(Window *window) = 0;
		RNAPI virtual void EndWindow() = 0;

		RNAPI virtual GPUBuffer *CreateBufferWithLength(size_t length, GPUBuffer::Options options) = 0;
		RNAPI virtual GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUBuffer::Options options) = 0;

	protected:
		RNAPI Renderer();

		RNDeclareMeta(Renderer)
	};
}


#endif /* __RAYNE_RENDERER_H_ */
