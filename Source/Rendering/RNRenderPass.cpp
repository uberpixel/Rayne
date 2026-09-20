//
//  RNRenderPass.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderPass.h"
#include "RNPostProcessing.h"
#include "../Rendering/RNRenderer.h"

namespace RN
{
	RNDefineMeta(RenderPass, FramePass)

    RenderPass::RenderPass(bool isSubpass) :
        _flags(Flags::Defaults), _framebuffer(nullptr), _clearDepth(0.0f), _clearStencil(0), _isSubpass(isSubpass), _isRoot(false), _renderGroupMask(0xffff), _subpassWritesDepthStencil(false), _subpassReadDepthStencilAttachment(false), _shaderHint(Shader::UsageHint::Default), _viewMode(ViewMode::Auto), _overrideMaterial(nullptr), _subpassNeedToStoreDepthStencil(false), _subpassNeedToPreserveDepthStencil(false), _subpassLastDepthStencilWrite(false), _subpassFirstDepthStencilWrite(false), _depthFirstUseIsRead(false), _depthLastUseIsRead(false), _subpassIndex(0), _subpassCount(0), _renderResources(nullptr), _drawSnapshotVersion(1)
	{
	}

	RenderPass::~RenderPass()
	{
		if(_renderResources)
			_renderResources->Delete();
		SafeRelease(_framebuffer);
		SafeRelease(_overrideMaterial);
	}

	void RenderPass::MarkDrawSnapshotDirty()
	{
		_drawSnapshotVersion += 1;
		GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
			RenderPass *renderPass = nextPass->Downcast<RenderPass>();
			if(renderPass && renderPass->GetIsSubpass())
				renderPass->MarkDrawSnapshotDirty();
		});
	}

	// Setter
	void RenderPass::SetFramebuffer(Framebuffer *framebuffer)
	{
		RN_ASSERT(!_isSubpass, "Cannot set framebuffer for subpass");

		SafeRelease(_framebuffer);
		_framebuffer = framebuffer->Retain();
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetFlags(Flags flags)
	{
		_flags = flags;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetFrame(const Rect &frame)
	{
		RN_ASSERT(!_isSubpass, "Cannot set frame for subpass");
		_frame = std::move(frame.GetIntegral());
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetClearColor(const Color &color)
	{
		RN_ASSERT(!_isSubpass, "Cannot set clear color for subpass");
		_clearColor = color;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetClearDepthStencil(float depth, uint8 stencil)
	{
		RN_ASSERT(!_isSubpass, "Cannot set clear depth stencil for subpass");
		_clearDepth = depth;
		_clearStencil = stencil;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetRenderGroupMask(uint16 mask)
	{
		_renderGroupMask = mask;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetShaderHint(Shader::UsageHint hint)
	{
		_shaderHint = hint;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetViewMode(ViewMode viewMode)
	{
		_viewMode = viewMode;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetOverrideMaterial(Material *material)
	{
		SafeRelease(_overrideMaterial);
		_overrideMaterial = SafeRetain(material);
		MarkDrawSnapshotDirty();
	}

	//Getter
	Material *RenderPass::GetEffectiveOverrideMaterial() const
	{
		return _overrideMaterial;
	}

	RenderPassResources *RenderPass::GetRenderResources(Renderer *renderer)
	{
		return GetRenderResources(renderer, GetEffectiveOverrideMaterial());
	}

	RenderPassResources *RenderPass::GetRenderResources(Renderer *renderer, Material *effectiveOverrideMaterial)
	{
		if(!_renderResources)
			_renderResources = renderer->CreateRenderPassResources();

		_renderResources->Update(this, effectiveOverrideMaterial);
		return _renderResources;
	}

	void RenderPass::GetDrawSnapshot(DrawSnapshot &snapshot) const
	{
		snapshot._flags = _flags;
		snapshot._clearColor = _clearColor;
		snapshot._clearDepth = _clearDepth;
		snapshot._clearStencil = _clearStencil;
		snapshot._renderGroupMask = _renderGroupMask;
		snapshot._shaderHint = _shaderHint;
		snapshot._isSubpass = _isSubpass;
		snapshot._isRoot = _isRoot;
		snapshot._subpass.SetColorAttachmentStates(_subpassWritesColorAttachments, _subpassReadColorAttachments, _subpassFirstColorWriteAttachment, _subpassLastColorWriteAttachment, _subpassNeedToStoreColorAttachment, _subpassNeedToPreserveColorAttachment,
												   _subpassFirstUseIsRead, _subpassLastUseIsRead);
		snapshot._subpass._writesDepthStencil = _subpassWritesDepthStencil;
		snapshot._subpass._readsDepthStencil = _subpassReadDepthStencilAttachment;
		snapshot._subpass._firstDepthStencilWrite = _subpassFirstDepthStencilWrite;
		snapshot._subpass._lastDepthStencilWrite = _subpassLastDepthStencilWrite;
		snapshot._subpass._depthStencilNeedsStore = _subpassNeedToStoreDepthStencil;
		snapshot._subpass._depthStencilNeedsPreserve = _subpassNeedToPreserveDepthStencil;
		snapshot._subpass._depthFirstUseIsRead = _depthFirstUseIsRead;
		snapshot._subpass._depthLastUseIsRead = _depthLastUseIsRead;
		UpdateDrawSnapshotFrame(snapshot);
	}

	bool RenderPass::DrawSnapshot::IsValidSize(const Vector2 &size)
	{
		return size.x >= 0.5f && size.y >= 0.5f;
	}

	Rect RenderPass::DrawSnapshot::GetFrame() const
	{
		Vector2 renderAreaSize = _frame.GetSize();
		if(!IsValidSize(renderAreaSize))
		{
			RN_DEBUG_ASSERT(false, "Invalid render area size in render pass snapshot");
			return Rect(Vector2(), Vector2(2.0f));
		}

		return _frame;
	}

	Vector2 RenderPass::DrawSnapshot::GetFramebufferResourceSize() const
	{
		if(!IsValidSize(_framebufferResourceSize))
		{
			RN_DEBUG_ASSERT(false, "Invalid framebuffer resource size in render pass snapshot");
			return Vector2(2.0f);
		}

		return _framebufferResourceSize;
	}

	void RenderPass::UpdateDrawSnapshotFrame(DrawSnapshot &snapshot) const
	{
		if(!_isSubpass)
		{
			snapshot._framebuffer = GetFramebuffer();
			snapshot._frame = GetEffectiveFrame();
			snapshot._framebufferResourceSize = snapshot._framebuffer ? snapshot._framebuffer->GetSize() : Vector2();
			return;
		}

		snapshot._framebuffer = nullptr;
		snapshot._framebufferResourceSize = Vector2();
		snapshot._frame = Rect();
	}

	Framebuffer *RenderPass::GetFramebuffer() const
	{
		RN_ASSERT(!_isSubpass, "Cannot get framebuffer for subpass");
		if(!_framebuffer)
		{
			return Renderer::GetActiveRenderer()->GetMainWindow()->GetFramebuffer();
		}

		return _framebuffer;
	}

	Rect RenderPass::GetFrame() const
	{
		RN_ASSERT(!_isSubpass, "Cannot get frame for subpass");
		if(std::abs(_frame.GetArea()) > 0.0001)
		{
			return _frame;
		}

		if(_framebuffer)
		{
			return Rect(Vector2(), _framebuffer->GetRenderAreaSize());
		}

		Renderer *renderer = Renderer::GetActiveRenderer();
		Window *mainWindow = renderer->GetMainWindow();
		Framebuffer *framebuffer = mainWindow->GetFramebuffer();
		Vector2 frameSize = framebuffer ? framebuffer->GetRenderAreaSize() : mainWindow->GetSize();
		return Rect(Vector2(), frameSize);
	}

	Rect RenderPass::GetEffectiveFrame() const
	{
		Rect frame = GetFrame();

		const FramePass *resolveParent = this;
		while(resolveParent->GetNextFramePasses()->GetCount() == 1)
		{
			RenderPass *subpass = resolveParent->GetNextFramePasses()->GetObjectAtIndex(0)->Downcast<RenderPass>();
			if(!subpass || !subpass->GetIsSubpass()) break;
			resolveParent = subpass;
		}

		resolveParent->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t, bool &stop) {
			PostProcessingAPIStage *apiStage = nextPass->Downcast<PostProcessingAPIStage>();
			if(apiStage && apiStage->GetType() == PostProcessingAPIStage::Type::ResolveMSAA)
			{
				Rect resolveFrame = apiStage->GetFrame();
				if(resolveFrame.width >= 0.5f && resolveFrame.height >= 0.5f)
					frame = resolveFrame;
				stop = true;
			}
		});

		return frame;
	}

	void RenderPass::SetSubpassWritesDepthStencilAttachment(bool writesDepthStencil)
	{
		RN_ASSERT(_isSubpass, "Cannot set subpass writes depth stencil for non-subpass");
		RN_ASSERT(_subpassReadDepthStencilAttachment != writesDepthStencil, "Cannot set subpass writes depth stencil to the same value");
		_subpassWritesDepthStencil = writesDepthStencil;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::SetSubpassReadsDepthStencilAttachment(bool depthStencilAttachment)
	{
		RN_ASSERT(_isSubpass, "Cannot set subpass read depth stencil attachment for non-subpass");
		RN_ASSERT(_subpassWritesDepthStencil != depthStencilAttachment, "Cannot set subpass read depth stencil attachment to the same value");
		_subpassReadDepthStencilAttachment = depthStencilAttachment;
		MarkDrawSnapshotDirty();
	}
	
	void RenderPass::SetSubpassWritesColorAttachments(std::vector<uint32> colorAttachments)
	{
		RN_ASSERT(_isSubpass, "Cannot set subpass writes color attachments for non-subpass");
		std::sort(colorAttachments.begin(), colorAttachments.end());
		bool hasDuplicates = std::adjacent_find(colorAttachments.begin(), colorAttachments.end()) != colorAttachments.end();
		RN_ASSERT(!hasDuplicates, "Cannot set duplicate color write attachments");
		std::vector<uint32> intersection;
		std::set_intersection(colorAttachments.begin(), colorAttachments.end(), _subpassReadColorAttachments.begin(), _subpassReadColorAttachments.end(), std::back_inserter(intersection));
		RN_ASSERT(intersection.empty(), "Subpass can not read and write the same color attachment");
		_subpassWritesColorAttachments = colorAttachments;
		MarkDrawSnapshotDirty();
	}
	
	void RenderPass::SetSubpassReadsColorAttachments(std::vector<uint32> colorAttachments)
	{
		RN_ASSERT(_isSubpass, "Cannot set subpass read color attachments for non-subpass");
		std::sort(colorAttachments.begin(), colorAttachments.end());
		bool hasDuplicates = std::adjacent_find(colorAttachments.begin(), colorAttachments.end()) != colorAttachments.end();
		RN_ASSERT(!hasDuplicates, "Cannot set duplicate color read attachments");
		std::vector<uint32> intersection;
		std::set_intersection(colorAttachments.begin(), colorAttachments.end(), _subpassWritesColorAttachments.begin(), _subpassWritesColorAttachments.end(), std::back_inserter(intersection));
		RN_ASSERT(intersection.empty(), "Subpass can not read and write the same color attachment");
		_subpassReadColorAttachments = colorAttachments;
		MarkDrawSnapshotDirty();
	}
	
	void RenderPass::WillAddFramePass(FramePass *framePass) const
	{
		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		if(renderPass)
		{
			RN_ASSERT((!_isRoot && !_isSubpass && !renderPass->_isSubpass) || GetNextFramePasses()->GetCount() == 0, "Subpasses must be a flat hierarchy");
		}
	}

	void RenderPass::DidAddFramePass(FramePass *framePass)
	{
		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		if(renderPass && renderPass->GetIsSubpass() && !_isSubpass)
		{
			_isRoot = true;
		}

		MarkDrawSnapshotDirty();
	}

	void RenderPass::DidRemoveFramePass(FramePass *)
	{
		UpdateRootFlag();
		MarkDrawSnapshotDirty();
	}

	void RenderPass::DidRemoveAllFramePasses()
	{
		_isRoot = false;
		MarkDrawSnapshotDirty();
	}

	void RenderPass::UpdateRootFlag()
	{
		_isRoot = false;

		GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
			RenderPass *renderPass = nextPass->Downcast<RenderPass>();
			if(renderPass && renderPass->GetIsSubpass() && !_isSubpass)
			{
				_isRoot = true;
				stop = true;
			}
		});
	}

	void RenderPass::UpdateSubpassChain()
	{
		_subpassCount = 0;
		if(!_isRoot) return; // only meaningful for roots
	
        // Collect complete subpass chain (in order)
        std::vector<RenderPass*> subpasses;
        std::function<void(RenderPass*)> collect = [&](RenderPass *node){
            node->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				RenderPass *nextRenderPass = nextPass->Downcast<RenderPass>();
                if(nextRenderPass && nextRenderPass->GetIsSubpass())
                {
                    subpasses.push_back(nextRenderPass);
                    collect(nextRenderPass);
                }
            });
        };
        collect(this);
		_subpassCount = subpasses.size();

        // Prepare root-level aggregation containers sized to framebuffer
        Framebuffer *fb = GetFramebuffer();
        uint32 numColorAttachments = fb ? fb->GetColorTargetCount() : 0;
        auto readsColorAttachment = [](const RenderPass *renderPass, uint32 index) {
            return std::find(renderPass->_subpassReadColorAttachments.begin(), renderPass->_subpassReadColorAttachments.end(), index) != renderPass->_subpassReadColorAttachments.end();
        };
        auto writesColorAttachment = [](const RenderPass *renderPass, uint32 index) {
            return std::find(renderPass->_subpassWritesColorAttachments.begin(), renderPass->_subpassWritesColorAttachments.end(), index) != renderPass->_subpassWritesColorAttachments.end();
        };

        _subpassFirstUseIsRead.assign(numColorAttachments, false);
        _subpassLastUseIsRead.assign(numColorAttachments, false);

        // Compute first/last for color
        for(uint32 ci = 0; ci < numColorAttachments; ++ci)
        {
            for(size_t si = 0; si < subpasses.size(); ++si)
            {
                RenderPass *rp = subpasses[si];
                bool writes = writesColorAttachment(rp, ci);
                bool reads = readsColorAttachment(rp, ci);
                if(writes || reads)
                {
                    _subpassFirstUseIsRead[ci] = (reads && !writes);
                    break;
                }
            }

            for(int si = static_cast<int>(subpasses.size()) - 1; si >= 0; --si)
            {
                RenderPass *rp = subpasses[si];
                bool writes = writesColorAttachment(rp, ci);
                bool reads = readsColorAttachment(rp, ci);
                if(writes || reads)
                {
                    _subpassLastUseIsRead[ci] = (reads && !writes);
                    break;
                }
            }
        }

        // Compute first/last for depth-stencil
        _depthFirstUseIsRead = false;
        _depthLastUseIsRead = false;
        if(fb && fb->GetDepthStencilTexture())
        {
            for(size_t si = 0; si < subpasses.size(); ++si)
            {
                RenderPass *rp = subpasses[si];
                bool writes = rp->_subpassWritesDepthStencil;
                bool reads = rp->_subpassReadDepthStencilAttachment;
                if(writes || reads)
                {
                    _depthFirstUseIsRead = (reads && !writes);
                    break;
                }
            }

            for(int si = static_cast<int>(subpasses.size()) - 1; si >= 0; --si)
            {
                RenderPass *rp = subpasses[si];
                bool writes = rp->_subpassWritesDepthStencil;
                bool reads = rp->_subpassReadDepthStencilAttachment;
                if(writes || reads)
                {
                    _depthLastUseIsRead = (reads && !writes);
                    break;
                }
            }
        }

        // Now compute per-subpass write/store info
        std::vector<bool> loadedColorTargets(numColorAttachments, false);
        bool loadedDepthStencil = false;
        std::vector<RenderPass *> lastColorWriter(numColorAttachments, nullptr);
        std::vector<RenderPass *> lastColorReader(numColorAttachments, nullptr);
        RenderPass *lastDepthStencilWriter = nullptr;
        RenderPass *lastDepthStencilReader = nullptr;

        size_t index = 0;
        for(RenderPass *rp : subpasses)
        {
            rp->_flags = _flags;
            rp->_clearDepth = _clearDepth;
            rp->_clearStencil = _clearStencil;
            rp->_clearColor = _clearColor;
            rp->_subpassFirstDepthStencilWrite = false;
            rp->_subpassNeedToStoreDepthStencil = false;
            rp->_subpassNeedToStoreColorAttachment.clear();
            rp->_subpassNeedToPreserveColorAttachment.clear();
            rp->_subpassNeedToPreserveDepthStencil = false;
            rp->_subpassFirstColorWriteAttachment.clear();
            rp->_subpassLastDepthStencilWrite = false;
            rp->_subpassLastColorWriteAttachment.clear();
            rp->_subpassIndex = index++;

            for(uint32 attachment : rp->_subpassWritesColorAttachments)
            {
                if(attachment >= numColorAttachments) continue;
                if(!loadedColorTargets[attachment])
                {
                    rp->_subpassFirstColorWriteAttachment.push_back(attachment);
                    loadedColorTargets[attachment] = true;
                }
                lastColorWriter[attachment] = rp;
            }

            for(uint32 attachment : rp->_subpassReadColorAttachments)
            {
                if(attachment >= numColorAttachments) continue;
                lastColorReader[attachment] = rp;
            }

            if(!loadedDepthStencil && rp->_subpassWritesDepthStencil)
            {
                rp->_subpassFirstDepthStencilWrite = true;
                loadedDepthStencil = true;
            }

            if(rp->_subpassWritesDepthStencil) lastDepthStencilWriter = rp;
            if(rp->_subpassReadDepthStencilAttachment) lastDepthStencilReader = rp;
        }

        for(uint32 attachment = 0; attachment < numColorAttachments; attachment++)
        {
            RenderPass *writer = lastColorWriter[attachment];
            if(writer)
            {
                writer->_subpassLastColorWriteAttachment.push_back(attachment);
                RenderPass *reader = lastColorReader[attachment];
                if(reader && reader->_subpassIndex > writer->_subpassIndex)
                {
                    writer->_subpassNeedToStoreColorAttachment.push_back(attachment);
                }
            }
        }

        if(lastDepthStencilWriter)
        {
            lastDepthStencilWriter->_subpassLastDepthStencilWrite = true;
            if(lastDepthStencilReader && lastDepthStencilReader->_subpassIndex > lastDepthStencilWriter->_subpassIndex)
            {
                lastDepthStencilWriter->_subpassNeedToStoreDepthStencil = true;
            }
        }

		std::vector<bool> colorNeededLater(numColorAttachments, false);
		bool depthStencilNeededLater = false;
		for(int si = static_cast<int>(subpasses.size()) - 1; si >= 0; --si)
		{
			RenderPass *rp = subpasses[si];
			for(uint32 ci = 0; ci < numColorAttachments; ci++)
			{
				bool uses = writesColorAttachment(rp, ci) || readsColorAttachment(rp, ci);
				if(!uses && colorNeededLater[ci])
					rp->_subpassNeedToPreserveColorAttachment.push_back(ci);
				if(uses)
					colorNeededLater[ci] = true;
			}

			if(fb && fb->GetDepthStencilTexture())
			{
				bool usesDepthStencil = rp->_subpassWritesDepthStencil || rp->_subpassReadDepthStencilAttachment;
				rp->_subpassNeedToPreserveDepthStencil = !usesDepthStencil && depthStencilNeededLater;
				if(usesDepthStencil)
					depthStencilNeededLater = true;
			}
		}

		MarkDrawSnapshotDirty();
	}

} // namespace RN
