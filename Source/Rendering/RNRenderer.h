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
#include "../Objects/RNWeakStorage.h"
#include "../System/RNScreen.h"
#include "RNWindow.h"
#include "RNGPUBuffer.h"
#include "RNShaderLibrary.h"
#include "RNTexture.h"

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

		RNAPI virtual bool SupportsTextureFormat(Texture::Descriptor::Format format) = 0;

		RNAPI virtual GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions options) = 0;
		RNAPI virtual GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options) = 0;

		RNAPI virtual ShaderLibrary *GetShaderLibraryWithFile(const String *file) = 0;
		RNAPI virtual ShaderLibrary *GetShaderLibraryWithSource(const String *source) = 0;

		RNAPI virtual Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) = 0;

	protected:
		RNAPI Renderer();

		RNDeclareMeta(Renderer)
	};

	RNExceptionType(ShaderCompilation)
}


#endif /* __RAYNE_RENDERER_H_ */
