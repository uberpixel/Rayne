//
//  RNMetalRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERER_H__
#define __RAYNE_METALRENDERER_H__

#include "../../Base/RNBase.h"
#include "../RNRenderer.h"
#include "RNMetalWindow.h"

namespace RN
{
	struct MetalRendererInternals;
	class MetalWindow;

	class MetalRenderer : public Renderer
	{
	public:
		friend class MetalWindow;

		RNAPI MetalRenderer();

		RNAPI Window *CreateWindow(const Rect &frame, Screen *screen) final;
		RNAPI Window *GetMainWindow();

		RNAPI void BeginWindow(Window *window) final;
		RNAPI void EndWindow() final;

		RNAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUBuffer::Options options);
		RNAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUBuffer::Options options);

	protected:
		PIMPL<MetalRendererInternals> _internals;
		MetalWindow *_mainWindow;
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */
