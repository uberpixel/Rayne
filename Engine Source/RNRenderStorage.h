//
//  RNRenderStorage.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERSTORAGE_H__
#define __RAYNE_RENDERSTORAGE_H__

#include "RNBase.h"
#include "RNTexture.h"
#include "RNArray.h"

namespace RN
{
	class Camera;
	class RenderStorage : public Object
	{
	friend class Camera;
	public:
		enum
		{
			BufferFormatColor = (1 << 0),
			BufferFormatDepth = (1 << 1),
			BufferFormatStencil = (1 << 2),
			
			BufferFormatComplete = (BufferFormatColor | BufferFormatDepth | BufferFormatStencil)
		};
		typedef uint32 BufferFormat;
		
		RNAPI RenderStorage(BufferFormat format, Texture *depthTexture=0);
		RNAPI virtual ~RenderStorage();
		
		RNAPI void SetFrame(const Rect& frame);
		RNAPI void SetRenderTarget(Texture *target, uint32 index=0);
		RNAPI void AddRenderTarget(Texture *target);
		RNAPI void AddRenderTarget(Texture::Format format);
		RNAPI void RemoveRenderTarget(Texture *target);
		RNAPI void SetDepthTarget(Texture *texture);
		RNAPI void SetDepthTarget(Texture::Format format);
		
		RNAPI void UpdateBuffer();
		
		bool HasDepthbuffer() const { return (_format & BufferFormatDepth); }
		bool HasStencilbuffer() const { return (_format & BufferFormatStencil); }
		
		uint32 RenderTargets() const { return (uint32)_renderTargets->Count(); }
		Texture *RenderTarget(uint32 index=0) const { return _renderTargets->ObjectAtIndex(index); }
		Texture *DepthTarget() const { return _depthTexture; }
		
		RNAPI static uint32 MaxRenderTargets();
		
	private:
		void UpdateDrawBuffers(uint32 count);
		void CheckFramebufferStatus();
		
		GLuint _framebuffer;
		GLuint _depthbuffer;
		GLuint _stencilbuffer;
		
		Rect _frame;
		BufferFormat _format;
		
		Array<Texture> *_renderTargets;
		uint32 _boundRenderTargets;
		Texture *_depthTexture;
		
		bool _frameChanged;
		bool _formatChanged;
		bool _renderTargetsChanged;
		float _scaleFactor;
		
		RNDefineConstructorlessMeta(RenderStorage, Object)
	};
}

#endif
