//
//  RNRenderStorage.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		RNAPI RenderStorage(BufferFormat format, Texture *depthTexture=0, float scaleFactor=0.0f);
		RNAPI virtual ~RenderStorage();
		
		RNAPI void SetSize(const Vector2& size);
		RNAPI void SetRenderTarget(Texture *target, uint32 index=0);
		RNAPI void AddRenderTarget(Texture *target);
		RNAPI void AddRenderTarget(Texture::Format format);
		RNAPI void RemoveRenderTarget(Texture *target);
		RNAPI void SetDepthTarget(Texture *texture, uint32 layer=-1);
		RNAPI void SetDepthTarget(Texture::Format format);
		
		RNAPI void UpdateBuffer();
		
		RNAPI void Bind();
		RNAPI void Unbind();
		
		bool HasDepthbuffer() const { return (_format & BufferFormatDepth); }
		bool HasStencilbuffer() const { return (_format & BufferFormatStencil); }
		
		uint32 GetRenderTargets() const { return (uint32)_renderTargets->GetCount(); }
		Texture *GetRenderTarget(uint32 index=0) const { return _renderTargets->GetObjectAtIndex<Texture>(index); }
		Texture *GetDepthTarget() const { return _depthTexture; }
		const Vector2& GetSize() const { return _size; }
		
		RNAPI static uint32 GetMaxRenderTargets();
		
	private:
		void UpdateDrawBuffers(uint32 count);
		void CheckFramebufferStatus();
		
		GLuint _framebuffer;
		GLuint _depthbuffer;
		GLuint _stencilbuffer;
		
		Vector2 _size;
		BufferFormat _format;
		
		Array *_renderTargets;
		uint32 _boundRenderTargets;
		Texture *_depthTexture;
		uint32 _depthLayer;
		
		bool _sizeChanged;
		bool _formatChanged;
		bool _renderTargetsChanged;
		float _scaleFactor;
		bool _fixedScaleFactor;
		
		RNDefineMeta(RenderStorage, Object)
	};
}

#endif
