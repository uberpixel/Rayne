//
//  RNMetalInternals.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALINTERNALS_H__
#define __RAYNE_METALINTERNALS_H__

#import <Metal/Metal.h>

#include "RNMetal.h"
#include "RNMetalStateCoordinator.h"
#include "RNMetalUniformBuffer.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalRenderer.h"
#include "../../../Source/Rendering/RNRenderThreadQueue.h"

#if RN_PLATFORM_MAC_OS
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#endif

#if RN_PLATFORM_IOS
#import <UIKit/UIKit.h>
#endif

#if RN_PLATFORM_MAC_OS
@interface RNMetalView : NSView
- (id<CAMetalDrawable>)nextDrawable;
- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device screen:(RN::Screen*)screen andFormat:(MTLPixelFormat)format;
- (CGSize)getSize;
- (void)setDrawableSize:(CGSize)size;
- (void)setWindowOwner:(RN::MetalWindow *)window;
@end

@interface RNMetalWindow : NSWindow
@end
#endif

#if RN_PLATFORM_IOS
class RNMetalLayerContainer
{
public:
	RNMetalLayerContainer(CAMetalLayer *metalLayer);
	id<CAMetalDrawable> GetNextDrawable();
	RN::Vector2 GetSize();

private:
	CAMetalLayer *_metalLayer;
};
#endif

namespace RN
{
	class MetalWindow;
	class Framebuffer;
	class Camera;
	class MetalGPUBuffer;
	class MetalTexture;

	struct MetalDrawable : public Drawable
	{
		MetalDrawable() = default;

		struct RenderResources
		{
			const MetalRenderingState *pipelineState = nullptr;
			Drawable::PipelineKey pipelineKey;
			Drawable::MergedMaterialSnapshot mergedMaterialSnapshot;
			Drawable::InstancingHashCache instancingHashCache;
			bool supportsInstancing = false;

			std::vector<Shader::ArgumentBuffer*> argumentBufferToUniformBufferMapping;
			std::vector<MetalUniformBufferReference*> vertexShaderUniformBuffers;
			std::vector<MetalUniformBufferReference*> fragmentShaderUniformBuffers;
		};

		~MetalDrawable()
		{
			for(RenderResources &resources : _renderResources)
			{
				for(MetalUniformBufferReference *buffer : resources.vertexShaderUniformBuffers)
					buffer->Release();

				for(MetalUniformBufferReference *buffer : resources.fragmentShaderUniformBuffers)
					buffer->Release();

				for(Shader::ArgumentBuffer *buffer : resources.argumentBufferToUniformBufferMapping)
					buffer->Release();
			}
		}

		RenderResources &EnsureRenderResources(size_t resourceIndex)
		{
			if(_renderResources.size() <= resourceIndex)
				_renderResources.resize(resourceIndex + 1);

			return _renderResources[resourceIndex];
		}

		RenderResources &GetRenderResources(size_t resourceIndex)
		{
			RN_DEBUG_ASSERT(resourceIndex < _renderResources.size(), "Invalid render resources index");
			return _renderResources[resourceIndex];
		}

		void UpdateRenderingState(RenderResources &resources, Renderer *renderer, const MetalRenderingState *state, const Drawable::PipelineKey &pipelineKey)
		{
			resources.pipelineState = state;
			resources.pipelineKey = pipelineKey;
			resources.instancingHashCache.Invalidate();
			resources.supportsInstancing = state->vertexShader->GetHasInstancing() && state->fragmentShader->GetHasInstancing();

			for(Shader::ArgumentBuffer *buffer : resources.argumentBufferToUniformBufferMapping)
				buffer->Release();
			resources.argumentBufferToUniformBufferMapping.clear();

			for(MetalUniformBufferReference *buffer : resources.vertexShaderUniformBuffers)
				buffer->Release();
			resources.vertexShaderUniformBuffers.clear();

			for(MetalUniformBufferReference *buffer : resources.fragmentShaderUniformBuffers)
				buffer->Release();
			resources.fragmentShaderUniformBuffers.clear();

			MetalRenderer *metalRenderer = renderer->Downcast<MetalRenderer>();

			const Shader::Signature *vertexShaderSignature = state->vertexShader->GetSignature();
			vertexShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				if(buffer->GetSource() != Shader::ArgumentBuffer::Source::Draw)
					return;

				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					resources.argumentBufferToUniformBufferMapping.push_back(buffer->Retain());
					resources.vertexShaderUniformBuffers.push_back(metalRenderer->GetUniformBufferReference(totalSize, buffer->GetIndex())->Retain());
				}
			});

			const Shader::Signature *fragmentShaderSignature = state->fragmentShader->GetSignature();
			fragmentShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				if(buffer->GetSource() != Shader::ArgumentBuffer::Source::Draw)
					return;

				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					resources.argumentBufferToUniformBufferMapping.push_back(buffer->Retain());
					resources.fragmentShaderUniformBuffers.push_back(metalRenderer->GetUniformBufferReference(totalSize, buffer->GetIndex())->Retain());
				}
			});
		}

	private:
		std::vector<RenderResources> _renderResources;
	};

	struct MetalRenderPass
	{
		enum Type
		{
			Default,
			ResolveMSAA,
			Blit,
			Convert,
			Compute
		};

		bool UsesDrawItems() const { return type == Type::Default || type == Type::Convert; }

		Type type = Type::Default;
		RenderPass *renderPass = nullptr;
		ComputePass *computePass = nullptr;
		size_t renderFramePassIndex = RenderFrame::InvalidPassIndex;
		size_t preparedRenderPassIndex = RenderFrame::InvalidPassIndex;
		size_t frameStatisticsIndex = static_cast<size_t>(-1);
		MetalFramebuffer *previousStoredFramebuffer = nullptr;
		Vector2 previousStoredRenderAreaSize;

		MetalFramebuffer *framebuffer = nullptr;
		Vector2 renderAreaSize;
		Shader::UsageHint shaderHint = Shader::UsageHint::Default;
		MetalFramebuffer *resolveFramebuffer = nullptr;
		Vector2 resolveRenderAreaSize;

		ComputePass::DispatchSnapshot computeDispatch;
		RenderFrame::CameraSnapshot computeCameraSnapshot;
		std::vector<RenderFrame::CameraSnapshot> computeMultiviewCameraSnapshots;
		const MetalComputeState *computePipelineState = nullptr;
		std::vector<StrongRef<MetalUniformBufferReference>> computeUniformBuffers;

		uint8 multiviewLayer = 0;
		uint8 multiviewCount = 0; // Explicit inherited view span; 1 also represents layered single-view fallback.
	};

	struct MetalPreparedDrawItem
	{
		const RenderFrame::DrawItem *drawItem = nullptr;
		MetalDrawable::RenderResources *renderResources = nullptr;
		RenderFrame::InstancingSortKey instancingSortKey;

		void PrepareInstancing()
		{
			instancingSortKey.isSupported = renderResources->supportsInstancing && !drawItem->HasIndirectDraw();
			if(!instancingSortKey.isSupported) return;

			const size_t meshHash = drawItem->GetMeshInstancingHash();
			const size_t textureSetHash = renderResources->mergedMaterialSnapshot.GetTextureSetHash();
			if(renderResources->instancingHashCache.TryGet(meshHash, textureSetHash, instancingSortKey.hash)) return;

			instancingSortKey.hash = meshHash;
			HashCombine(instancingSortKey.hash, renderResources->pipelineState);
			HashCombine(instancingSortKey.hash, textureSetHash);

			bool hasBackingBuffers = true;
			HashCombine(instancingSortKey.hash, renderResources->vertexShaderUniformBuffers.size());
			for(MetalUniformBufferReference *buffer : renderResources->vertexShaderUniformBuffers)
			{
				HashCombine(instancingSortKey.hash, buffer->uniformBuffer);
				hasBackingBuffers &= buffer->uniformBuffer != nullptr;
			}
			HashCombine(instancingSortKey.hash, renderResources->fragmentShaderUniformBuffers.size());
			for(MetalUniformBufferReference *buffer : renderResources->fragmentShaderUniformBuffers)
			{
				HashCombine(instancingSortKey.hash, buffer->uniformBuffer);
				hasBackingBuffers &= buffer->uniformBuffer != nullptr;
			}

			if(hasBackingBuffers) renderResources->instancingHashCache.Store(meshHash, textureSetHash, instancingSortKey.hash);
		}
	};

	struct MetalPreparedRenderPass
	{
		std::vector<MetalPreparedDrawItem> drawItems;
		std::vector<uint32> instanceSteps; //Number of draw items that use the same pipeline state and can be rendered with the same draw call.
	};

	struct MetalFrameSubmission
	{
		using RenderPassType = MetalRenderPass;
		using FramebufferType = MetalFramebuffer;
		using SwapChainType = MetalSwapChain;

		void AddSwapChain(MetalSwapChain *swapChain);
		void PruneSkippedRenderPasses();

		RenderFrame renderFrame;
		std::deque<MetalRenderPass> renderPasses;
		std::vector<MetalPreparedRenderPass> preparedRenderPasses;
		std::vector<MetalSwapChain *> swapChains;
		size_t activeRenderPassIndex = 0;
	};

	struct MetalRendererInternals
	{
		RenderThreadQueue<MetalFrameSubmission> renderThreadQueue;
		Thread *renderThread = nullptr;
#if RN_BUILD_DEBUG
		std::thread::id submissionThread;
		bool hasSubmissionThread = false;
#endif
		MetalStateCoordinator stateCoordinator;

		id<MTLDevice> device;
		id<MTLCommandQueue> commandQueue;

		id<MTLCommandBuffer> commandBuffer;
		id<MTLRenderCommandEncoder> commandEncoder;

		const MetalRenderingState *currentRenderState;
	};

	struct MetalWindowInternals
	{
#if RN_PLATFORM_MAC_OS
		NSWindow *window;
#endif
#if RN_PLATFORM_IOS
		RNMetalLayerContainer *metalLayerContainer;
#endif
	};
}

#endif /* __RAYNE_METALINTERNALS_H__ */
