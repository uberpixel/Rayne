//
//  RNVulkanRenderer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanRenderer.h"
#include "RNVulkanWindow.h"
#include "RNVulkanInternals.h"
#include "RNVulkanGPUBuffer.h"
#include "RNVulkanShader.h"
#include "RNVulkanShaderLibrary.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanConstantBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanRenderer, Renderer)

	VulkanRenderer::VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_currentFrame(0),
		_mipMapTextures(new Array()),
		_submittedCommandBuffers(new Array()),
		_executedCommandBuffers(new Array())
	{
		vk::GetDeviceQueue(device->GetDevice(), device->GetWorkQueue(), 0, &_workQueue);

		//Create command pool
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetWorkQueue();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		RNVulkanValidate(vk::CreateCommandPool(device->GetDevice(), &cmdPoolInfo, nullptr, &_commandPool));

		//Create descriptor pool
		VkDescriptorPoolSize uniformBufferPoolSize = {};
		uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPoolSize.descriptorCount = 1000;
		VkDescriptorPoolSize textureBufferPoolSize = {};
		textureBufferPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		textureBufferPoolSize.descriptorCount = 1000;
		VkDescriptorPoolSize samplerBufferPoolSize = {};
		samplerBufferPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerBufferPoolSize.descriptorCount = 1000;
		std::vector<VkDescriptorPoolSize> poolSizes = { uniformBufferPoolSize, samplerBufferPoolSize, textureBufferPoolSize };

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = NULL;
		descriptorPoolInfo.poolSizeCount = poolSizes.size();
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = 100000;
		RNVulkanValidate(vk::CreateDescriptorPool(device->GetDevice(), &descriptorPoolInfo, GetAllocatorCallback(), &_descriptorPool));

		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneVulkan:/Shaders.json"));
	}

	VulkanRenderer::~VulkanRenderer()
	{
		_mipMapTextures->Release();
	}

	VkRenderPass VulkanRenderer::GetVulkanRenderPass(VulkanFramebuffer *framebuffer)
	{
		return _internals->stateCoordinator.GetRenderPassState(framebuffer)->renderPass;
	}

	void VulkanRenderer::CreateVulkanCommandBuffers(size_t count, std::vector<VkCommandBuffer> &buffers)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = _commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(count);

		buffers.resize(count);
		RNVulkanValidate(vk::AllocateCommandBuffers(GetVulkanDevice()->GetDevice(), &commandBufferAllocateInfo, buffers.data()));
	}

	VkCommandBuffer VulkanRenderer::CreateVulkanCommandBuffer()
	{
		std::vector<VkCommandBuffer> buffers;
		CreateVulkanCommandBuffers(1, buffers);

		return buffers[0];
	}

	VulkanCommandBuffer *VulkanRenderer::GetCommandBuffer()
	{
		VulkanCommandBuffer *commandBuffer = new VulkanCommandBuffer(GetVulkanDevice()->GetDevice(), _commandPool);
		_lock.Lock();
		commandBuffer->_commandBuffer = CreateVulkanCommandBuffer();
		_lock.Unlock();

		return commandBuffer->Autorelease();
	}

	VulkanCommandBufferWithCallback *VulkanRenderer::GetCommandBufferWithCallback()
	{
		VulkanCommandBufferWithCallback *commandBuffer = new VulkanCommandBufferWithCallback(GetVulkanDevice()->GetDevice(), _commandPool);
		_lock.Lock();
		commandBuffer->_commandBuffer = CreateVulkanCommandBuffer();
		_lock.Unlock();

		return commandBuffer->Autorelease();
	}

	void VulkanRenderer::SubmitCommandBuffer(VulkanCommandBuffer *commandBuffer)
	{
		_lock.Lock();
		_submittedCommandBuffers->AddObject(commandBuffer);
		_lock.Unlock();
	}

	Window *VulkanRenderer::CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor)
	{
		VulkanWindow *window = new VulkanWindow(size, screen, this, descriptor);

		if(!_mainWindow)
			_mainWindow = window->Retain();

		return window;
	}

	void VulkanRenderer::SetMainWindow(Window *window)
	{
		_mainWindow = window;
	}

	Window *VulkanRenderer::GetMainWindow()
	{
		return _mainWindow;
	}

	void VulkanRenderer::Render(Function &&function)
	{
		_currentDrawableIndex = 0;
		_internals->renderPasses.clear();
		_internals->totalDrawableCount = 0;
		_internals->currentRenderPassIndex = 0;
		_internals->currentDrawableResourceIndex = 0;
		_internals->totalDescriptorTables = 0;
		_internals->swapChains.clear();
/*		_currentRootSignature = nullptr;
		_currentSrvCbvIndex = 0;

		_completedFenceValue = _fence->GetCompletedValue();*/

		//Delete command lists that finished execution on the graphics card (the command allocator needs to be alive the whole time)
/*		for(int i = _executedCommandLists->GetCount() - 1; i >= 0; i--)
		{
			if(_executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i)->_fenceValue <= _completedFenceValue)
			{
				D3D12CommandList *commandList = _executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i);
				commandList->Finish();
				_commandListPool->AddObject(commandList);
				_executedCommandLists->RemoveObjectAtIndex(i);
			}
		}

		//Add descriptor heaps that aren't in use by the GPU anymore back to the pool
		for(int i = _boundDescriptorHeaps->GetCount() - 1; i >= 0; i--)
		{
			if(_boundDescriptorHeaps->GetObjectAtIndex<D3D12DescriptorHeap>(i)->_fenceValue <= _completedFenceValue)
			{
				D3D12DescriptorHeap *descriptorHeap = _boundDescriptorHeaps->GetObjectAtIndex<D3D12DescriptorHeap>(i);
				_descriptorHeapPool->AddObject(descriptorHeap);
				_boundDescriptorHeaps->RemoveObjectAtIndex(i);
			}
		}

		//Free other frame resources such as descriptor heaps, that are not in use by the gpu anymore
		for(int i = _internals->frameResources.size()-1; i >= 0; i--)
		{
			D3D12FrameResource &frameResource = _internals->frameResources[i];
			if(frameResource.frame <= _completedFenceValue)
			{
				frameResource.resource->Release();
				_internals->frameResources.erase(_internals->frameResources.begin() + i);
			}
		}*/

		CreateMipMaps();

		//SubmitCamera is called for each camera and creates lists of drawables per camera
		function();

		for(VulkanSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->AcquireBackBuffer();
		}

		//PolpulateDescriptorHeap();
		//SubmitDescriptorHeap(_currentSrvCbvHeap);

		if(_internals->swapChains.size() > 0)
		{
			_currentCommandBuffer = GetCommandBuffer();
			_currentCommandBuffer->Retain();
			_currentCommandBuffer->Begin();
			VkCommandBuffer commandBuffer = _currentCommandBuffer->GetCommandBuffer();

			for(VulkanSwapChain *swapChain : _internals->swapChains)
			{
				swapChain->Prepare(commandBuffer);
			}

			_internals->currentRenderPassIndex = 0;
			_internals->currentDrawableResourceIndex = 0;
			for(const VulkanRenderPass &renderPass : _internals->renderPasses)
			{
				if(renderPass.type != VulkanRenderPass::Type::Default && renderPass.type != VulkanRenderPass::Type::Convert)
				{
//					RenderAPIRenderPass(_currentCommandBuffer, renderPass);
					_internals->currentRenderPassIndex += 1;
					continue;
				}
				SetupRendertargets(commandBuffer, renderPass);

				if(renderPass.drawables.size() > 0)
				{
					//Draw drawables
					for(VulkanDrawable *drawable : renderPass.drawables)
					{
						//TODO: Sort drawables by camera and root signature
						/*if(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature != _currentRootSignature)
						{
							_currentRootSignature = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature;
							commandList->SetGraphicsRootSignature(_currentRootSignature->signature);

							// Set the one big descriptor heap for the whole frame
							ID3D12DescriptorHeap* srvCbvHeaps[] = { _currentSrvCbvHeap->_heap };
							commandList->SetDescriptorHeaps(_countof(srvCbvHeaps), srvCbvHeaps);
						}*/
						RenderDrawable(commandBuffer, drawable);
					}

					_internals->currentDrawableResourceIndex += 1;
				}

				_internals->currentRenderPassIndex += 1;
			}

			vk::CmdEndRenderPass(commandBuffer);

			for(VulkanSwapChain *swapChain : _internals->swapChains)
			{
				swapChain->Finalize(commandBuffer);
			}
		}

		// Execute all command lists
/*		std::vector<ID3D12CommandList*> commandLists;
		_lock.Lock();
		_submittedCommandLists->Enumerate<D3D12CommandList>([&](D3D12CommandList *list, size_t index, bool &stop) {
			commandLists.push_back(list->GetCommandList());
			list->_fenceValue = _scheduledFenceValue;
		});
		_executedCommandLists->AddObjectsFromArray(_submittedCommandLists);
		_submittedCommandLists->RemoveAllObjects();
		_lock.Unlock();
		if(commandLists.size() > 0)
			_commandQueue->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

		_commandQueue->Signal(_fence, _scheduledFenceValue++);*/



		std::vector<VkSemaphore> presentSemaphores;
		std::vector<VkSemaphore> renderSemaphores;
		for(VulkanSwapChain *swapChain : _internals->swapChains)
		{
			presentSemaphores.push_back(swapChain->GetCurrentPresentSemaphore());
			renderSemaphores.push_back(swapChain->GetCurrentRenderSemaphore());
		}

		_currentCommandBuffer->End();
		SubmitCommandBuffer(_currentCommandBuffer);
		_currentCommandBuffer = nullptr;

		std::vector<VkCommandBuffer> buffers;
		_lock.Lock();
		if(_submittedCommandBuffers->GetCount() == 0)
		{
			_lock.Unlock();
			return;
		}

		buffers.reserve(_submittedCommandBuffers->GetCount());
		_submittedCommandBuffers->Enumerate<VulkanCommandBuffer>([&](VulkanCommandBuffer *buffer, int i, bool &stop){
			buffers.push_back(buffer->_commandBuffer);
			_executedCommandBuffers->AddObject(buffer);
		});
		_submittedCommandBuffers->RemoveAllObjects();
		_lock.Unlock();

		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = buffers.size();
		submitInfo.pCommandBuffers = buffers.data();
		submitInfo.waitSemaphoreCount = presentSemaphores.size();
		submitInfo.pWaitSemaphores = presentSemaphores.data();
		submitInfo.signalSemaphoreCount = renderSemaphores.size();
		submitInfo.pSignalSemaphores = renderSemaphores.data();
		submitInfo.pWaitDstStageMask = &pipelineStageFlags;

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));

		for(VulkanSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->PresentBackBuffer(_workQueue);
		}

		RNVulkanValidate(vk::QueueWaitIdle(_workQueue));
		vk::DeviceWaitIdle(GetVulkanDevice()->GetDevice());

		_executedCommandBuffers->RemoveAllObjects();

		_currentFrame ++;
	}

	void VulkanRenderer::SetupRendertargets(VkCommandBuffer commandBuffer, const VulkanRenderPass &renderpass)
	{
		//TODO: Call PrepareAsRendertargetForFrame() only once per framebuffer per frame
		renderpass.framebuffer->PrepareAsRendertargetForFrame();
		renderpass.framebuffer->SetAsRendertarget(commandBuffer, renderpass.renderPass->GetClearColor(), renderpass.renderPass->GetClearDepth(), renderpass.renderPass->GetClearStencil());

		//Setup viewport and scissor rect
		Rect cameraRect = renderpass.renderPass->GetFrame();

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.width = cameraRect.width;
		viewport.height = cameraRect.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::CmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = static_cast<uint32_t>(cameraRect.width);
		scissor.extent.height = static_cast<uint32_t>(cameraRect.height);
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		vk::CmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	//TODO: Merge parts of this with SubmitRenderPass and call it in here
	void VulkanRenderer::SubmitCamera(Camera *camera, Function &&function)
	{
		VulkanRenderPass renderPass;
		renderPass.drawables.resize(0);

		renderPass.type = VulkanRenderPass::Type::Default;
		renderPass.renderPass = camera->GetRenderPass();
		renderPass.previousRenderPass = nullptr;

		renderPass.shaderHint = camera->GetShaderHint();
		renderPass.overrideMaterial = camera->GetMaterial();

		renderPass.viewPosition = camera->GetWorldPosition();
		renderPass.viewMatrix = camera->GetViewMatrix();
		renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		renderPass.projectionMatrix = camera->GetProjectionMatrix();
		renderPass.projectionMatrix.m[5] *= -1.0f;
		renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		renderPass.projectionViewMatrix = renderPass.projectionMatrix * renderPass.viewMatrix;
		renderPass.directionalShadowDepthTexture = nullptr;

		Framebuffer *framebuffer = camera->GetRenderPass()->GetFramebuffer();
		VulkanSwapChain *newSwapChain = nullptr;
		newSwapChain = framebuffer->Downcast<VulkanFramebuffer>()->GetSwapChain();
		renderPass.framebuffer = framebuffer->Downcast<VulkanFramebuffer>();

		if(newSwapChain)
		{
			bool notIncluded = true;
			for(VulkanSwapChain *swapChain : _internals->swapChains)
			{
				if(swapChain == newSwapChain)
				{
					notIncluded = false;
					break;
				}
			}

			if(notIncluded)
			{
				_internals->swapChains.push_back(newSwapChain);
			}
		}

		_internals->currentRenderPassIndex = _internals->renderPasses.size();
		_internals->renderPasses.push_back(renderPass);

		// Create drawables
		function();

		size_t numberOfDrawables = _internals->renderPasses[_internals->currentRenderPassIndex].drawables.size();
		_internals->totalDrawableCount += numberOfDrawables;

		if(numberOfDrawables > 0) _internals->currentDrawableResourceIndex += 1;

		const Array *nextRenderPasses = renderPass.renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop) {
			SubmitRenderPass(nextPass, renderPass.renderPass);
		});
	}

	void VulkanRenderer::SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass)
	{
		VulkanRenderPass vulkanRenderPass;
		vulkanRenderPass.drawables.resize(0);

		PostProcessingAPIStage *apiStage = renderPass->Downcast<PostProcessingAPIStage>();
		PostProcessingStage *ppStage = renderPass->Downcast<PostProcessingStage>();

		vulkanRenderPass.renderPass = renderPass;
		vulkanRenderPass.previousRenderPass = previousRenderPass;

		vulkanRenderPass.shaderHint = Shader::UsageHint::Default;
		vulkanRenderPass.overrideMaterial = ppStage ? ppStage->GetMaterial() : nullptr;

		vulkanRenderPass.directionalShadowDepthTexture = nullptr;

		if(!apiStage)
		{
			vulkanRenderPass.type = VulkanRenderPass::Type::Default;
		}
		else
		{
			switch(apiStage->GetType())
			{
				case PostProcessingAPIStage::Type::ResolveMSAA:
				{
					vulkanRenderPass.type = VulkanRenderPass::Type::ResolveMSAA;
					break;
				}
				case PostProcessingAPIStage::Type::Blit:
				{
					vulkanRenderPass.type = VulkanRenderPass::Type::Blit;
					break;
				}
				case PostProcessingAPIStage::Type::Convert:
				{
					vulkanRenderPass.type = VulkanRenderPass::Type::Convert;

/*					if(!_ppConvertMaterial)
					{
						_ppConvertMaterial = Material::WithShaders(_defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_vertex")), _defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_blit_fragment")));
					}
					vulkanRenderPass.overrideMaterial = _ppConvertMaterial;*/
					break;
				}
			}
		}

		Framebuffer *framebuffer = renderPass->GetFramebuffer();
		VulkanSwapChain *newSwapChain = nullptr;
		newSwapChain = framebuffer->Downcast<VulkanFramebuffer>()->GetSwapChain();
		vulkanRenderPass.framebuffer = framebuffer->Downcast<VulkanFramebuffer>();

		if(newSwapChain)
		{
			bool notIncluded = true;
			for (VulkanSwapChain *swapChain : _internals->swapChains)
			{
				if (swapChain == newSwapChain)
				{
					notIncluded = false;
					break;
				}
			}

			if (notIncluded)
			{
				_internals->swapChains.push_back(newSwapChain);
			}
		}

		_internals->currentRenderPassIndex = _internals->renderPasses.size();
		_internals->renderPasses.push_back(vulkanRenderPass);

		if(ppStage || vulkanRenderPass.type == VulkanRenderPass::Type::Convert)
		{
			//Submit fullscreen quad drawable
/*			if(!_defaultPostProcessingDrawable)
			{
				Mesh *planeMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f, 0.0f)), Vector3(0.0f), Vector2(1.0f, 1.0f));
				Material *planeMaterial = Material::WithShaders(GetDefaultShader(Shader::Type::Vertex, nullptr), GetDefaultShader(Shader::Type::Fragment, nullptr));

				_lock.Lock();
				_defaultPostProcessingDrawable = static_cast<D3D12Drawable*>(CreateDrawable());
				_defaultPostProcessingDrawable->mesh = planeMesh->Retain();
				_defaultPostProcessingDrawable->material = planeMaterial->Retain();
				_lock.Unlock();
			}*/

			/*			Texture *sourceTexture = d3dRenderPass.previousRenderPass->GetFramebuffer()->GetColorTexture(0);
						if(d3dRenderPass.type == D3D12RenderPass::Type::Convert)
						{
							_defaultPostProcessingDrawable->material->RemoveAllTextures();
							_defaultPostProcessingDrawable->material->AddTexture(sourceTexture);
						}*/
//			SubmitDrawable(_defaultPostProcessingDrawable);

/*			size_t numberOfDrawables = _internals->renderPasses[_internals->currentRenderPassIndex].drawables.size();
			_internals->totalDrawableCount += numberOfDrawables;

			if(numberOfDrawables > 0)
				_internals->currentDrawableResourceIndex += 1;*/
		}

		const Array *nextRenderPasses = renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop)
												{
													SubmitRenderPass(nextPass, renderPass);
												});
	}

/*	void VulkanRenderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		_internals->renderPass.drawableHead = nullptr;

		_internals->renderPass.viewMatrix = camera->GetViewMatrix();
		_internals->renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		_internals->renderPass.projectionMatrix = camera->GetProjectionMatrix();
		_internals->renderPass.projectionMatrix.m[5] *= -1.0f;
		_internals->renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		_internals->renderPass.projectionViewMatrix = _internals->renderPass.projectionMatrix * _internals->renderPass.viewMatrix;

		// Submit drawables
		function();

		VulkanFramebuffer *framebuffer = static_cast<VulkanFramebuffer *>(camera->GetFramebuffer());
		Vector2 size = _mainWindow->GetSize();

		//Happens to be 0 when the window is closed.
		if(size.x < 0.01f || size.y < 0.01f)
		{
			size.x = 1.0f;
			size.y = 1.0f;
		}

		if(!framebuffer)
			framebuffer = _mainWindow->GetFramebuffer();

		VulkanBackBuffer *backbuffer = _mainWindow->GetActiveBackbuffer();
		uint32_t framebufferIndex = backbuffer->GetImageIndex();

		VulkanCommandBuffer *bufferObject = GetCommandBuffer();
		bufferObject->Begin();
		VkCommandBuffer commandBuffer = bufferObject->GetCommandBuffer();

		VkImageMemoryBarrier postPresentBarrier = {};
		postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		postPresentBarrier.pNext = NULL;
		postPresentBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		postPresentBarrier.image = static_cast<VulkanTexture *>(framebuffer->GetColorTexture(framebufferIndex))->GetImage();

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &postPresentBarrier);





		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.width = size.x;
		viewport.height = size.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::CmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = static_cast<uint32_t>(size.x);
		scissor.extent.height = static_cast<uint32_t>(size.y);
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		vk::CmdSetScissor(commandBuffer, 0, 1, &scissor);

		VulkanDrawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(commandBuffer, drawable);
			drawable = drawable->_next;
		}

		vk::CmdEndRenderPass(commandBuffer);

		// Add a present memory barrier to the end of the command buffer
		// This will transform the frame buffer color attachment to a
		// new layout for presenting it to the windowing system integration
		VkImageMemoryBarrier prePresentBarrier = {};
		prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		prePresentBarrier.pNext = NULL;
		prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		prePresentBarrier.image = static_cast<VulkanTexture *>(framebuffer->GetColorTexture(framebufferIndex))->GetImage();

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &prePresentBarrier);
		bufferObject->End();

		VkSemaphore presentSemaphore = backbuffer->GetPresentSemaphore();
		VkSemaphore renderSemaphore = backbuffer->GetRenderSemaphore();
		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderSemaphore;
		submitInfo.pWaitDstStageMask = &pipelineStageFlags;

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));
	}*/

	bool VulkanRenderer::SupportsTextureFormat(const String *format) const
	{
		return false;
	}
	bool VulkanRenderer::SupportsDrawMode(DrawMode mode) const
	{
		return false;
	}

	size_t VulkanRenderer::GetAlignmentForType(PrimitiveType type) const
	{
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
				return 4;

			case PrimitiveType::Vector2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;
		}
	}
	size_t VulkanRenderer::GetSizeForType(PrimitiveType type) const
	{
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
				return 4;

			case PrimitiveType::Vector2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			case PrimitiveType::Matrix:
				return 64;
		}
	}

	void VulkanRenderer::CreateMipMapForTexture(VulkanTexture *texture)
	{
		_lock.Lock();
		_mipMapTextures->AddObject(texture);
		_lock.Unlock();
	}

	void VulkanRenderer::CreateMipMaps()
	{
		if(_mipMapTextures->GetCount() == 0)
			return;

		VulkanCommandBuffer *commandBuffer = GetCommandBuffer();
		commandBuffer->Begin();

		_mipMapTextures->Enumerate<VulkanTexture>([&](VulkanTexture *texture, size_t index, bool &stop) {

			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), 1, texture->GetDescriptor().mipMaps-1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			for(uint16 i = 0; i < texture->GetDescriptor().mipMaps-1; i++)
			{
				VkImageBlit imageBlit = {};

				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.mipLevel = i;
				imageBlit.srcSubresource.baseArrayLayer = 0;
				imageBlit.srcSubresource.layerCount = 1;

				imageBlit.srcOffsets[0].x = 0;
				imageBlit.srcOffsets[0].y = 0;
				imageBlit.srcOffsets[0].z = 0;
				imageBlit.srcOffsets[1].x = texture->GetDescriptor().GetWidthForMipMapLevel(i);
				imageBlit.srcOffsets[1].y = texture->GetDescriptor().GetHeightForMipMapLevel(i);
				imageBlit.srcOffsets[1].z = 1;

				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.mipLevel = i+1;
				imageBlit.dstSubresource.baseArrayLayer = 0;
				imageBlit.dstSubresource.layerCount = 1;

				imageBlit.dstOffsets[0].x = 0;
				imageBlit.dstOffsets[0].y = 0;
				imageBlit.dstOffsets[0].z = 0;
				imageBlit.dstOffsets[1].x = texture->GetDescriptor().GetWidthForMipMapLevel(i+1);
				imageBlit.dstOffsets[1].y = texture->GetDescriptor().GetHeightForMipMapLevel(i+1);
				imageBlit.dstOffsets[1].z = 1;

				vk::CmdBlitImage(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->GetVulkanImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

				VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), i + 1, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), 0, texture->GetDescriptor().mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

		commandBuffer->End();
		SubmitCommandBuffer(commandBuffer);

		_mipMapTextures->RemoveAllObjects();
	}

	GPUBuffer *VulkanRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		void *data = malloc(length);
		return (new VulkanGPUBuffer(this, data, length, usageOptions));
	}
	GPUBuffer *VulkanRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		void *data = malloc(length);
		memcpy(data, bytes, length);
		return (new VulkanGPUBuffer(this, data, length, usageOptions));
	}

	ShaderLibrary *VulkanRenderer::CreateShaderLibraryWithFile(const String *file)
	{
		return new VulkanShaderLibrary(file);
	}

	ShaderLibrary *VulkanRenderer::CreateShaderLibraryWithSource(const String *source)
	{
		RN_ASSERT(-1, "NOT IMPLEMENTED!");
		return nullptr;
	}

	ShaderLibrary *VulkanRenderer::GetDefaultShaderLibrary()
	{
		return _defaultShaderLibrary;
	}

	Shader *VulkanRenderer::GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint usageHint)
	{
		LockGuard<Lockable> lock(_lock);

		Shader *shader = nullptr;
		if(type == Shader::Type::Vertex)
		{
			if(usageHint == Shader::UsageHint::Depth)
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("depth_vertex"), options);
			}
			else
			{
				const String *skyDefine = options? options->GetDefines()->GetObjectForKey<const String>(RNCSTR("RN_SKY")) : nullptr;
				if(skyDefine && !skyDefine->IsEqual(RNCSTR("0")))	//Use a different shader for the sky
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("sky_vertex"), options);
				}
				else
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("gouraud_vertex"), options);
				}
			}
		}
		else if(type == Shader::Type::Fragment)
		{
			if(usageHint == Shader::UsageHint::Depth)
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("depth_fragment"), options);
			}
			else
			{
				const String *skyDefine = options? options->GetDefines()->GetObjectForKey<const String>(RNCSTR("RN_SKY")) : nullptr;
				if(skyDefine && !skyDefine->IsEqual(RNCSTR("0")))	//Use a different shader for the sky
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("sky_fragment"), options);
				}
				else
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("gouraud_fragment"), options);
				}
			}
		}

		return shader;
	}

	Texture *VulkanRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		VulkanTexture *texture = new VulkanTexture(descriptor, this);
		return texture;
	}

	Framebuffer *VulkanRenderer::CreateFramebuffer(const Vector2 &size)
	{
		return new VulkanFramebuffer(size, this);
	}

	void VulkanRenderer::FillUniformBuffer(uint8 *buffer, VulkanDrawable *drawable, Shader *shader, size_t &offset)
	{
		Material *overrideMaterial = _internals->renderPasses[_internals->currentRenderPassIndex].overrideMaterial;
		Material::Properties mergedMaterialProperties = drawable->material->GetMergedProperties(overrideMaterial);
		const VulkanRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		buffer += offset;

		shader->GetSignature()->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
			offset = descriptor->GetOffset() + descriptor->GetSize();

			switch(descriptor->GetIdentifier())
			{
				case Shader::UniformDescriptor::Identifier::Time:
				{
					float temp = static_cast<float>(Kernel::GetSharedInstance()->GetTotalTime());
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->modelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewMatrix:
				{
					Matrix result = renderPass.viewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrix:
				{
					Matrix result = renderPass.projectionViewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.viewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.projectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.projectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->inverseModelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrix:
				{
					Matrix result = renderPass.inverseViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrix:
				{
					Matrix result = renderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseProjectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseProjectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AmbientColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.ambientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DiffuseColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.diffuseColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpecularColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.specularColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::EmissiveColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.emissiveColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::TextureTileFactor:
				{
					float temp = mergedMaterialProperties.textureTileFactor;
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AlphaToCoverageClamp:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.alphaToCoverageClamp.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraPosition:
				{
					RN::Vector3 cameraPosition = renderPass.viewPosition;
					std::memcpy(buffer + descriptor->GetOffset(), &cameraPosition.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					size_t lightCount = renderPass.directionalLights.size();
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					size_t lightCount = renderPass.directionalLights.size();
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalLights[0], (16 + 16) * lightCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatricesCount:
				{
					size_t matrixCount = renderPass.directionalShadowMatrices.size();
					if(matrixCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &matrixCount, descriptor->GetSize());
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices:
				{
					size_t matrixCount = renderPass.directionalShadowMatrices.size();
					if (matrixCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalShadowMatrices[0].m[0], 64 * matrixCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowInfo:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalShadowInfo.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::Custom:
				{
					//TODO: Implement custom shader variables!
					break;
				}

				default:
					break;
			}
		});
	}

	Drawable *VulkanRenderer::CreateDrawable()
	{
		VulkanDrawable *newDrawable = new VulkanDrawable();
		return newDrawable;
	}

	void VulkanRenderer::DeleteDrawable(Drawable *drawable)
	{
		delete drawable;
	}

	void VulkanRenderer::SubmitLight(const Light *light)
	{
		_lock.Lock();
		VulkanRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		_lock.Unlock();

		if(light->GetType() == Light::Type::DirectionalLight)
		{
			if(renderPass.directionalLights.size() < 5) //TODO: Don't hardcode light limit here
			{
				renderPass.directionalLights.push_back(VulkanLightDirectional{ light->GetForward(), 0.0f, light->GetColor() });
			}

			//TODO: Allow more lights with shadows or prevent multiple light with shadows overwriting each other
			if(light->HasShadows())
			{
				renderPass.directionalShadowDepthTexture = light->GetShadowDepthTexture()->Downcast<VulkanTexture>();
				renderPass.directionalShadowMatrices = light->GetShadowMatrices();
				renderPass.directionalShadowInfo = Vector2(1.0f / light->GetShadowParameters().resolution);
			}
		}
	}

	void VulkanRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		VulkanDrawable *drawable = static_cast<VulkanDrawable *>(tdrawable);
		drawable->AddUniformStateIfNeeded(_internals->currentDrawableResourceIndex);

		_lock.Lock();
		VulkanRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		_lock.Unlock();

		Material *material = drawable->material;
		if(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].dirty)
		{
			//TODO: Fix the camera situation...
			_lock.Lock();
			const VulkanPipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(material, drawable->mesh, renderPass.framebuffer, renderPass.shaderHint, renderPass.overrideMaterial);
			VulkanUniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState);
			_lock.Unlock();

			RN_ASSERT(pipelineState && uniformState, "Failed to create pipeline or uniform state for drawable!");
			drawable->UpdateRenderingState(_internals->currentDrawableResourceIndex, pipelineState, uniformState);
		}

		const VulkanPipelineState *pipelineState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState;

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = NULL;
		descriptorSetAllocateInfo.descriptorPool = _descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = &pipelineState->rootSignature->descriptorSetLayout;
		descriptorSetAllocateInfo.descriptorSetCount = 1;

		RNVulkanValidate(vk::AllocateDescriptorSets(GetVulkanDevice()->GetDevice(), &descriptorSetAllocateInfo, &drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet));
		VkDescriptorSet descriptorSet = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet;

		VulkanUniformState *uniformState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
		std::vector<VkDescriptorBufferInfo> constantBufferDescriptorInfoArray;
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		size_t binding = 0;
		if(uniformState->vertexConstantBuffer)
		{
			GPUBuffer *gpuBuffer = uniformState->vertexConstantBuffer->Advance(0, 0);
			uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());
			size_t offset = 0;
			FillUniformBuffer(buffer, drawable, pipelineState->descriptor.vertexShader, offset);
			gpuBuffer->Invalidate();

			VkDescriptorBufferInfo constantBufferDescriptorInfo = {};
			constantBufferDescriptorInfo.buffer = gpuBuffer->Downcast<VulkanGPUBuffer>()->GetVulkanBuffer();
			constantBufferDescriptorInfo.offset = 0;
			constantBufferDescriptorInfo.range = gpuBuffer->GetLength();
			constantBufferDescriptorInfoArray.push_back(constantBufferDescriptorInfo);

			VkWriteDescriptorSet writeConstantDescriptorSet = {};
			writeConstantDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeConstantDescriptorSet.pNext = NULL;
			writeConstantDescriptorSet.dstSet = descriptorSet;
			writeConstantDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeConstantDescriptorSet.dstBinding = binding++;
			writeConstantDescriptorSet.pBufferInfo = &constantBufferDescriptorInfo;
			writeConstantDescriptorSet.descriptorCount = 1;
			writeDescriptorSets.push_back(writeConstantDescriptorSet);
		}
		if (uniformState->fragmentConstantBuffer)
		{
			GPUBuffer *gpuBuffer = uniformState->fragmentConstantBuffer->Advance(0, 0);
			uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());
			size_t offset = 0;
			FillUniformBuffer(buffer, drawable, pipelineState->descriptor.fragmentShader, offset);
			gpuBuffer->Invalidate();

			VkDescriptorBufferInfo constantBufferDescriptorInfo = {};
			constantBufferDescriptorInfo.buffer = gpuBuffer->Downcast<VulkanGPUBuffer>()->GetVulkanBuffer();
			constantBufferDescriptorInfo.offset = 0;
			constantBufferDescriptorInfo.range = gpuBuffer->GetLength();
			constantBufferDescriptorInfoArray.push_back(constantBufferDescriptorInfo);

			VkWriteDescriptorSet writeConstantDescriptorSet = {};
			writeConstantDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeConstantDescriptorSet.pNext = NULL;
			writeConstantDescriptorSet.dstSet = descriptorSet;
			writeConstantDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeConstantDescriptorSet.dstBinding = binding++;
			writeConstantDescriptorSet.pBufferInfo = &constantBufferDescriptorInfo;
			writeConstantDescriptorSet.descriptorCount = 1;
			writeDescriptorSets.push_back(writeConstantDescriptorSet);
		}

		binding += pipelineState->rootSignature->samplers->GetCount();

		std::vector<VkDescriptorImageInfo> imageBufferDescriptorInfoArray;
		imageBufferDescriptorInfoArray.reserve(material->GetTextures()->GetCount());
		material->GetTextures()->Enumerate<VulkanTexture>([&](VulkanTexture *texture, size_t index, bool &stop) {
			VkDescriptorImageInfo imageBufferDescriptorInfo = {};
			imageBufferDescriptorInfo.imageView = texture->_imageView;
			imageBufferDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageBufferDescriptorInfoArray.push_back(imageBufferDescriptorInfo);

			VkWriteDescriptorSet writeImageDescriptorSet = {};
			writeImageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeImageDescriptorSet.pNext = NULL;
			writeImageDescriptorSet.dstSet = descriptorSet;
			writeImageDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writeImageDescriptorSet.dstBinding = binding++;
			writeImageDescriptorSet.pImageInfo = &imageBufferDescriptorInfoArray[index];
			writeImageDescriptorSet.descriptorCount = 1;

			writeDescriptorSets.push_back(writeImageDescriptorSet);

			if(index >= pipelineState->rootSignature->textureCount - 1) stop = true;
		});

		vk::UpdateDescriptorSets(GetVulkanDevice()->GetDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

		// Push into the queue
		_lock.Lock();
		renderPass.drawables.push_back(drawable);
		_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->textureCount;
		_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->constantBufferCount;
		_lock.Unlock();
	}

	void VulkanRenderer::RenderDrawable(VkCommandBuffer commandBuffer, VulkanDrawable *drawable)
	{
		const VulkanPipelineState *pipelineState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState;
		const VulkanRootSignature *rootSignature = pipelineState->rootSignature;

		vk::CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rootSignature->pipelineLayout, 0, 1, &drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet, 0, NULL);
		vk::CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState->state);

		VulkanGPUBuffer *buffer = static_cast<VulkanGPUBuffer *>(drawable->mesh->GetGPUVertexBuffer());
		VulkanGPUBuffer *indices = static_cast<VulkanGPUBuffer *>(drawable->mesh->GetGPUIndicesBuffer());

		VkDeviceSize offsets[1] = { 0 };
		// Bind mesh vertex buffer
		vk::CmdBindVertexBuffers(commandBuffer, 0, 1, &buffer->_buffer, offsets);
		// Bind mesh index buffer
		vk::CmdBindIndexBuffer(commandBuffer, indices->_buffer, 0, drawable->mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices)->GetType() == PrimitiveType::Uint16? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		// Render mesh vertex buffer using it's indices
		vk::CmdDrawIndexed(commandBuffer, drawable->mesh->GetIndicesCount(), 1, 0, 0, 0);

		_currentDrawableIndex += 1;
	}
}
