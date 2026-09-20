//
//  RNMetalRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalRenderer.h"
#include "../../../Source/Scene/RNLight.h"
#include "RNMetalInternals.h"
#include "RNMetalShaderLibrary.h"
#include "RNMetalGPUBuffer.h"
#include "RNMetalDynamicGPUBuffer.h"
#include "RNMetalTexture.h"
#include "RNMetalTextureInfo.h"
#include "RNMetalUniformBuffer.h"
#include "RNMetalDevice.h"
#include "RNMetalRendererDescriptor.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalSwapChain.h"
#include "../../../Source/Rendering/RNShader.h"

namespace RN
{
	RNDefineMeta(MetalRenderer, Renderer)

	static_assert(sizeof(DrawIndirectArguments) == sizeof(MTLDrawPrimitivesIndirectArguments), "DrawIndirectArguments must match MTLDrawPrimitivesIndirectArguments");
	static_assert(sizeof(DrawIndexedIndirectArguments) == sizeof(MTLDrawIndexedPrimitivesIndirectArguments), "DrawIndexedIndirectArguments must match MTLDrawIndexedPrimitivesIndirectArguments");

	bool MetalRenderer::ShouldInheritViews(RenderPass::ViewMode viewMode, bool isSubpass, bool hasInheritedViewState, bool destinationSupportsViewState) const
	{
		if(isSubpass)
			return hasInheritedViewState;

		switch(viewMode)
		{
			case RenderPass::ViewMode::Auto:
				return hasInheritedViewState && destinationSupportsViewState;

			case RenderPass::ViewMode::InheritViews:
				if(hasInheritedViewState)
				{
					RN_ASSERT(destinationSupportsViewState, "Render pass requested InheritViews, but the destination framebuffer does not support the incoming view state");
				}
				return hasInheritedViewState;

			case RenderPass::ViewMode::SingleView:
				return false;
		}

		return false;
	}

	bool MetalRenderer::SupportsViewState(const MetalFramebuffer *framebuffer, uint8 multiviewLayer, uint8 multiviewCount) const
	{
		if(multiviewLayer == 0 && multiviewCount == 0)
			return true;
		if(!framebuffer)
			return false;

		auto supportsTargetView = [multiviewLayer, multiviewCount](const MetalFramebuffer::MetalTargetView *targetView) {
			if(!targetView)
				return true;
			if(!targetView->targetView.texture)
				return false;

			const uint32 requiredLayerCount = (multiviewCount > 0) ? multiviewCount : 1;
			switch(targetView->targetView.texture->GetDescriptor().type)
			{
				case Texture::Type::Type1DArray:
				case Texture::Type::Type2DArray:
				case Texture::Type::Type3D:
					return multiviewLayer + requiredLayerCount <= targetView->targetView.length;

				default:
					return false;
			}
		};

		for(const MetalFramebuffer::MetalTargetView *targetView : framebuffer->_colorTargets)
		{
			if(!supportsTargetView(targetView))
				return false;
		}

		if(!supportsTargetView(framebuffer->_depthStencilTarget))
			return false;

		return !(framebuffer->_colorTargets.empty() && !framebuffer->_depthStencilTarget);
	}

	MetalRenderer::MetalRenderer(MetalRendererDescriptor *descriptor, MetalDevice *device) :
		Renderer(descriptor, device),
		_mipMapTextures(new Set()),
		_mainWindow(nullptr),
		_activeFrameSubmission(nullptr),
		_defaultPostProcessingDrawable(nullptr),
		_ppConvertMaterial(nullptr),
		_defaultShaderLibrary(nullptr),
		_currentMultiviewLayer(0),
		_currentMultiviewFallbackRenderPass(nullptr)
	{
		RN_PROFILE_SCOPE();
		_internals->device = device->GetDevice();
		_internals->commandQueue = [_internals->device newCommandQueue];
		_internals->stateCoordinator.SetDevice(_internals->device);

		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneMetal:/Shaders.json"));
		_uniformBufferPool = new MetalUniformBufferPool();
		StartRenderThread();
	}

	MetalRenderer::~MetalRenderer()
	{
		RN_PROFILE_SCOPE();
		StopRenderThread();
		FlushAllDeletedDrawables();

		[_internals->commandQueue release];
		[_internals->device release];

		SafeRelease(_mipMapTextures);
		SafeRelease(_defaultShaderLibrary);

		delete _uniformBufferPool;
	}


	Window *MetalRenderer::CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor, void *hwnd)
	{
		RN_PROFILE_SCOPE();
		MetalWindow *window = new MetalWindow(size, screen, this, descriptor);
		if(!_mainWindow)
			_mainWindow = window->Retain();

		return window;
	}

	Window *MetalRenderer::GetMainWindow()
	{
		RN_PROFILE_SCOPE();
		return _mainWindow;
	}

	void MetalRenderer::SetMainWindow(Window *window)
	{
		RN_PROFILE_SCOPE();
		_mainWindow = window;
	}

	id MetalRenderer::GetCommandQueue() const
	{
		RN_PROFILE_SCOPE();
		return _internals->commandQueue;
	}

	MetalFrameSubmission &MetalRenderer::GetActiveFrameSubmission()
	{
		AssertOnSubmissionThread();
		RN_ASSERT(_activeFrameSubmission, "No active Metal frame submission");
		return *_activeFrameSubmission;
	}

	Shader::UsageHint MetalRenderer::GetMetalShaderHint(Shader::UsageHint shaderHint) const
	{
		if(shaderHint == Shader::UsageHint::Multiview)
		{
			return Shader::UsageHint::Default;
		}
		if(shaderHint == Shader::UsageHint::DepthMultiview)
		{
			return Shader::UsageHint::Depth;
		}
		if(shaderHint == Shader::UsageHint::ShadowDepthMultiview)
		{
			return Shader::UsageHint::ShadowDepth;
		}

		return shaderHint;
	}


	void MetalRenderer::CreateMipMapForTexture(MetalTexture *texture)
	{
		RN_PROFILE_SCOPE();
		LockGuard<Lockable> lock(_lock);
		_mipMapTextures->AddObject(texture);
	}

	void MetalRenderer::CreateMipMaps()
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		Set *mipMapTextures = nullptr;
		{
			LockGuard<Lockable> lock(_lock);
			if(_mipMapTextures->GetCount() > 0)
			{
				mipMapTextures = new Set(_mipMapTextures);
				_mipMapTextures->RemoveAllObjects();
			}
		}

		if(!mipMapTextures)
			return;

		id<MTLCommandBuffer> commandBuffer = [_internals->commandQueue commandBuffer];

		mipMapTextures->Enumerate<MetalTexture>([&](MetalTexture *texture, bool &stop) {

			id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
			[commandEncoder generateMipmapsForTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture()];
			[commandEncoder endEncoding];
		});

		[commandBuffer commit];

		//TODO: make async
		[commandBuffer waitUntilCompleted];

		mipMapTextures->Release();
	}


	void MetalRenderer::Render(Function &&function)
	{
		RN_PROFILE_SCOPE();
		@autoreleasepool {
			QueueFrameSubmission(std::move(function));
		}
	}

	void MetalRenderer::StartRenderThread()
	{
		_internals->renderThread = new Thread([this]() {
			while(true)
			{
				@autoreleasepool {
					AutoreleasePool pool;
					if(!ConsumeRenderThreadWork())
						return;
				}
			}
		}, false);
		_internals->renderThread->SetName(RNCSTR("RN::MetalRender"));
		_internals->renderThread->Start();
		while(!_internals->renderThread->IsRunning())
			std::this_thread::yield();
	}

	void MetalRenderer::StopRenderThread()
	{
		Thread *renderThread = _internals->renderThread;
		if(!renderThread)
			return;

		_internals->renderThreadQueue.Shutdown();
		renderThread->WaitForExit();
		_internals->renderThreadQueue.Drain();
		renderThread->Release();
		_internals->renderThread = nullptr;
	}

	void MetalRenderer::AssertOnSubmissionThread()
	{
#if RN_BUILD_DEBUG
		std::thread::id currentThread = std::this_thread::get_id();
		if(!_internals->hasSubmissionThread)
		{
			_internals->submissionThread = currentThread;
			_internals->hasSubmissionThread = true;
		}

		RN_DEBUG_ASSERT(_internals->submissionThread == currentThread, "Metal frame submission must stay on one submission thread");
		RN_DEBUG_ASSERT(!_internals->renderThread || !_internals->renderThread->OnThread(), "Metal frame submission must not run on the render thread");
#endif
	}

	bool MetalRenderer::IsOnRenderThread() const
	{
		return _internals->renderThread && _internals->renderThread->OnThread();
	}

	void MetalRenderer::AssertOnRenderThread() const
	{
#if RN_BUILD_DEBUG
		RN_DEBUG_ASSERT(IsOnRenderThread(), "Metal render work must run on the render thread");
#endif
	}

	void MetalRenderer::ScheduleRenderThreadWork(Function &&function)
	{
		if(IsOnRenderThread())
		{
			function();
			return;
		}

		_internals->renderThreadQueue.PushTask(std::move(function));
	}

	void MetalRenderer::SynchronizeRenderThread()
	{
		if(IsOnRenderThread())
			return;

		_internals->renderThreadQueue.Synchronize();
	}

	void MetalRenderer::QueueFrameSubmission(Function &&function)
	{
		RN_PROFILE_SCOPE();
		AssertOnSubmissionThread();

		MetalFrameSubmission submission;
		if(!_internals->renderThreadQueue.WaitForSpace())
			return;

		BuildFrameSubmission(submission, std::move(function));
		_internals->renderThreadQueue.Push(std::move(submission));
	}

	bool MetalRenderer::ConsumeRenderThreadWork()
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();

		MetalFrameSubmission submission;
		Function task;
		using WorkType = RenderThreadQueue<MetalFrameSubmission>::WorkType;
		WorkType workType = _internals->renderThreadQueue.Pop(submission, task);
		if(workType == WorkType::None)
			return false;

		if(workType == WorkType::Task)
		{
			task();
			return true;
		}

		if(!submission.renderFrame.BeginPresentationStatesOnRenderThread())
		{
			FinishRenderFrameSubmission(submission.renderFrame);
			return true;
		}

		PrepareRenderFrame(submission);
		RenderFrameSubmission(submission);
		PrintFrameStatistics(submission.renderFrame);
		FinishRenderFrameSubmission(submission.renderFrame);
		return true;
	}

	void MetalRenderer::BuildFrameSubmission(MetalFrameSubmission &submission, Function &&function)
	{
		RN_PROFILE_SCOPE();
		AssertOnSubmissionThread();

		//Submit camera is called for each camera and creates draw items per camera
		BeginRenderFrameSubmission(submission.renderFrame);
		RenderFrame *previousRenderFrame = SetActiveRenderFrame(&submission.renderFrame);
		ScopeGuard activeRenderFrameGuard([this, previousRenderFrame]() {
			SetActiveRenderFrame(previousRenderFrame);
		});
		MetalFrameSubmission *previousSubmission = _activeFrameSubmission;
		_activeFrameSubmission = &submission;
		function();
		_activeFrameSubmission = previousSubmission;
		submission.PruneSkippedRenderPasses();
	}

	void MetalRenderer::RenderFrameSubmission(const MetalFrameSubmission &submission)
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();

		for(MetalSwapChain *swapChain : submission.swapChains)
		{
			//TODO: do this the first time the swap chain is actually used
			swapChain->AcquireBackBuffer();
			swapChain->Prepare();
		}

		_internals->commandBuffer = [_internals->commandQueue commandBuffer];

		for(const MetalRenderPass &renderPass : submission.renderPasses)
		{
			if(renderPass.type == MetalRenderPass::Type::Compute)
			{
				RenderComputePass(submission, renderPass);
				continue;
			}

			if(renderPass.framebuffer->GetSwapChain() && !renderPass.framebuffer->GetSwapChain()->GetMetalColorTexture())
			{
				continue;
			}

			if(!renderPass.UsesDrawItems())
			{
				RenderAPIRenderPass(submission, renderPass);
				continue;
			}

			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
			const RenderPass::DrawSnapshot &drawSnapshot = framePass.GetDrawSnapshot();
			// Skip creating a Metal render encoder for root/container passes
			if(drawSnapshot.IsRoot())
			{
				continue;
			}

			_internals->currentRenderState = nullptr; //This is a property of the encoder and needs to be set to nullptr here to force setting it again.
			MTLRenderPassDescriptor *descriptor = renderPass.framebuffer->GetRenderPassDescriptor(drawSnapshot, renderPass.resolveFramebuffer, renderPass.multiviewLayer, renderPass.multiviewCount);
			_internals->commandEncoder = [_internals->commandBuffer renderCommandEncoderWithDescriptor:descriptor];
			[descriptor release];

			Rect cameraRect = drawSnapshot.IsSubpass() ? framePass.GetCameraSnapshot().GetFrame() : drawSnapshot.GetFrame();
			MTLViewport viewPort;
			viewPort.originX = cameraRect.x;
			viewPort.originY = cameraRect.y;
			viewPort.width = cameraRect.width;
			viewPort.height = cameraRect.height;
			viewPort.znear = 0.0f;
			viewPort.zfar = 1.0f;
			[_internals->commandEncoder setViewport:viewPort];

			if(renderPass.type == MetalRenderPass::Type::Convert)
			{
				RenderAPIRenderPass(submission, renderPass);
			}
			else
			{
				RN_DEBUG_ASSERT(renderPass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
				const MetalPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderPass.preparedRenderPassIndex];
				const std::vector<MetalPreparedDrawItem> &drawItems = preparedPass.drawItems;
				uint32 stepSize = 0;
				uint32 stepSizeIndex = 0;
				for(size_t i = 0; i < drawItems.size(); i += stepSize)
				{
					stepSize = preparedPass.instanceSteps[stepSizeIndex++];

					uint32 counter = 0;
					const MetalPreparedDrawItem &baseDrawItem = drawItems[i];
					const MetalDrawable::RenderResources &renderResources = *baseDrawItem.renderResources;
					for(size_t n = 0; n < renderResources.vertexShaderUniformBuffers.size(); n++)
					{
						for(size_t instance = 0; instance < stepSize; instance++)
						{
							Shader::ArgumentBuffer *argument = renderResources.argumentBufferToUniformBufferMapping[counter];

							//TODO: Somehow find a better way to know if an argument buffer contains instance data or not
							//Assume that only storage buffers can contain per instance data
							if(instance > 0 && argument->GetType() != Shader::ArgumentBuffer::Type::StorageBuffer) break;

							const MetalPreparedDrawItem &preparedDrawItem = drawItems[i + instance];
							const RenderFrame::DrawItem &drawItem = *preparedDrawItem.drawItem;
							const MetalDrawable::RenderResources &drawableRenderResources = *preparedDrawItem.renderResources;
							const Material::Properties &mergedMaterialProperties = drawableRenderResources.mergedMaterialSnapshot.GetProperties();

							MetalUniformBufferReference *bufferReference = drawableRenderResources.vertexShaderUniformBuffers[n];
							UpdateUniformBufferReference(bufferReference, instance == 0);
							FillUniformBuffer(argument, bufferReference, drawItem, mergedMaterialProperties, framePass);
						}
						counter += 1;
					}

					for(size_t n = 0; n < renderResources.fragmentShaderUniformBuffers.size(); n++)
					{
						for(size_t instance = 0; instance < stepSize; instance++)
						{
							Shader::ArgumentBuffer *argument = renderResources.argumentBufferToUniformBufferMapping[counter];

							//TODO: Somehow find a better way to know if an argument buffer contains instance data or not
							//Assume that only storage buffers can contain per instance data
							if(instance > 0 && argument->GetType() != Shader::ArgumentBuffer::Type::StorageBuffer) break;

							const MetalPreparedDrawItem &preparedDrawItem = drawItems[i + instance];
							const RenderFrame::DrawItem &drawItem = *preparedDrawItem.drawItem;
							const MetalDrawable::RenderResources &drawableRenderResources = *preparedDrawItem.renderResources;
							const Material::Properties &mergedMaterialProperties = drawableRenderResources.mergedMaterialSnapshot.GetProperties();

							MetalUniformBufferReference *bufferReference = drawableRenderResources.fragmentShaderUniformBuffers[n];
							UpdateUniformBufferReference(bufferReference, instance == 0);
							FillUniformBuffer(argument, bufferReference, drawItem, mergedMaterialProperties, framePass);
						}

						counter += 1;
					}

					RenderDrawable(baseDrawItem, stepSize, renderPass, submission.renderFrame, framePass);
				}
			}

			[_internals->commandEncoder endEncoding];
			_internals->commandEncoder = nil;
		}

		for(MetalSwapChain *swapChain : submission.swapChains)
		{
			swapChain->Finalize();
			swapChain->PresentBackBuffer(_internals->commandBuffer);
		}

		submission.renderFrame.EndPresentationStatesOnRenderThread();

		//Flush all uniform buffers to make the GPU get the latest changes from CPU
		_uniformBufferPool->FlushAllBuffers();

		[_internals->commandBuffer commit];

		for(MetalSwapChain *swapChain : submission.swapChains)
		{
			swapChain->PostPresent(_internals->commandBuffer);
		}

		RN_PROFILE_FRAME();

		_internals->commandBuffer = nil;
	}

	void MetalRenderer::RenderAPIRenderPass(const MetalFrameSubmission &submission, const MetalRenderPass &renderPass)
	{
		RN_PROFILE_SCOPE();
		switch(renderPass.type)
		{
				case MetalRenderPass::Type::Convert:
				{
					RN_DEBUG_ASSERT(renderPass.previousStoredFramebuffer, "Convert render pass requires a previous framebuffer");
					RN_DEBUG_ASSERT(renderPass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
					const MetalPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderPass.preparedRenderPassIndex];
					RN_DEBUG_ASSERT(!preparedPass.drawItems.empty(), "Convert render pass requires a prepared draw item");
					const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
					RenderDrawable(preparedPass.drawItems[0], 1, renderPass, submission.renderFrame, framePass);
					break;
				}

			case MetalRenderPass::Type::Blit:
			{
				//TODO: Handle multiple and not existing textures
				MetalFramebuffer *sourceFramebuffer = renderPass.previousStoredFramebuffer;
				Texture *sourceTexture = sourceFramebuffer->GetColorTexture(0);
				MetalFramebuffer *destinationFramebuffer = renderPass.framebuffer;
				Texture *destinationTexture = destinationFramebuffer->GetColorTexture(0);

				id<MTLTexture> sourceMTLTexture = nullptr;
				id<MTLTexture> destinationMTLTexture = nullptr;

				RN::Vector3 sourceTextureSize;

				if(sourceTexture)
				{
					sourceMTLTexture = static_cast< id<MTLTexture> >(sourceTexture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
					sourceTextureSize.x = sourceTexture->GetDescriptor().width;
					sourceTextureSize.y = sourceTexture->GetDescriptor().height;
					sourceTextureSize.z = sourceTexture->GetDescriptor().depth;
				}
				else
				{
					sourceMTLTexture = sourceFramebuffer->GetSwapChain()->GetMetalColorTexture();
					sourceTextureSize.x = renderPass.previousStoredRenderAreaSize.x;
					sourceTextureSize.y = renderPass.previousStoredRenderAreaSize.y;
					sourceTextureSize.z = 0;
				}

				if(destinationTexture)
				{
					destinationMTLTexture = static_cast< id<MTLTexture> >(destinationTexture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
				}
				else
				{
					destinationMTLTexture = destinationFramebuffer->GetSwapChain()->GetMetalColorTexture();
				}

				const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
				const RenderPass::DrawSnapshot &drawSnapshot = framePass.GetDrawSnapshot();
				MTLRenderPassDescriptor *descriptor = renderPass.framebuffer->GetRenderPassDescriptor(drawSnapshot, nullptr, 0, 0);
				id<MTLBlitCommandEncoder> commandEncoder = [[_internals->commandBuffer blitCommandEncoder] retain];
				[descriptor release];

				Rect targetRect = drawSnapshot.GetFrame();
				[commandEncoder copyFromTexture:sourceMTLTexture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(sourceTextureSize.x, sourceTextureSize.y, sourceTextureSize.z) toTexture:destinationMTLTexture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(targetRect.x, targetRect.y, 0)];

				[commandEncoder endEncoding];
				[commandEncoder release];

				break;
			}

			default:
				break;
		}
	}

	void MetalRenderer::SubmitCamera(Camera *camera, Function &&function)
	{
		SubmitCamera(GetActiveFrameSubmission(), camera, std::move(function));
	}

	void MetalRenderer::SubmitCamera(MetalFrameSubmission &frameSubmission, Camera *camera, Function &&function)
	{
		RN_PROFILE_SCOPE();
		const Array *multiviewCameras = camera->GetMultiviewCameras();
		if(multiviewCameras && multiviewCameras->GetCount() > 0)
		{
			//TODO: Multiview is not supported by metal, should use instanced with viewport selection instead (https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/rendering_to_multiple_texture_slices_in_a_draw_command?language=objc)
			/*if(multiviewCameras->GetCount() > 1 && GetVulkanDevice()->GetSupportsMultiview())
			{

			}
			else*/
			{
				//If multiview is not supported or there is only one multiview camera, render them individually into the correct framebuffer texture layers
				multiviewCameras->Enumerate<Camera>([&](Camera *multiviewCamera, size_t index, bool &stop){
					_currentMultiviewLayer = index;
					_currentMultiviewFallbackRenderPass = camera->GetRenderPass();

					RN::Function submission = RN::MakeFunction([&function](){ function(); });
					SubmitCamera(frameSubmission, multiviewCamera, std::move(submission));

					_currentMultiviewLayer = 0;
					_currentMultiviewFallbackRenderPass = nullptr;
				});

				return;
			}
		}

		size_t previousRenderPassIndex = frameSubmission.renderPasses.size();
		frameSubmission.activeRenderPassIndex = previousRenderPassIndex;

		FramePass *cameraFramePass = _currentMultiviewFallbackRenderPass ? static_cast<FramePass *>(_currentMultiviewFallbackRenderPass) : camera->GetRootFramePass();
		SubmitRootFramePass(frameSubmission, camera, cameraFramePass);

		const size_t submittedRenderPassEndIndex = frameSubmission.renderPasses.size();

		// Now distribute drawables across the newly created passes for this camera.
		frameSubmission.activeRenderPassIndex = previousRenderPassIndex;
		BeginCameraPassAttachmentSnapshots();
		ScopeGuard cameraPassAttachmentSnapshotsGuard([this]() {
			FinishCameraPassAttachmentSnapshots();
		});
		function();

		for(size_t pi = previousRenderPassIndex; pi < submittedRenderPassEndIndex; pi++)
		{
			MetalRenderPass &submittedRenderPass = frameSubmission.renderPasses[pi];
			if(!submittedRenderPass.UsesDrawItems())
				continue;

			AddCameraPassAttachmentSnapshots(submittedRenderPass.renderFramePassIndex);
		}
	}

	size_t MetalRenderer::SubmitRootRenderPass(MetalFrameSubmission &frameSubmission, Camera *camera, RenderPass *cameraRenderPass)
	{
		RN_PROFILE_SCOPE();

		cameraRenderPass->UpdateSubpassChain();
		const size_t frameStatisticsIndex = frameSubmission.renderFrame.AddCameraStatistics();

		RenderPassResources *renderPassResources = cameraRenderPass->GetRenderResources(this);
		const RenderPass::DrawSnapshot &drawSnapshot = renderPassResources->GetDrawSnapshot();
		Framebuffer *framebuffer = drawSnapshot.GetFramebuffer();
		if(!framebuffer) return RenderFrame::InvalidPassIndex;

		MetalRenderPass renderPass;
		renderPass.renderPass = cameraRenderPass;
		renderPass.frameStatisticsIndex = frameStatisticsIndex;
		renderPass.multiviewLayer = _currentMultiviewLayer;
		renderPass.multiviewCount = (_currentMultiviewFallbackRenderPass || _currentMultiviewLayer > 0) ? 1 : 0;
		renderPass.renderAreaSize = drawSnapshot.GetFrame().GetSize();
		renderPass.shaderHint = GetMetalShaderHint(drawSnapshot.GetShaderHint());
		renderPass.renderFramePassIndex = frameSubmission.renderFrame.AddPass(drawSnapshot, renderPassResources->GetOverrideMaterialSnapshot(), renderPassResources->GetIdentity(), renderPassResources->GetOverrideMaterialSnapshotVersion());

		RenderFrame::CameraSnapshot cameraSnapshot = RenderFrame::CameraSnapshot::WithCamera(camera, drawSnapshot.GetFrame());
		RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(renderPass.renderFramePassIndex);
		framePass.SetCameraSnapshot(cameraSnapshot);

		renderPass.framebuffer = framebuffer->Downcast<MetalFramebuffer>();
		frameSubmission.AddSwapChain(renderPass.framebuffer->GetSwapChain());

		const size_t renderPassIndex = frameSubmission.renderPasses.size();
		frameSubmission.activeRenderPassIndex = renderPassIndex;
		frameSubmission.renderPasses.push_back(renderPass);
		return renderPassIndex;
	}

	void MetalRenderer::SubmitRootFramePass(MetalFrameSubmission &frameSubmission, Camera *camera, FramePass *framePass)
	{
		RN_PROFILE_SCOPE();

		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		if(renderPass)
		{
			const size_t rootRenderPassIndex = SubmitRootRenderPass(frameSubmission, camera, renderPass);
			if(rootRenderPassIndex == RenderFrame::InvalidPassIndex) return;

			renderPass->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				SubmitFramePass(frameSubmission, camera, nextPass, frameSubmission.renderPasses[rootRenderPassIndex]);
			});
			return;
		}

		ComputePass *computePass = framePass->Downcast<ComputePass>();
		if(computePass)
		{
			SubmitComputePass(frameSubmission, computePass, nullptr, camera);
			computePass->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				SubmitRootFramePass(frameSubmission, camera, nextPass);
			});
			return;
		}

		RN_ASSERT(false, "Metal renderer only supports RenderPass and ComputePass frame pass nodes");
	}

	void MetalRenderer::SubmitFramePass(MetalFrameSubmission &frameSubmission, Camera *camera, FramePass *framePass, MetalRenderPass &previousRenderPass)
	{
		RN_PROFILE_SCOPE();

		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		if(renderPass)
		{
			SubmitRenderPass(frameSubmission, camera, renderPass, previousRenderPass);
			return;
		}

		ComputePass *computePass = framePass->Downcast<ComputePass>();
		if(computePass)
		{
			SubmitComputePass(frameSubmission, computePass, &previousRenderPass, camera);
			computePass->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				SubmitFramePass(frameSubmission, camera, nextPass, previousRenderPass);
			});
			return;
		}

		RN_ASSERT(false, "Metal renderer only supports RenderPass and ComputePass frame pass nodes");
	}

	void MetalRenderer::SubmitComputePass(MetalFrameSubmission &frameSubmission, ComputePass *computePass, MetalRenderPass *previousRenderPass, Camera *camera)
	{
		RN_PROFILE_SCOPE();

		MetalRenderPass metalComputePass;
		metalComputePass.type = MetalRenderPass::Type::Compute;
		metalComputePass.computePass = computePass;
		metalComputePass.previousStoredFramebuffer = previousRenderPass ? (previousRenderPass->resolveFramebuffer ? previousRenderPass->resolveFramebuffer : previousRenderPass->framebuffer) : nullptr;
		computePass->GetDispatchSnapshot(metalComputePass.computeDispatch);
		if(previousRenderPass && previousRenderPass->renderFramePassIndex != RenderFrame::InvalidPassIndex)
		{
			const RenderFrame::Pass &previousFramePass = frameSubmission.renderFrame.GetPass(previousRenderPass->renderFramePassIndex);
			metalComputePass.computeCameraSnapshot = previousFramePass.GetCameraSnapshot();
			metalComputePass.computeMultiviewCameraSnapshots = previousFramePass.GetMultiviewCameraSnapshots();
		}
		else if(camera)
		{
			RenderPassResources *renderPassResources = camera->GetRenderPass()->GetRenderResources(this);
			const RenderPass::DrawSnapshot &drawSnapshot = renderPassResources->GetDrawSnapshot();
			metalComputePass.computeCameraSnapshot = RenderFrame::CameraSnapshot::WithCamera(camera, drawSnapshot.GetFrame());
		}

		frameSubmission.renderPasses.push_back(metalComputePass);
	}

	void MetalRenderer::SubmitRenderPass(MetalFrameSubmission &frameSubmission, Camera *camera, RenderPass *renderPass, MetalRenderPass &previousRenderPass)
	{
		RN_PROFILE_SCOPE();

		renderPass->UpdateSubpassChain();

		// Set up
		MetalRenderPass metalRenderPass;
		metalRenderPass.renderPass = renderPass;
		metalRenderPass.frameStatisticsIndex = previousRenderPass.frameStatisticsIndex;

		PostProcessingAPIStage *apiStage = renderPass->Downcast<PostProcessingAPIStage>();
		bool isPostProcessingStage = renderPass->IsKindOfClass(PostProcessingStage::GetMetaClass());
		Material *rendererOverrideMaterial = nullptr;
		if(apiStage)
		{
			switch(apiStage->GetType())
			{
				case PostProcessingAPIStage::Type::ResolveMSAA:
				{
					metalRenderPass.type = MetalRenderPass::Type::ResolveMSAA;
					break;
				}

				case PostProcessingAPIStage::Type::Convert:
				{
					metalRenderPass.type = MetalRenderPass::Type::Convert;

					if(!_ppConvertMaterial)
					{
						_ppConvertMaterial = Material::WithShaders(_defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_vertex")), _defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_blit_fragment")))->Retain();
					}
					rendererOverrideMaterial = _ppConvertMaterial;
					break;
				}

				case PostProcessingAPIStage::Type::Blit:
				{
					metalRenderPass.type = MetalRenderPass::Type::Blit;
					break;
				}
			}
		}
		Material *effectiveOverrideMaterial = rendererOverrideMaterial ? rendererOverrideMaterial : renderPass->GetEffectiveOverrideMaterial();
		RenderPassResources *renderPassResources = renderPass->GetRenderResources(this, effectiveOverrideMaterial);
		const RenderPass::DrawSnapshot &drawSnapshot = renderPassResources->GetDrawSnapshot();

		if(previousRenderPass.renderPass)
		{
			metalRenderPass.previousStoredFramebuffer = previousRenderPass.resolveFramebuffer ? previousRenderPass.resolveFramebuffer : previousRenderPass.framebuffer;
			metalRenderPass.previousStoredRenderAreaSize = previousRenderPass.resolveFramebuffer ? previousRenderPass.resolveRenderAreaSize : previousRenderPass.renderAreaSize;
		}

		metalRenderPass.shaderHint = GetMetalShaderHint(drawSnapshot.GetShaderHint());

		Framebuffer *framebuffer = nullptr;
		if(drawSnapshot.IsSubpass())
		{
			// Subpass inherits root framebuffer
			framebuffer = previousRenderPass.framebuffer;
			metalRenderPass.renderAreaSize = previousRenderPass.renderAreaSize;
		}
		else
		{
			framebuffer = drawSnapshot.GetFramebuffer();
			metalRenderPass.renderAreaSize = drawSnapshot.GetFrame().GetSize();
		}
		metalRenderPass.framebuffer = framebuffer? framebuffer->Downcast<MetalFramebuffer>() : nullptr;
		if(!drawSnapshot.IsSubpass())
			frameSubmission.AddSwapChain(metalRenderPass.framebuffer ? metalRenderPass.framebuffer->GetSwapChain() : nullptr);

		const uint8 inheritedMultiviewLayer = previousRenderPass.multiviewLayer;
		const uint8 inheritedMultiviewCount = previousRenderPass.multiviewCount;
		const bool hasInheritedViewState = inheritedMultiviewLayer > 0 || inheritedMultiviewCount > 0;
		const bool destinationSupportsViewState = drawSnapshot.IsSubpass() ? false : SupportsViewState(metalRenderPass.framebuffer, inheritedMultiviewLayer, inheritedMultiviewCount);
		const bool shouldInheritViews = ShouldInheritViews(renderPass->GetViewMode(), drawSnapshot.IsSubpass(), hasInheritedViewState, destinationSupportsViewState);

		metalRenderPass.multiviewLayer = shouldInheritViews ? inheritedMultiviewLayer : 0;
		metalRenderPass.multiviewCount = shouldInheritViews ? inheritedMultiviewCount : 0;

		if(metalRenderPass.type != MetalRenderPass::Type::ResolveMSAA)
		{
			metalRenderPass.renderFramePassIndex = frameSubmission.renderFrame.AddPass(drawSnapshot, renderPassResources->GetOverrideMaterialSnapshot(), renderPassResources->GetIdentity(), renderPassResources->GetOverrideMaterialSnapshotVersion());
			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(metalRenderPass.renderFramePassIndex);
			const RenderFrame::CameraSnapshot &previousCamera = frameSubmission.renderFrame.GetPass(previousRenderPass.renderFramePassIndex).GetCameraSnapshot();
			framePass.SetCameraSnapshot(previousCamera.WithFrame(drawSnapshot.IsSubpass() ? previousCamera.GetFrame() : drawSnapshot.GetFrame()));
			frameSubmission.activeRenderPassIndex = frameSubmission.renderPasses.size();
			frameSubmission.renderPasses.push_back(metalRenderPass);

			if(isPostProcessingStage || metalRenderPass.type == MetalRenderPass::Type::Convert)
			{
				//Submit fullscreen quad drawable
				if(!_defaultPostProcessingDrawable)
				{
					Mesh *planeMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f, 0.0f)), Vector3(0.0f), Vector2(1.0f, 1.0f));
					Material *planeMaterial = Material::WithShaders(GetDefaultShader(Shader::Type::Vertex, nullptr), GetDefaultShader(Shader::Type::Fragment, nullptr));
					planeMaterial->SetDepthWriteEnabled(false);
					planeMaterial->SetDepthMode(DepthMode::Always);
					planeMaterial->SetOverride(Material::Override::DepthWrite | Material::Override::GroupDepth);

					_lock.Lock();
					_defaultPostProcessingDrawable = static_cast<MetalDrawable*>(CreateDrawable());
					_defaultPostProcessingDrawable->SetSources(planeMesh, planeMaterial, nullptr);
					_lock.Unlock();
				}
				SubmitDrawable(frameSubmission, _defaultPostProcessingDrawable, nullptr);
			}
		}
		else
		{
			frameSubmission.renderPasses[frameSubmission.activeRenderPassIndex].resolveFramebuffer = metalRenderPass.framebuffer;
			frameSubmission.renderPasses[frameSubmission.activeRenderPassIndex].resolveRenderAreaSize = metalRenderPass.renderAreaSize;
		}

		const Array *nextFramePasses = renderPass->GetNextFramePasses();
		nextFramePasses->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop){
			SubmitFramePass(frameSubmission, camera, nextPass, frameSubmission.renderPasses[frameSubmission.activeRenderPassIndex]);
		});
	}

	//TODO: Move into an utility class
	MTLResourceOptions MetalRenderer::MetalResourceOptionsFromOptions(GPUResource::AccessOptions options)
	{
		RN_PROFILE_SCOPE();
#if RN_PLATFORM_MAC_OS
		switch(options)
		{
			case GPUResource::AccessOptions::ReadWrite:
				return MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeManaged;
			case GPUResource::AccessOptions::WriteOnly:
				return MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeManaged;
			case GPUResource::AccessOptions::Private:
				return MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeManaged; //return  MTLResourceStorageModePrivate; //This allows creating with good flags without having to deal with a staging buffer in the metal renderer for now
		}
#else
		switch(options)
		{
			case GPUResource::AccessOptions::ReadWrite:
				return MTLResourceCPUCacheModeDefaultCache;
			case GPUResource::AccessOptions::WriteOnly:
				return MTLResourceCPUCacheModeWriteCombined;
			case GPUResource::AccessOptions::Private:
				return MTLResourceCPUCacheModeWriteCombined; //return  MTLResourceStorageModePrivate; //This allows creating with good flags without having to deal with a staging buffer in the metal renderer for now
		}
#endif
	}

	GPUBuffer *MetalRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool streameable)
	{
		RN_PROFILE_SCOPE();
		MTLResourceOptions resourceOptions = MetalResourceOptionsFromOptions(accessOptions);
		if(streameable)
		{
			// Use a small dynamic wrapper that rotates underlying MTLBuffers per FlushRange
			return (new MetalDynamicGPUBuffer(_internals->device, length, resourceOptions));
		}

		id<MTLBuffer> buffer = [_internals->device newBufferWithLength:length options:resourceOptions];
		if(!buffer) return nullptr;
		return (new MetalGPUBuffer(buffer));
	}

	MetalUniformBufferReference *MetalRenderer::GetUniformBufferReference(size_t size, size_t index)
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		LockGuard<Lockable> lock(_lock);
		return _uniformBufferPool->GetUniformBufferReference(size, index);
	}

	void MetalRenderer::UpdateUniformBufferReference(MetalUniformBufferReference *reference, bool align)
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		LockGuard<Lockable> lock(_lock);
		return _uniformBufferPool->UpdateUniformBufferReference(reference, align);
	}

	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithFile(const String *file)
	{
		RN_PROFILE_SCOPE();
		MetalShaderLibrary *lib = new MetalShaderLibrary(_internals->device, file, &_internals->stateCoordinator);
		return lib;
	}
	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithSource(const String *source)
	{
		RN_PROFILE_SCOPE();
		MetalShaderLibrary *lib = new MetalShaderLibrary(_internals->device, nullptr, &_internals->stateCoordinator);
		return lib;
	}

	ShaderLibrary *MetalRenderer::GetDefaultShaderLibrary()
	{
		RN_PROFILE_SCOPE();
		return _defaultShaderLibrary;
	}

	bool MetalRenderer::SupportsTextureFormat(const String *format) const
	{
		RN_PROFILE_SCOPE();
		//TODO: Fix this
		return true;
	}
	bool MetalRenderer::SupportsDrawMode(DrawMode mode) const
	{
		RN_PROFILE_SCOPE();
		return true;
	}

	// https://developer.apple.com/library/ios/documentation/Metal/Reference/MetalShadingLanguageGuide/MetalShadingLanguageGuide.pdf
	size_t MetalRenderer::GetAlignmentForType(PrimitiveType type) const
	{
		RN_PROFILE_SCOPE();
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
			case PrimitiveType::Half:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
			case PrimitiveType::HalfVector2:
				return 4;

			case PrimitiveType::Vector2:
			case PrimitiveType::HalfVector3:
			case PrimitiveType::HalfVector4:
			case PrimitiveType::Matrix2x2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix3x3:
			case PrimitiveType::Matrix4x4:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;
		}

		return 1;
	}

	size_t MetalRenderer::GetSizeForType(PrimitiveType type) const
	{
		RN_PROFILE_SCOPE();
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
			case PrimitiveType::Half:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
			case PrimitiveType::HalfVector2:
				return 4;

			case PrimitiveType::Vector2:
			case PrimitiveType::HalfVector3:
			case PrimitiveType::HalfVector4:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix2x2:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			case PrimitiveType::Matrix3x3:
				return 48; //Stored as 3 x float4

			case PrimitiveType::Matrix4x4:
				return 64;
		}

		return 1;
	}

	Texture *MetalRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		RN_PROFILE_SCOPE();
		MTLTextureDescriptor *metalDescriptor = MetalTextureInfo::CreateTextureDescriptor(descriptor);
		id<MTLTexture> texture = [_internals->device newTextureWithDescriptor:metalDescriptor];
		[metalDescriptor release];

		return new MetalTexture(this, texture, descriptor);
	}

	Texture *MetalRenderer::CreateTextureWithExternalMemory(const Texture::Descriptor &descriptor, const Texture::ExternalMemoryDescriptor &externalMemoryDescriptor)
	{
		RN_ASSERT(false, "Metal renderer does not support external texture memory import");
		return nullptr;
	}

	Texture *MetalRenderer::CreateTextureWithDescriptorAndIOSurface(const Texture::Descriptor &descriptor, IOSurfaceRef ioSurface)
	{
		RN_PROFILE_SCOPE();
		MTLTextureDescriptor *metalDescriptor = MetalTextureInfo::CreateTextureDescriptor(descriptor, true);
		id<MTLTexture> texture = [_internals->device newTextureWithDescriptor:metalDescriptor iosurface:ioSurface plane:0];
		[metalDescriptor release];

		return new MetalTexture(this, texture, descriptor);
	}

	Framebuffer *MetalRenderer::CreateFramebuffer(const Vector2 &size)
	{
		RN_PROFILE_SCOPE();
		return new MetalFramebuffer(size);
	}

	Drawable *MetalRenderer::CreateDrawable()
	{
		RN_PROFILE_SCOPE();
		MetalDrawable *drawable = new MetalDrawable();
		return drawable;
	}

	void MetalRenderer::DeleteDrawable(Drawable *drawable)
	{
		RN_PROFILE_SCOPE();
		QueueDrawableDeletion(drawable);
	}

	void MetalRenderer::FillUniformBuffer(Shader::ArgumentBuffer *argument, MetalUniformBufferReference *uniformBufferReference, const RenderFrame::DrawItem &drawItem, const Material::Properties &materialProperties, const RenderFrame::Pass &framePass)
	{
		RN_PROFILE_SCOPE();
		GPUBuffer *gpuBuffer = uniformBufferReference->uniformBuffer->GetActiveBuffer();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer()) + uniformBufferReference->offset;
		FillDrawUniformBuffer(argument, buffer, drawItem, materialProperties, framePass);
	}

	void MetalRenderer::SubmitLight(const Light *light)
	{
		RN_PROFILE_SCOPE();
		MetalFrameSubmission &frameSubmission = GetActiveFrameSubmission();

		// Distribute the light to all passes of the current camera
		size_t startIndex = frameSubmission.activeRenderPassIndex;

		for(size_t pi = startIndex; pi < frameSubmission.renderPasses.size(); pi++)
		{
			MetalRenderPass &renderPass = frameSubmission.renderPasses[pi];
			// Only apply to real draw passes
			if(!renderPass.UsesDrawItems()) continue;
			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(renderPass.renderFramePassIndex);

			framePass.AddLight(light);
		}
	}

	void MetalRenderer::PrepareRenderFrame(MetalFrameSubmission &submission)
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		PrepareRendererAttachments(submission.renderFrame);
		CreateMipMaps();

		submission.preparedRenderPasses.clear();
		submission.preparedRenderPasses.reserve(submission.renderFrame.GetPassCount());

		auto ensureRenderPassResources = [&](MetalRenderPass &renderPass) {
			renderPass.preparedRenderPassIndex = RenderFrame::InvalidPassIndex;

			if(!renderPass.UsesDrawItems())
			{
				return;
			}

			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
			const std::vector<size_t> &drawItemIndices = framePass.GetDrawItemIndices();
			renderPass.preparedRenderPassIndex = submission.preparedRenderPasses.size();
			submission.preparedRenderPasses.emplace_back();

			for(size_t drawItemIndex : drawItemIndices)
			{
				const RenderFrame::DrawItem &drawItem = submission.renderFrame.GetDrawItem(drawItemIndex);
				MetalDrawable *drawable = static_cast<MetalDrawable *>(drawItem.GetSourceDrawableForPreparation());
				drawable->EnsureRenderResources(renderPass.preparedRenderPassIndex);
			}
		};

		auto appendPreparedDrawItem = [](MetalPreparedRenderPass &preparedPass, const RenderFrame::DrawItem &drawItem, MetalDrawable::RenderResources &renderResources, RenderFrame::CameraStatistics &statistics) {
			MetalPreparedDrawItem preparedDrawItem;
			preparedDrawItem.drawItem = &drawItem;
			preparedDrawItem.renderResources = &renderResources;
			preparedDrawItem.instancingSortKey.submissionIndex = preparedPass.drawItems.size();
			preparedDrawItem.instancingSortKey.renderPriority = drawItem.GetRenderPriority();
			preparedDrawItem.PrepareInstancing();
			preparedPass.drawItems.push_back(preparedDrawItem);

			statistics.numberOfDrawables += 1;
			statistics.numberOfVertices += drawItem.GetMesh().GetVerticesCount();
			statistics.numberOfIndices += drawItem.GetMesh().GetIndicesCount();
		};

		auto getPipelineKey = [](const RenderFrame::DrawItem &drawItem, const MetalRenderPass &renderPass, const Drawable::MergedMaterialSnapshot &mergedMaterialSnapshot) {
			Drawable::PipelineKey pipelineKey;
			pipelineKey.meshPipelineHash = drawItem.GetMesh().GetPipelineHash();
			pipelineKey.framebuffer = renderPass.framebuffer;
			pipelineKey.vertexShader = mergedMaterialSnapshot.GetVertexShader();
			pipelineKey.fragmentShader = mergedMaterialSnapshot.GetFragmentShader();
			pipelineKey.materialProperties = mergedMaterialSnapshot.GetPipelineProperties();
			pipelineKey.renderPass = renderPass.renderPass;
			return pipelineKey;
		};

		for(MetalRenderPass &renderPass : submission.renderPasses)
		{
			if(renderPass.type == MetalRenderPass::Type::Compute)
			{
				Shader *computeShader = renderPass.computeDispatch.GetShader();
				RN_ASSERT(computeShader && computeShader->GetType() == Shader::Type::Compute, "Metal compute pass requires a compute shader");
				if(!computeShader || computeShader->GetType() != Shader::Type::Compute) continue;

				{
					LockGuard<Lockable> lock(_lock);
					renderPass.computePipelineState = _internals->stateCoordinator.GetComputePipelineState(computeShader);
				}

				const Shader::Signature *signature = computeShader->GetSignature();
				if(signature)
				{
					signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop) {
						if(buffer->GetSource() != Shader::ArgumentBuffer::Source::Draw)
							return;
						if(buffer->GetType() != Shader::ArgumentBuffer::Type::UniformBuffer)
							return;

						size_t totalSize = buffer->GetTotalUniformSize();
						if(totalSize > 0)
						{
							renderPass.computeUniformBuffers.emplace_back(GetUniformBufferReference(totalSize, buffer->GetIndex()));
						}
					});
				}
				continue;
			}

			ensureRenderPassResources(renderPass);
		}

		auto prepareRenderPass = [&](MetalRenderPass &renderPass) {
			if(renderPass.preparedRenderPassIndex == RenderFrame::InvalidPassIndex)
				return;

			MetalPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderPass.preparedRenderPassIndex];
			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
			const RenderPass::DrawSnapshot &passDrawSnapshot = framePass.GetDrawSnapshot();
			const std::vector<size_t> &drawItemIndices = framePass.GetDrawItemIndices();
			if(drawItemIndices.empty())
				return;

			preparedPass.drawItems.reserve(drawItemIndices.size());

			RenderFrame::CameraStatistics &statistics = submission.renderFrame.GetCameraStatistics(renderPass.frameStatisticsIndex);

			for(size_t drawItemIndex : drawItemIndices)
			{
				const RenderFrame::DrawItem &drawItem = submission.renderFrame.GetDrawItem(drawItemIndex);
				MetalDrawable *drawable = static_cast<MetalDrawable *>(drawItem.GetSourceDrawableForPreparation());
				MetalDrawable::RenderResources &renderResources = drawable->GetRenderResources(renderPass.preparedRenderPassIndex);

				const Material::DrawSnapshot *overrideMaterialSnapshot = framePass.GetOverrideMaterialSnapshot();
				renderResources.mergedMaterialSnapshot.Update(drawItem.GetMaterial(), drawItem.GetMaterialSnapshotVersion(), renderPass.shaderHint, overrideMaterialSnapshot, framePass.GetOverrideMaterialCacheIdentity(), framePass.GetOverrideMaterialSnapshotVersion());
				Drawable::PipelineKey pipelineKey = getPipelineKey(drawItem, renderPass, renderResources.mergedMaterialSnapshot);

				if(!renderResources.pipelineState || renderResources.pipelineKey != pipelineKey)
				{
					_lock.Lock();
					const MetalRenderingState *state = _internals->stateCoordinator.GetRenderPipelineState(pipelineKey.vertexShader, pipelineKey.fragmentShader, drawItem.GetMesh(), renderPass.framebuffer, pipelineKey.materialProperties, passDrawSnapshot);
					_lock.Unlock();

					drawable->UpdateRenderingState(renderResources, this, state, pipelineKey);
				}

				appendPreparedDrawItem(preparedPass, drawItem, renderResources, statistics);
			}

			if(framePass.GetCameraSnapshot().GetSortInstancable())
			{
				std::sort(preparedPass.drawItems.begin(), preparedPass.drawItems.end(), [](const MetalPreparedDrawItem &a, const MetalPreparedDrawItem &b) { return a.instancingSortKey < b.instancingSortKey; });
			}

			const MetalPreparedDrawItem *currentInstanceDrawItem = nullptr;
			for(const MetalPreparedDrawItem &preparedDrawItem : preparedPass.drawItems)
			{
				const bool isCompatible = currentInstanceDrawItem && preparedDrawItem.instancingSortKey.CanInstanceWith(currentInstanceDrawItem->instancingSortKey);
				if(isCompatible)
				{
					preparedPass.instanceSteps.back() += 1;
					continue;
				}

				currentInstanceDrawItem = &preparedDrawItem;
				preparedPass.instanceSteps.push_back(1);
				statistics.numberOfDrawCalls += 1;
			}
		};

		for(MetalRenderPass &renderPass : submission.renderPasses)
		{
			prepareRenderPass(renderPass);
		}

		// Do this after pipeline preparation so newly created uniform references have backing buffers.
		_uniformBufferPool->Update(this); //This will also reset all reference offsets into the buffer
	}

	void MetalRenderer::SubmitDrawable(Drawable *drawable, const SceneNode *node)
	{
		SubmitDrawable(GetActiveFrameSubmission(), drawable, node);
	}

	void MetalRenderer::SubmitDrawable(Drawable *drawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID, int32 renderPriority)
	{
		SubmitDrawable(GetActiveFrameSubmission(), drawable, modelMatrix, inverseModelMatrix, renderGroup, sourceNodeUID, renderPriority);
	}

	void MetalRenderer::SubmitDrawable(MetalFrameSubmission &frameSubmission, Drawable *sourceDrawable, const SceneNode *node)
	{
		RN_PROFILE_SCOPE();
		MetalDrawable *drawable = static_cast<MetalDrawable *>(sourceDrawable);
		uint16 renderGroup = node ? node->GetRenderGroup() : 0xffff;
		size_t drawItemIndex = RenderFrame::InvalidDrawItemIndex;

		size_t startIndex = frameSubmission.activeRenderPassIndex;
		for(size_t pi = startIndex; pi < frameSubmission.renderPasses.size(); pi += 1)
		{
			MetalRenderPass &pass = frameSubmission.renderPasses[pi];
			if(!pass.UsesDrawItems()) continue;
			if((pass.type == MetalRenderPass::Type::Convert || pass.renderPass->IsKindOfClass(PostProcessingStage::GetMetaClass())) && drawable != _defaultPostProcessingDrawable) continue;

			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(pass.renderFramePassIndex);
			const RenderPass::DrawSnapshot &passDrawSnapshot = framePass.GetDrawSnapshot();
			if((renderGroup & passDrawSnapshot.GetRenderGroupMask()) == 0) continue;

			if(drawItemIndex == RenderFrame::InvalidDrawItemIndex)
				drawItemIndex = frameSubmission.renderFrame.AddDrawItem(drawable, node, framePass);
			framePass.AddDrawItemIndex(drawItemIndex);
		}
	}

	void MetalRenderer::SubmitDrawable(MetalFrameSubmission &frameSubmission, Drawable *sourceDrawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID, int32 renderPriority)
	{
		RN_PROFILE_SCOPE();
		MetalDrawable *drawable = static_cast<MetalDrawable *>(sourceDrawable);
		size_t drawItemIndex = RenderFrame::InvalidDrawItemIndex;

		size_t startIndex = frameSubmission.activeRenderPassIndex;
		for(size_t pi = startIndex; pi < frameSubmission.renderPasses.size(); pi += 1)
		{
			MetalRenderPass &pass = frameSubmission.renderPasses[pi];
			if(!pass.UsesDrawItems()) continue;
			if((pass.type == MetalRenderPass::Type::Convert || pass.renderPass->IsKindOfClass(PostProcessingStage::GetMetaClass())) && drawable != _defaultPostProcessingDrawable) continue;

			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(pass.renderFramePassIndex);
			const RenderPass::DrawSnapshot &passDrawSnapshot = framePass.GetDrawSnapshot();
			if((renderGroup & passDrawSnapshot.GetRenderGroupMask()) == 0) continue;

			if(drawItemIndex == RenderFrame::InvalidDrawItemIndex)
				drawItemIndex = frameSubmission.renderFrame.AddDrawItem(drawable, modelMatrix, inverseModelMatrix, sourceNodeUID, renderPriority);
			framePass.AddDrawItemIndex(drawItemIndex);
		}
	}

	void MetalRenderer::RenderDrawable(const MetalPreparedDrawItem &preparedDrawItem, uint32 instanceCount, const MetalRenderPass &renderPass, const RenderFrame &renderFrame, const RenderFrame::Pass &framePass)
	{
		RN_PROFILE_SCOPE();
		const RenderFrame::DrawItem &drawItem = *preparedDrawItem.drawItem;
		const MetalDrawable::RenderResources &renderResources = *preparedDrawItem.renderResources;

		id<MTLRenderCommandEncoder> encoder = _internals->commandEncoder;
		if(_internals->currentRenderState != renderResources.pipelineState)
		{
			_internals->currentRenderState = renderResources.pipelineState;
			[encoder setRenderPipelineState:_internals->currentRenderState->state];
		}

		Shader *vertexShader = _internals->currentRenderState->vertexShader;
		Shader *fragmentShader = _internals->currentRenderState->fragmentShader;
		MetalShader *metalVertexShader = nullptr;
		MetalShader *metalFragmentShader = nullptr;
		if(vertexShader)
		{
			metalVertexShader = vertexShader->Downcast<MetalShader>();
		}
		if(fragmentShader)
		{
			metalFragmentShader = fragmentShader->Downcast<MetalShader>();
		}

		const Material::PipelineProperties &mergedMaterialProperties = renderResources.pipelineKey.materialProperties;
		[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(mergedMaterialProperties, _internals->currentRenderState)];
		[encoder setCullMode:static_cast<MTLCullMode>(mergedMaterialProperties.cullMode)];
		if(mergedMaterialProperties.usePolygonOffset)
		{
			[encoder setDepthBias:mergedMaterialProperties.polygonOffsetUnits slopeScale:mergedMaterialProperties.polygonOffsetFactor clamp:FLT_MAX];
		}
		else
		{
			[encoder setDepthBias:0.0f slopeScale:0.0f clamp:FLT_MAX];
		}

		// Update uniform buffers and set them for rendering
		{
			for(MetalUniformBufferReference *uniformBufferReference : renderResources.vertexShaderUniformBuffers)
			{
				MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBufferReference->uniformBuffer->GetActiveBuffer());
				[encoder setVertexBuffer:(id <MTLBuffer>)buffer->_buffer offset:uniformBufferReference->offset atIndex:uniformBufferReference->shaderResourceIndex];
			}

			for(MetalUniformBufferReference *uniformBufferReference : renderResources.fragmentShaderUniformBuffers)
			{
				MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBufferReference->uniformBuffer->GetActiveBuffer());
				[encoder setFragmentBuffer:(id <MTLBuffer>)buffer->_buffer offset:uniformBufferReference->offset atIndex:uniformBufferReference->shaderResourceIndex];
			}

			auto bindRequiredBuffer = [&](Shader::ArgumentBuffer *argument, GPUBuffer *buffer, const char *missingMessage, bool vertexStage) {
				MetalGPUBuffer *metalBuffer = buffer ? static_cast<MetalGPUBuffer *>(buffer->GetActiveBuffer()) : nullptr;
				RN_DEBUG_ASSERT(metalBuffer, "%s '%s' at %s buffer index %u", missingMessage, argument->GetName()->GetUTF8String(), vertexStage ? "vertex" : "fragment", argument->GetIndex());

				if(vertexStage) [encoder setVertexBuffer:(metalBuffer ? (id<MTLBuffer>)metalBuffer->_buffer : nil) offset:0 atIndex:argument->GetIndex()];
				else [encoder setFragmentBuffer:(metalBuffer ? (id<MTLBuffer>)metalBuffer->_buffer : nil) offset:0 atIndex:argument->GetIndex()];
			};

			auto bindPassBuffers = [&](MetalShader *shader, bool vertexStage) {
				if(!shader || !shader->GetSignature()) return;

				shader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop) {
					if(argument->GetSource() != Shader::ArgumentBuffer::Source::Pass)
						return;

					bindRequiredBuffer(argument, framePass.GetPassResourceBuffer(argument->GetNameHash()), "Missing pass resource buffer", vertexStage);
				});
			};

			auto bindGlobalBuffers = [&](MetalShader *shader, bool vertexStage) {
				if(!shader || !shader->GetSignature()) return;

				shader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop) {
					if(argument->GetSource() != Shader::ArgumentBuffer::Source::Frame)
						return;

					bindRequiredBuffer(argument, renderFrame.GetGlobalBuffer(argument->GetNameHash()), "Missing frame global buffer", vertexStage);
				});
			};

			bindPassBuffers(metalVertexShader, true);
			bindPassBuffers(metalFragmentShader, false);
			bindGlobalBuffers(metalVertexShader, true);
			bindGlobalBuffers(metalFragmentShader, false);
		}

		// Set textures
		const Array *textures = renderResources.mergedMaterialSnapshot.GetTextures();
		auto bindTexture = [&](Shader::ArgumentTexture *argument, id<MTLTexture> texture, bool vertexStage) {
			if(vertexStage) [encoder setVertexTexture:texture atIndex:argument->GetIndex()];
			else [encoder setFragmentTexture:texture atIndex:argument->GetIndex()];
		};

		auto getTextureForArgument = [&](Shader::ArgumentTexture *argument, bool vertexStage) -> id<MTLTexture> {
			switch(argument->GetSource())
			{
				case Shader::ArgumentTexture::Source::Frame:
				{
					Texture *globalTexture = renderFrame.GetGlobalTexture(argument->GetNameHash());
					MetalTexture *metalTexture = globalTexture ? globalTexture->Downcast<MetalTexture>() : nullptr;
					RN_DEBUG_ASSERT(metalTexture, "Missing frame global texture '%s' at %s texture index %u", argument->GetName()->GetUTF8String(), vertexStage ? "vertex" : "fragment", argument->GetIndex());
					return metalTexture ? (id<MTLTexture>)metalTexture->__GetUnderlyingTexture() : nil;
				}

				case Shader::ArgumentTexture::Source::Pass:
				{
					Texture *passTexture = framePass.GetPassResourceTexture(argument->GetNameHash());
					MetalTexture *metalTexture = passTexture ? passTexture->Downcast<MetalTexture>() : nullptr;
					RN_DEBUG_ASSERT(metalTexture, "Missing pass resource texture '%s' at %s texture index %u", argument->GetName()->GetUTF8String(), vertexStage ? "vertex" : "fragment", argument->GetIndex());
					return metalTexture ? (id<MTLTexture>)metalTexture->__GetUnderlyingTexture() : nil;
				}

				case Shader::ArgumentTexture::Source::Framebuffer:
				{
					MetalFramebuffer *previousFramebuffer = renderPass.previousStoredFramebuffer;
					if(previousFramebuffer)
					{
						MetalSwapChain *swapChain = previousFramebuffer->GetSwapChain();
						if(swapChain)
							return swapChain->GetMetalColorTexture();

						MetalTexture *colorBuffer = previousFramebuffer->GetColorTexture()->Downcast<MetalTexture>();
						return (id<MTLTexture>)colorBuffer->__GetUnderlyingTexture();
					}
					return nil;
				}

				case Shader::ArgumentTexture::Source::Material:
				{
					uint8 materialTextureIndex = argument->GetMaterialTextureIndex();
					if(textures && materialTextureIndex < textures->GetCount())
					{
						Object *textureObject = textures->GetObjectAtIndex(materialTextureIndex);

						if(textureObject->IsKindOfClass(MetalTexture::GetMetaClass()))
							return (id<MTLTexture>)static_cast<MetalTexture*>(textureObject)->__GetUnderlyingTexture();

						MetalFramebuffer *framebuffer = static_cast<MetalFramebuffer*>(textureObject);
						if(framebuffer->GetSwapChain())
							return framebuffer->GetSwapChain()->GetMetalColorTexture();

						return (id<MTLTexture>)framebuffer->GetColorTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture();
					}
					return nil;
				}

				case Shader::ArgumentTexture::Source::SubpassInput:
					RN_DEBUG_ASSERT(false, "Subpass input texture argument '%s' must be bound through subpass inputs", argument->GetName()->GetUTF8String());
					return nil;
			}

			return nil;
		};

		auto bindShaderTextures = [&](MetalShader *shader, bool vertexStage) {
			if(!shader || !shader->GetSignature()) return;

			shader->GetSignature()->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop){
				bindTexture(argument, getTextureForArgument(argument, vertexStage), vertexStage);
			});
		};

		bindShaderTextures(metalVertexShader, true);
		bindShaderTextures(metalFragmentShader, false);

		if(metalFragmentShader && metalFragmentShader->GetSignature())
		{
			metalFragmentShader->GetSignature()->GetSubpassInputs()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop){
				uint8 materialTextureIndex = argument->GetMaterialTextureIndex();
				bool isDepthInput = materialTextureIndex >= 128;
				materialTextureIndex = isDepthInput ? materialTextureIndex - 128 : materialTextureIndex;

				if(!renderPass.framebuffer)
				{
					[encoder setFragmentTexture:nil atIndex:argument->GetIndex()];
					return;
				}

				const RenderPass::DrawSnapshot &drawSnapshot = framePass.GetDrawSnapshot();
				if(!isDepthInput && drawSnapshot.IsSubpass())
				{
					//Skip unused color attachments in the assignment to match vulkan subpass behavior
					uint8 targetIndex = materialTextureIndex;
					for(uint8 i = 0; i < renderPass.framebuffer->GetColorTargetCount(); i++)
					{
						if(drawSnapshot.GetSubpass().GetColorAttachment(i).GetReads())
						{
							if(targetIndex == 0)
							{
								materialTextureIndex = i;
								break;
							}
							targetIndex -= 1;
						}
					}
				}

				Texture *texture = isDepthInput ? renderPass.framebuffer->GetDepthStencilTexture() : renderPass.framebuffer->GetColorTexture(materialTextureIndex);
				MetalTexture *framebufferTexture = texture ? texture->Downcast<MetalTexture>() : nullptr;

				if(!framebufferTexture)
				{
					[encoder setFragmentTexture:nil atIndex:argument->GetIndex()];
					return;
				}

				[encoder setFragmentTexture:(id<MTLTexture>)framebufferTexture->__GetUnderlyingTexture() atIndex:argument->GetIndex()];
			});
		}

		//Set samplers
		if(metalVertexShader)
		{
			size_t count = 0;
			for(void *sampler : metalVertexShader->_samplers)
			{
				id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
				[encoder setVertexSamplerState:samplerState atIndex:metalVertexShader->_samplerToIndexMapping[count++]];
			}
		}
		if(metalFragmentShader)
		{
			size_t count = 0;
			for(void *sampler : metalFragmentShader->_samplers)
			{
				id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
				[encoder setFragmentSamplerState:samplerState atIndex:metalFragmentShader->_samplerToIndexMapping[count++]];
			}
		}

		// Mesh
		const Mesh::DrawSnapshot &mesh = drawItem.GetMesh();
		const Mesh::BufferSnapshot &meshBuffers = drawItem.GetMeshBuffers();
		const Drawable::IndirectDrawSnapshot &indirectDrawSnapshot = drawItem.GetIndirectDrawSnapshot();
		const bool hasIndirectDraw = indirectDrawSnapshot.IsValid();
		const bool usesIndexedDraw = hasIndirectDraw ? indirectDrawSnapshot.GetType() == Drawable::IndirectDrawType::DrawIndexed : mesh.GetIndicesCount() > 0;
		MetalGPUBuffer *indirectBuffer = nullptr;
		size_t indirectCommandStride = 0;
		if(hasIndirectDraw)
		{
			RN_ASSERT(indirectDrawSnapshot.GetType() != Drawable::IndirectDrawType::DrawIndexed || mesh.GetIndicesCount() > 0, "Indexed indirect draw requires an indexed mesh");
			RN_ASSERT(indirectDrawSnapshot.GetType() != Drawable::IndirectDrawType::Draw || mesh.GetIndicesCount() == 0, "Non-indexed indirect draw requires a non-indexed mesh");

			const size_t indirectCommandSize = usesIndexedDraw ? sizeof(DrawIndexedIndirectArguments) : sizeof(DrawIndirectArguments);
			indirectCommandStride = indirectDrawSnapshot.GetStride() > 0 ? indirectDrawSnapshot.GetStride() : indirectCommandSize;
			const size_t indirectCommandRange = indirectCommandStride * (indirectDrawSnapshot.GetDrawCount() - 1) + indirectCommandSize;
			RN_DEBUG_ASSERT(indirectDrawSnapshot.GetArgumentBufferOffset() % 4 == 0, "Indirect draw argument buffer offset must be 4-byte aligned");
			RN_DEBUG_ASSERT(indirectCommandStride % 4 == 0, "Indirect draw command stride must be 4-byte aligned");
			RN_DEBUG_ASSERT(indirectCommandStride >= indirectCommandSize, "Indirect draw command stride must fit the command type");
			RN_DEBUG_ASSERT(indirectDrawSnapshot.GetArgumentBufferOffset() + indirectCommandRange <= indirectDrawSnapshot.GetArgumentBuffer()->GetLength(), "Indirect draw argument buffer range is out of bounds");

			GPUBuffer *activeBuffer = indirectDrawSnapshot.GetArgumentBuffer()->GetActiveBuffer();
			indirectBuffer = activeBuffer ? activeBuffer->Downcast<MetalGPUBuffer>() : nullptr;
			RN_DEBUG_ASSERT(indirectBuffer, "Indirect draw argument buffer must be a Metal buffer");
		}

		MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(meshBuffers.GetVertexBuffer());

		if(_internals->currentRenderState->vertexPositionBufferShaderResourceIndex <= 30)
		{
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:_internals->currentRenderState->vertexPositionBufferShaderResourceIndex];
		}

		if(_internals->currentRenderState->vertexBufferShaderResourceIndex <= 30)
		{
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:mesh.GetVertexPositionsSeparatedSize() atIndex:_internals->currentRenderState->vertexBufferShaderResourceIndex];
		}

		DrawMode drawMode = mesh.GetDrawMode();
		MTLPrimitiveType primitiveType;

		switch(drawMode)
		{
			case DrawMode::Point:
				primitiveType = MTLPrimitiveTypePoint;
				break;
			case DrawMode::Line:
				primitiveType = MTLPrimitiveTypeLine;
				break;
			case DrawMode::LineStrip:
				primitiveType = MTLPrimitiveTypeLineStrip;
				break;
			case DrawMode::Triangle:
				primitiveType = MTLPrimitiveTypeTriangle;
				break;
			case DrawMode::TriangleStrip:
				primitiveType = MTLPrimitiveTypeTriangleStrip;
				break;
		}

		if(usesIndexedDraw)
		{
			MetalGPUBuffer *indexBuffer = static_cast<MetalGPUBuffer *>(meshBuffers.GetIndicesBuffer());
			MTLIndexType indexType = mesh.GetIndexType() == PrimitiveType::Uint16? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;

			if(hasIndirectDraw)
			{
				if(indirectBuffer)
				{
					for(uint32 drawIndex = 0; drawIndex < indirectDrawSnapshot.GetDrawCount(); drawIndex++)
					{
						NSUInteger indirectCommandOffset = static_cast<NSUInteger>(indirectDrawSnapshot.GetArgumentBufferOffset() + indirectCommandStride * drawIndex);
						[encoder drawIndexedPrimitives:primitiveType indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0 indirectBuffer:(id <MTLBuffer>)indirectBuffer->_buffer indirectBufferOffset:indirectCommandOffset];
					}
				}
			}
			else if(instanceCount == 1)
			{
				[encoder drawIndexedPrimitives:primitiveType indexCount:mesh.GetIndicesCount() indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];
			}
			else
			{
				[encoder drawIndexedPrimitives:primitiveType indexCount:mesh.GetIndicesCount() indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0 instanceCount:instanceCount];
			}
		}
		else
		{
			if(hasIndirectDraw)
			{
				if(indirectBuffer)
				{
					for(uint32 drawIndex = 0; drawIndex < indirectDrawSnapshot.GetDrawCount(); drawIndex++)
					{
						NSUInteger indirectCommandOffset = static_cast<NSUInteger>(indirectDrawSnapshot.GetArgumentBufferOffset() + indirectCommandStride * drawIndex);
						[encoder drawPrimitives:primitiveType indirectBuffer:(id <MTLBuffer>)indirectBuffer->_buffer indirectBufferOffset:indirectCommandOffset];
					}
				}
			}
			else if(instanceCount == 1)
			{
				[encoder drawPrimitives:primitiveType vertexStart:0 vertexCount:mesh.GetVerticesCount()];
			}
			else
			{
				[encoder drawPrimitives:primitiveType vertexStart:0 vertexCount:mesh.GetVerticesCount() instanceCount:instanceCount];
			}
		}
	}

	void MetalRenderer::RenderComputePass(const MetalFrameSubmission &submission, const MetalRenderPass &computePass)
	{
		RN_PROFILE_SCOPE();

		RN_DEBUG_ASSERT(computePass.computePipelineState, "Missing compute pipeline state");
		if(!computePass.computePipelineState) return;

		Shader *computeShader = computePass.computeDispatch.GetShader();
		RN_DEBUG_ASSERT(computeShader && computeShader->GetType() == Shader::Type::Compute, "Metal compute pass requires a compute shader");
		if(!computeShader || computeShader->GetType() != Shader::Type::Compute) return;

		MetalShader *metalShader = computeShader->Downcast<MetalShader>();
		const Shader::Signature *signature = computeShader->GetSignature();

		id<MTLComputeCommandEncoder> encoder = [_internals->commandBuffer computeCommandEncoder];
		[encoder setComputePipelineState:computePass.computePipelineState->state];

		auto getTextureForArgument = [&](Shader::ArgumentTexture *argument) -> id<MTLTexture> {
			Texture *texture = nullptr;

			switch(argument->GetSource())
			{
				case Shader::ArgumentTexture::Source::Frame:
					texture = computePass.computeDispatch.GetResourceTexture(argument->GetNameHash());
					if(!texture)
						texture = submission.renderFrame.GetGlobalTexture(argument->GetNameHash());
					break;

				case Shader::ArgumentTexture::Source::Framebuffer:
					if(computePass.previousStoredFramebuffer)
					{
						MetalSwapChain *swapChain = computePass.previousStoredFramebuffer->GetSwapChain();
						if(swapChain)
							return swapChain->GetMetalColorTexture();

						texture = computePass.previousStoredFramebuffer->GetColorTexture(0);
					}
					RN_DEBUG_ASSERT(texture, "Missing previous framebuffer texture for compute texture '%s' at texture index %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
					break;

				case Shader::ArgumentTexture::Source::Material:
				case Shader::ArgumentTexture::Source::Pass:
				case Shader::ArgumentTexture::Source::SubpassInput:
					texture = computePass.computeDispatch.GetResourceTexture(argument->GetNameHash());
					break;
			}

			MetalTexture *metalTexture = texture ? texture->Downcast<MetalTexture>() : nullptr;
			RN_DEBUG_ASSERT(metalTexture, "Missing compute resource texture '%s' at texture index %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
			return metalTexture ? (id<MTLTexture>)metalTexture->__GetUnderlyingTexture() : nil;
		};

		if(signature)
		{
			size_t dynamicUniformIndex = 0;
			signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop) {
				if(argument->GetSource() == Shader::ArgumentBuffer::Source::Draw && argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer && argument->GetTotalUniformSize() > 0)
				{
					RN_DEBUG_ASSERT(dynamicUniformIndex < computePass.computeUniformBuffers.size(), "Missing compute uniform buffer");
					if(dynamicUniformIndex < computePass.computeUniformBuffers.size())
					{
						MetalUniformBufferReference *uniformBufferReference = computePass.computeUniformBuffers[dynamicUniformIndex++].Get();
						UpdateUniformBufferReference(uniformBufferReference, true);

						GPUBuffer *gpuBuffer = uniformBufferReference->uniformBuffer->GetActiveBuffer();
						uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer()) + uniformBufferReference->offset;
						std::memset(buffer, 0, uniformBufferReference->size);

						const RenderFrame::CameraSnapshot &cameraSnapshot = computePass.computeCameraSnapshot;

						argument->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
							const std::vector<uint8> *uniform = computePass.computeDispatch.GetUniform(descriptor->GetNameHash());
							if(uniform)
							{
								size_t copySize = std::min(uniform->size(), descriptor->GetSize());
								std::memcpy(buffer + descriptor->GetOffset(), uniform->data(), copySize);
							}
							else
							{
								FillCommonUniform(descriptor, buffer, &cameraSnapshot, nullptr);
							}
						});

						MetalGPUBuffer *metalBuffer = static_cast<MetalGPUBuffer *>(gpuBuffer);
						[encoder setBuffer:(id<MTLBuffer>)metalBuffer->_buffer offset:uniformBufferReference->offset atIndex:argument->GetIndex()];
					}
					return;
				}

				GPUBuffer *buffer = nullptr;
				if(argument->GetSource() == Shader::ArgumentBuffer::Source::Frame)
				{
					buffer = computePass.computeDispatch.GetResourceBuffer(argument->GetNameHash());
					if(!buffer)
						buffer = submission.renderFrame.GetGlobalBuffer(argument->GetNameHash());
				}
				else
				{
					buffer = computePass.computeDispatch.GetResourceBuffer(argument->GetNameHash());
				}

				MetalGPUBuffer *metalBuffer = buffer ? static_cast<MetalGPUBuffer *>(buffer->GetActiveBuffer()) : nullptr;
				RN_DEBUG_ASSERT(metalBuffer, "Missing compute resource buffer '%s' at buffer index %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
				[encoder setBuffer:(metalBuffer ? (id<MTLBuffer>)metalBuffer->_buffer : nil) offset:0 atIndex:argument->GetIndex()];
			});

			signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop) {
				[encoder setTexture:getTextureForArgument(argument) atIndex:argument->GetIndex()];
			});
		}

		if(metalShader)
		{
			size_t count = 0;
			for(void *sampler : metalShader->_samplers)
			{
				id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
				[encoder setSamplerState:samplerState atIndex:metalShader->_samplerToIndexMapping[count++]];
			}
		}

		const Shader::ComputeThreadsPerGroup &threadsPerGroup = computeShader->GetComputeThreadsPerGroup();
		NSUInteger totalThreadsPerGroup = threadsPerGroup.x * threadsPerGroup.y * threadsPerGroup.z;
		RN_DEBUG_ASSERT(totalThreadsPerGroup <= [computePass.computePipelineState->state maxTotalThreadsPerThreadgroup], "Compute pass thread group size exceeds Metal pipeline limit");

		MTLSize threadsPerThreadgroup = MTLSizeMake(threadsPerGroup.x, threadsPerGroup.y, threadsPerGroup.z);
		const std::vector<ComputePass::DispatchRegion> &dispatchRegions = computePass.computeDispatch.GetDispatchRegions();
		for(const ComputePass::DispatchRegion &dispatchRegion : dispatchRegions)
		{
			const ComputePass::DispatchSize &groupCount = dispatchRegion.groupCount;
			const ComputePass::DispatchOffset &groupOffset = dispatchRegion.groupOffset;

			if(metalShader && metalShader->_computeDispatchOffsetsBufferIndex <= 30)
			{
				uint32 dispatchOffsets[8] = {
					groupOffset.x * threadsPerGroup.x,
					groupOffset.y * threadsPerGroup.y,
					groupOffset.z * threadsPerGroup.z,
					0,
					groupOffset.x,
					groupOffset.y,
					groupOffset.z,
					0
				};
				[encoder setBytes:dispatchOffsets length:sizeof(dispatchOffsets) atIndex:metalShader->_computeDispatchOffsetsBufferIndex];
			}
			else
			{
				RN_ASSERT(groupOffset.x == 0 && groupOffset.y == 0 && groupOffset.z == 0, "Metal compute dispatch base offsets require repacked compute shaders");
			}

			MTLSize threadgroupCount = MTLSizeMake(groupCount.x, groupCount.y, groupCount.z);
			[encoder dispatchThreadgroups:threadgroupCount threadsPerThreadgroup:threadsPerThreadgroup];
		}
		[encoder endEncoding];
	}
}
