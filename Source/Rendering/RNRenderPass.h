//
//  RNRenderPass.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERPASS_H__
#define __RAYNE_RENDERPASS_H__

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Math/RNRect.h"
#include "RNFramebuffer.h"

namespace RN
{
	class RenderPass : public Object
	{
	public:
		RN_OPTIONS(Flags, uint32,
			ClearColor = (1 << 0),
			LoadColor = (1 << 1),
			StoreColor = (1 << 2),
			ClearDepthStencil = (1 << 3),
			LoadDepthStencil = (1 << 4),
			StoreDepthStencil = (1 << 5),
			Defaults = ClearDepthStencil|StoreColor);

		RNAPI RenderPass();
		RNAPI ~RenderPass();

		RNAPI void SetFramebuffer(Framebuffer *framebuffer);
		RNAPI void SetFlags(Flags flags);
		RNAPI void SetFrame(const Rect &frame);
		RNAPI void SetClearColor(const Color &color);
		RNAPI void SetClearDepthStencil(float depth, uint8 stencil);

		RNAPI Framebuffer *GetFramebuffer() const;
		Flags GetFlags() const { return _flags; }
		RNAPI Rect GetFrame() const;
		const Color &GetClearColor() const { return _clearColor; }
		float GetClearDepth() const { return _clearDepth; }
		uint8 GetClearStencil() const { return _clearStencil; }

		RNAPI void AddRenderPass(RenderPass *renderPass) const;
		RNAPI void RemoveRenderPass(RenderPass *renderPass) const;
		RNAPI void RemoveAllRenderPasses() const;
		const Array *GetNextRenderPasses() const { return _nextRenderPasses; }

	private:
		Flags _flags;
		Rect _frame;
		Framebuffer *_framebuffer;
		Color _clearColor;
		float _clearDepth;
		uint8 _clearStencil;

		Array *_nextRenderPasses;

		__RNDeclareMetaInternal(RenderPass)
	};
}


#endif /* __RAYNE_RENDERPASS_H__ */
