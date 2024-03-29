//
//  RNRenderPass.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderPass.h"
#include "../Rendering/RNRenderer.h"

namespace RN
{
	RNDefineMeta(RenderPass, Object)

	RenderPass::RenderPass() : _flags(Flags::Defaults), _framebuffer(nullptr), _clearDepth(0.0f), _clearStencil(0), _nextRenderPasses(new Array())
	{
		
	}

	RenderPass::~RenderPass()
	{
		SafeRelease(_framebuffer);
	}

	// Setter
	void RenderPass::SetFramebuffer(Framebuffer *framebuffer)
	{
		SafeRelease(_framebuffer);
		_framebuffer = framebuffer->Retain();
	}

	void RenderPass::SetFlags(Flags flags)
	{
		_flags = flags;
	}

	void RenderPass::SetFrame(const Rect &frame)
	{
		_frame = std::move(frame.GetIntegral());
	}

	void RenderPass::SetClearColor(const Color &color)
	{
		_clearColor = color;
	}

	void RenderPass::SetClearDepthStencil(float depth, uint8 stencil)
	{
		_clearDepth = depth;
		_clearStencil = stencil;
	}

	//Getter
	Framebuffer *RenderPass::GetFramebuffer() const
	{
		if(!_framebuffer)
		{
			return Renderer::GetActiveRenderer()->GetMainWindow()->GetFramebuffer();
		}

		return _framebuffer;
	}

	Rect RenderPass::GetFrame() const
	{
		if(std::abs(_frame.GetArea()) > 0.0001)
		{
			return _frame;
		}

		if(_framebuffer)
		{
			Rect frame(Vector2(), _framebuffer->GetSize());
			return frame;
		}

		Renderer *renderer = Renderer::GetActiveRenderer();
		Vector2 mainWindowSize = renderer->GetMainWindow()->GetSize();
		Rect windowFrame(Vector2(), mainWindowSize);
		return windowFrame;
	}

	void RenderPass::AddRenderPass(RenderPass *renderPass) const
	{
		_nextRenderPasses->AddObject(renderPass);
	}

	void RenderPass::RemoveRenderPass(RenderPass *renderPass) const
	{
		_nextRenderPasses->RemoveObject(renderPass);
	}

	void RenderPass::RemoveAllRenderPasses() const
	{
		_nextRenderPasses->RemoveAllObjects();
	}
}
