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
#include "RNVulkanDynamicBuffer.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace RN
{
	RNDefineMeta(VulkanRenderer, Renderer)

	VulkanRenderer::VulkanRenderer(VulkanRendererDescriptor *descriptor, VulkanDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_currentFrame(0),
		_completedFrame(-1),
		_mipMapTextures(new Array()),
		_submittedCommandBuffers(new Array()),
		_executedCommandBuffers(new Array()),
		_currentCommandBuffer(nullptr),
		_currentResourcesCommandBuffer(nullptr),
		_commandBufferPool(new Array()),
        _defaultPostProcessingDrawable(nullptr),
		_currentMultiviewLayer(0),
		_currentMultiviewCount(0),
		_currentMultiviewFallbackRenderPass(nullptr)
	{
		vk::GetDeviceQueue(device->GetDevice(), device->GetWorkQueue(), 0, &_workQueue);

		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vk::GetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vk::GetDeviceProcAddr;
		vulkanFunctions.vkGetPhysicalDeviceProperties = vk::GetPhysicalDeviceProperties;
		vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vk::GetPhysicalDeviceMemoryProperties;
		vulkanFunctions.vkAllocateMemory = vk::AllocateMemory;
		vulkanFunctions.vkFreeMemory = vk::FreeMemory;
		vulkanFunctions.vkMapMemory = vk::MapMemory;
		vulkanFunctions.vkUnmapMemory = vk::UnmapMemory;
		vulkanFunctions.vkFlushMappedMemoryRanges = vk::FlushMappedMemoryRanges;
		vulkanFunctions.vkInvalidateMappedMemoryRanges = vk::InvalidateMappedMemoryRanges;
		vulkanFunctions.vkBindBufferMemory = vk::BindBufferMemory;
		vulkanFunctions.vkBindImageMemory = vk::BindImageMemory;
		vulkanFunctions.vkGetBufferMemoryRequirements = vk::GetBufferMemoryRequirements;
		vulkanFunctions.vkGetImageMemoryRequirements = vk::GetImageMemoryRequirements;
		vulkanFunctions.vkCreateBuffer = vk::CreateBuffer;
		vulkanFunctions.vkDestroyBuffer = vk::DestroyBuffer;
		vulkanFunctions.vkCreateImage = vk::CreateImage;
		vulkanFunctions.vkDestroyImage = vk::DestroyImage;
		vulkanFunctions.vkCmdCopyBuffer = vk::CmdCopyBuffer;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.device = device->GetDevice();
		allocatorCreateInfo.physicalDevice = device->GetPhysicalDevice();
		allocatorCreateInfo.instance = device->GetInstance()->GetInstance();
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
		RNVulkanValidate(vmaCreateAllocator(&allocatorCreateInfo, &_internals->memoryAllocator));

		//Create command pool
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device->GetWorkQueue();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		RNVulkanValidate(vk::CreateCommandPool(device->GetDevice(), &cmdPoolInfo, nullptr, &_commandPool));

		//Create descriptor pool
		VkDescriptorPoolSize uniformBufferPoolSize = {};
		uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPoolSize.descriptorCount = 20000;
		VkDescriptorPoolSize storageBufferPoolSize = {};
		storageBufferPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storageBufferPoolSize.descriptorCount = 20000;
		VkDescriptorPoolSize textureBufferPoolSize = {};
		textureBufferPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		textureBufferPoolSize.descriptorCount = 10000;
		VkDescriptorPoolSize samplerBufferPoolSize = {};
		samplerBufferPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerBufferPoolSize.descriptorCount = 10000;
		std::vector<VkDescriptorPoolSize> poolSizes = { uniformBufferPoolSize, storageBufferPoolSize, samplerBufferPoolSize, textureBufferPoolSize };

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = NULL;
		descriptorPoolInfo.poolSizeCount = poolSizes.size();
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = 100000;
		descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		RNVulkanValidate(vk::CreateDescriptorPool(device->GetDevice(), &descriptorPoolInfo, GetAllocatorCallback(), &_descriptorPool));

		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneVulkan:/Shaders.json"));
		_dynamicBufferPool = new VulkanDynamicBufferPool();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		_mipMapTextures->Release();
		vmaDestroyAllocator(_internals->memoryAllocator);
		delete _dynamicBufferPool;
	}

	VkRenderPass VulkanRenderer::GetVulkanRenderPass(VulkanFramebuffer *framebuffer, VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags, uint8 multiviewCount)
	{
		return _internals->stateCoordinator.GetRenderPassState(framebuffer, resolveFramebuffer, flags, multiviewCount)->renderPass;
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
		VulkanCommandBuffer *commandBuffer = nullptr;

		if(_commandBufferPool->GetCount() == 0)
		{
			commandBuffer = new VulkanCommandBuffer(GetVulkanDevice()->GetDevice(), _commandPool);
			commandBuffer->_commandBuffer = CreateVulkanCommandBuffer();
		}
		else
		{
			commandBuffer = _commandBufferPool->GetLastObject<VulkanCommandBuffer>();
			commandBuffer->Retain();
			_commandBufferPool->RemoveObjectAtIndex(_commandBufferPool->GetCount() - 1);
		}

		return commandBuffer->Autorelease();
	}

	VulkanCommandBuffer *VulkanRenderer::StartResourcesCommandBuffer()
	{
		_currentResourcesCommandBufferLock.Lock();
		if(!_currentResourcesCommandBuffer)
        {
            _currentResourcesCommandBuffer = GetCommandBuffer()->Retain();
		    _currentResourcesCommandBuffer->Begin();
        }

		return _currentResourcesCommandBuffer;
	}

    void VulkanRenderer::EndResourcesCommandBuffer()
    {
        RN_DEBUG_ASSERT(_currentResourcesCommandBuffer, "No active Resources command buffer!");
	    _currentResourcesCommandBufferLock.Unlock();
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

	void VulkanRenderer::UpdateFrameFences()
	{
		//Check fence status
		int index = 0;
		int freeFenceIndex = -1;
		for(VkFence fence : _frameFences)
		{
			if(_frameFenceValues[index] != -1)
			{
				VkResult status = vk::GetFenceStatus(GetVulkanDevice()->GetDevice(), fence);
				if(status == VK_SUCCESS)
				{
					_completedFrame = _frameFenceValues[index];
					ReleaseFrameResources(_frameFenceValues[index]);
					RNVulkanValidate(vk::ResetFences(GetVulkanDevice()->GetDevice(), 1, &fence));

					_frameFenceValues[index] = -1;
					freeFenceIndex = index;
				}
			}
			else
			{
				freeFenceIndex = index;
			}

			index += 1;
		}

		if(freeFenceIndex == -1)
		{
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			VkFence frameFence;
			RNVulkanValidate(vk::CreateFence(GetVulkanDevice()->GetDevice(), &fenceInfo, GetAllocatorCallback(), &frameFence));
			_frameFences.push_back(frameFence);
			_frameFenceValues.push_back(0);

			freeFenceIndex = _frameFences.size() - 1;
		}

		_frameFenceValues[freeFenceIndex] = _currentFrame;
		_currentFrameFenceIndex = freeFenceIndex;
	}

	void VulkanRenderer::ReleaseFrameResources(uint32 frame)
	{
		//Delete command lists that finished execution on the graphics card (the command allocator needs to be alive the whole time)
		for(int i = _executedCommandBuffers->GetCount() - 1; i >= 0; i--)
		{
			VulkanCommandBuffer *commandBuffer = _executedCommandBuffers->GetObjectAtIndex<VulkanCommandBuffer>(i);
			if(commandBuffer->_frameValue <= frame)
			{
				_commandBufferPool->AddObject(commandBuffer);
				_executedCommandBuffers->RemoveObjectAtIndex(i);
				commandBuffer->Reset();
			}
		}

		//Free other frame resources such as unused framebuffers and imageviews
		for(int i = _internals->frameResources.size()-1; i >= 0; i--)
		{
			VulkanFrameResource &frameResource = _internals->frameResources[i];
			if(frameResource.frame <= frame)
			{
				if(frameResource.finishedCallback)
				{
					frameResource.finishedCallback();
				}

				_internals->frameResources.erase(_internals->frameResources.begin() + i);
			}
		}
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
//		_currentRootSignature = nullptr;

        UpdateFrameFences(); //Releases resources of frames that finished
        CreateMipMaps();

		_currentResourcesCommandBufferLock.Lock();
		if(_currentResourcesCommandBuffer)
		{
			_currentResourcesCommandBuffer->End();
            _submittedCommandBuffers->AddObject(_currentResourcesCommandBuffer);
			SafeRelease(_currentResourcesCommandBuffer);
		}
		_currentResourcesCommandBufferLock.Unlock();

		_lock.Lock();
        VkSemaphore resourceUploadsSemaphore = VK_NULL_HANDLE;
		if(_submittedCommandBuffers->GetCount() > 0)
		{
			std::vector<VkCommandBuffer> buffers;

			buffers.reserve(_submittedCommandBuffers->GetCount());
			_submittedCommandBuffers->Enumerate<VulkanCommandBuffer>([&](VulkanCommandBuffer *buffer, int i, bool &stop){
				buffer->_frameValue = _currentFrame;
				buffers.push_back(buffer->_commandBuffer);
                _executedCommandBuffers->AddObject(buffer);
			});

            //Create a semaphore for the render queue to wait for the resource upload queue
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkDevice device = GetVulkanDevice()->GetDevice();
            vk::CreateSemaphore(device, &semaphoreInfo, nullptr, &resourceUploadsSemaphore);
            AddFrameFinishedCallback([device, resourceUploadsSemaphore](){
                vk::DestroySemaphore(device, resourceUploadsSemaphore, nullptr);
            });

			//Submit command buffers
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = buffers.size();
			submitInfo.pCommandBuffers = buffers.data();
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &resourceUploadsSemaphore;

			std::vector<VkPipelineStageFlags> pipelineStageFlags;
			pipelineStageFlags.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
			submitInfo.pWaitDstStageMask = pipelineStageFlags.data();

			RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));

			//RNVulkanValidate(vk::DeviceWaitIdle(GetVulkanDevice()->GetDevice()));
			_submittedCommandBuffers->RemoveAllObjects();
		}
		_lock.Unlock();

		//SubmitCamera is called for each camera and creates lists of drawables per camera
		function();

		_dynamicBufferPool->Update(this, _currentFrame, _completedFrame);
		UpdateDescriptorSets();

		for(VulkanSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->AcquireBackBuffer();
		}

		_currentCommandBuffer = GetCommandBuffer();
		_currentCommandBuffer->Retain();
		_currentCommandBuffer->Begin();

		if(_internals->swapChains.size() > 0)
		{
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
					RenderAPIRenderPass(_currentCommandBuffer, renderPass);
					_internals->currentRenderPassIndex += 1;
					continue;
				}

				//Set shadow depth texture layout for reading
				if(renderPass.directionalShadowDepthTexture)
				{
					VulkanTexture::SetImageLayout(commandBuffer, renderPass.directionalShadowDepthTexture->GetVulkanImage(), 0, renderPass.directionalShadowDepthTexture->GetDescriptor().mipMaps, 0, renderPass.directionalShadowDepthTexture->GetDescriptor().depth, VK_IMAGE_ASPECT_DEPTH_BIT, renderPass.directionalShadowDepthTexture->GetCurrentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VulkanTexture::BarrierIntent::ShaderSource);
					renderPass.directionalShadowDepthTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}

                //Set textures layout for reading for render targets that are used in this frame
                for(VulkanTexture *vulkanTexture : renderPass.renderTargetsUsedInShader)
                {
                    if(vulkanTexture->GetCurrentLayout() == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) continue; //Nothing to do if the layout is already correct

                    VulkanTexture::SetImageLayout(commandBuffer, vulkanTexture->GetVulkanImage(), 0, vulkanTexture->GetDescriptor().mipMaps, 0, vulkanTexture->GetDescriptor().depth, VK_IMAGE_ASPECT_COLOR_BIT, vulkanTexture->GetCurrentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VulkanTexture::BarrierIntent::ShaderSource);
                    vulkanTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }

				//Set previous framebuffer texture layout for reading
				if(renderPass.previousRenderPass && renderPass.previousRenderPass->GetFramebuffer())
				{
					Texture *texture = renderPass.previousRenderPass->GetFramebuffer()->GetColorTexture(0);
					if(texture)
					{
						VulkanTexture *vulkanTexture = texture->Downcast<VulkanTexture>();
						if(vulkanTexture->GetCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
						{
							VulkanTexture::SetImageLayout(commandBuffer, vulkanTexture->GetVulkanImage(), 0, vulkanTexture->GetDescriptor().mipMaps, 0, vulkanTexture->GetDescriptor().depth, VK_IMAGE_ASPECT_COLOR_BIT, vulkanTexture->GetCurrentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VulkanTexture::BarrierIntent::ShaderSource);
							vulkanTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
						}
					}
				}

				SetupRendertargets(commandBuffer, renderPass);

				if(renderPass.drawables.size() > 0)
				{
					//TODO: Sort drawables by camera and root signature? Maybe not...
					//Draw drawables
					uint32 stepSize = 0;
					uint32 stepSizeIndex = 0;
					for(size_t i = 0; i < renderPass.drawables.size(); i+= stepSize)
					{
						stepSize = renderPass.instanceSteps[stepSizeIndex++];
						RenderDrawable(commandBuffer, renderPass.drawables[i], stepSize);
					}

					//RNDebug("draw calls: " << renderPass.instanceSteps.size());

					_internals->currentDrawableResourceIndex += 1;
				}

				vk::CmdEndRenderPass(commandBuffer);

				//Set shadow depth texture layout for writing
				if(renderPass.directionalShadowDepthTexture)
				{
					VulkanTexture::SetImageLayout(commandBuffer, renderPass.directionalShadowDepthTexture->GetVulkanImage(), 0, renderPass.directionalShadowDepthTexture->GetDescriptor().mipMaps, 0, renderPass.directionalShadowDepthTexture->GetDescriptor().depth, VK_IMAGE_ASPECT_DEPTH_BIT, renderPass.directionalShadowDepthTexture->GetCurrentLayout(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VulkanTexture::BarrierIntent::RenderTarget);
					renderPass.directionalShadowDepthTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
				}

				//Set textures layout for writing for render targets that are used in this frame
				//TODO: This should also support depth/stencil attachments, currently only correct for color attachments!
				for(VulkanTexture *vulkanTexture : renderPass.renderTargetsUsedInShader)
				{
					if(vulkanTexture->GetCurrentLayout() == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) continue; //Nothing to do if the layout is already correct

					VulkanTexture::SetImageLayout(commandBuffer, vulkanTexture->GetVulkanImage(), 0, vulkanTexture->GetDescriptor().mipMaps, 0, vulkanTexture->GetDescriptor().depth, VK_IMAGE_ASPECT_COLOR_BIT, vulkanTexture->GetCurrentLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VulkanTexture::BarrierIntent::RenderTarget);
					vulkanTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				}

				//Set previous framebuffer texture layout for writing
				if(renderPass.previousRenderPass && renderPass.previousRenderPass->GetFramebuffer())
				{
					Texture *texture = renderPass.previousRenderPass->GetFramebuffer()->GetColorTexture(0);
					if (texture)
					{
						VulkanTexture *vulkanTexture = texture->Downcast<VulkanTexture>();
						if(vulkanTexture->GetCurrentLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
						{
							VulkanTexture::SetImageLayout(commandBuffer, vulkanTexture->GetVulkanImage(), 0, vulkanTexture->GetDescriptor().mipMaps, 0, vulkanTexture->GetDescriptor().depth, VK_IMAGE_ASPECT_COLOR_BIT, vulkanTexture->GetCurrentLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VulkanTexture::BarrierIntent::RenderTarget);
							vulkanTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
						}
					}
				}

				_internals->currentRenderPassIndex += 1;
			}

			for(VulkanSwapChain *swapChain : _internals->swapChains)
			{
				swapChain->Finalize(commandBuffer);
			}
		}

		_dynamicBufferPool->FlushAllBuffers();

		//Prepare command buffer submission
		std::vector<VkSemaphore> presentSemaphores;
		std::vector<VkSemaphore> renderSemaphores;

        if(resourceUploadsSemaphore) presentSemaphores.push_back(resourceUploadsSemaphore); //Wait until all resources are available

		for(VulkanSwapChain *swapChain : _internals->swapChains)
		{
			VkSemaphore presentSemaphore = swapChain->GetCurrentPresentSemaphore();
			VkSemaphore renderSemaphore = swapChain->GetCurrentRenderSemaphore();
			if(presentSemaphore != VK_NULL_HANDLE) presentSemaphores.push_back(presentSemaphore);
			if(renderSemaphore != VK_NULL_HANDLE) renderSemaphores.push_back(renderSemaphore);
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
			buffer->_frameValue = _currentFrame;
			buffers.push_back(buffer->_commandBuffer);
			_executedCommandBuffers->AddObject(buffer);
		});
		_submittedCommandBuffers->RemoveAllObjects();
		_lock.Unlock();

		//Submit command buffers
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = buffers.size();
		submitInfo.pCommandBuffers = buffers.data();
		submitInfo.waitSemaphoreCount = presentSemaphores.size();
		submitInfo.pWaitSemaphores = presentSemaphores.data();
		submitInfo.signalSemaphoreCount = renderSemaphores.size();
		submitInfo.pSignalSemaphores = renderSemaphores.data();

		std::vector<VkPipelineStageFlags> pipelineStageFlags;
		for(int i = 0; i < presentSemaphores.size(); i++)
		{
			pipelineStageFlags.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		}
		submitInfo.pWaitDstStageMask = pipelineStageFlags.data();

		RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, _frameFences[_currentFrameFenceIndex]));

		for(VulkanSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->PresentBackBuffer(_workQueue);
		}

		_currentFrame ++;
	}

	void VulkanRenderer::SetupRendertargets(VkCommandBuffer commandBuffer, const VulkanRenderPass &renderpass)
	{
		//TODO: Call PrepareAsRendertargetForFrame() only once per framebuffer per frame, find new solution for setting things up for msaa while reusing a framebuffer?
		renderpass.framebuffer->PrepareAsRendertargetForFrame(renderpass.resolveFramebuffer, renderpass.renderPass->GetFlags(), renderpass.multiviewLayer, renderpass.multiviewCameraInfo.size());
		renderpass.framebuffer->SetAsRendertarget(commandBuffer, renderpass.resolveFramebuffer, renderpass.renderPass->GetClearColor(), renderpass.renderPass->GetClearDepth(), renderpass.renderPass->GetClearStencil());

		//Setup viewport and scissor rect
		Rect cameraRect = renderpass.renderPass->GetFrame();

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.x = cameraRect.x;
		viewport.y = cameraRect.y;
		viewport.width = cameraRect.width;
		viewport.height = cameraRect.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::CmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = static_cast<uint32_t>(cameraRect.width);
		scissor.extent.height = static_cast<uint32_t>(cameraRect.height);
		scissor.offset.x = cameraRect.x;
		scissor.offset.y = cameraRect.y;

		vk::CmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	//TODO: Merge parts of this with SubmitRenderPass and call it in here
	void VulkanRenderer::SubmitCamera(Camera *camera, Function &&function)
	{
		VulkanRenderPass renderPass;

		const Array *multiviewCameras = camera->GetMultiviewCameras();
		if(multiviewCameras && multiviewCameras->GetCount() > 0)
		{
			if(multiviewCameras->GetCount() > 1 && GetVulkanDevice()->GetSupportsMultiview())
			{
				if(multiviewCameras->GetCount() <= GetVulkanDevice()->GetMaxMultiviewViewCount() || _currentMultiviewCount > 0)
				{
					multiviewCameras->Enumerate<Camera>([&](Camera *multiviewCamera, size_t index, bool &stop){
						if(_currentMultiviewCount > 0 && index < _currentMultiviewLayer) return;
						if(_currentMultiviewCount > 0 && index >= _currentMultiviewLayer + _currentMultiviewCount)
						{
							stop = true;
							return;
						}

						VulkanRenderPassCameraInfo cameraInfo;

						cameraInfo.viewPosition = multiviewCamera->GetWorldPosition();
						cameraInfo.viewMatrix = multiviewCamera->GetViewMatrix();
						cameraInfo.inverseViewMatrix = multiviewCamera->GetInverseViewMatrix();

						Matrix clipSpaceCorrectionMatrix;
                        clipSpaceCorrectionMatrix.m[5] = -1.0f;
						cameraInfo.projectionMatrix = clipSpaceCorrectionMatrix * multiviewCamera->GetProjectionMatrix();
						cameraInfo.inverseProjectionMatrix = multiviewCamera->GetInverseProjectionMatrix();
						cameraInfo.projectionViewMatrix = cameraInfo.projectionMatrix * cameraInfo.viewMatrix;

						renderPass.multiviewCameraInfo.push_back(cameraInfo);
					});
				}
				else
				{
					int increment = GetVulkanDevice()->GetMaxMultiviewViewCount();
					int i = 0;
					while(increment > 0)
					{
						_currentMultiviewLayer = i;
						_currentMultiviewCount = increment;

						if(increment == 1)
						{
							_currentMultiviewFallbackRenderPass = camera->GetRenderPass();
							SubmitCamera(multiviewCameras->GetObjectAtIndex<Camera>(i), std::move(function));
							_currentMultiviewFallbackRenderPass = nullptr;
						}
						else
						{
							SubmitCamera(camera, std::move(function));
						}

						_currentMultiviewLayer = 0;
						_currentMultiviewCount = 0;

						i += increment;
						increment = std::min(static_cast<int>(GetVulkanDevice()->GetMaxMultiviewViewCount()), static_cast<int>(multiviewCameras->GetCount() - i));
					}

					return;
				}
			}
			else
			{
				//If multiview is not supported or there is only one multiview camera, render them individually (and ignore their parent camera)
				multiviewCameras->Enumerate<Camera>([&](Camera *multiviewCamera, size_t index, bool &stop){
					_currentMultiviewLayer = index;
					_currentMultiviewCount = 1;
					_currentMultiviewFallbackRenderPass = camera->GetRenderPass();

					SubmitCamera(multiviewCamera, std::move(function));

					_currentMultiviewLayer = 0;
					_currentMultiviewCount = 0;
					_currentMultiviewFallbackRenderPass = nullptr;
				});

				return;
			}
		}

		_internals->currentPipelineState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;

		RenderPass *cameraRenderPass = _currentMultiviewFallbackRenderPass? _currentMultiviewFallbackRenderPass : camera->GetRenderPass();

		renderPass.drawables.resize(0);
		renderPass.multiviewLayer = _currentMultiviewLayer;

		renderPass.type = VulkanRenderPass::Type::Default;
		renderPass.renderPass = cameraRenderPass;
		renderPass.previousRenderPass = nullptr;

		renderPass.resolveFramebuffer = nullptr;

		renderPass.shaderHint = camera->GetShaderHint();
		renderPass.overrideMaterial = camera->GetMaterial();

        renderPass.cameraInfo.camera = camera;

		renderPass.cameraInfo.viewPosition = camera->GetWorldPosition();
		renderPass.cameraInfo.viewMatrix = camera->GetViewMatrix();
		renderPass.cameraInfo.inverseViewMatrix = camera->GetInverseViewMatrix();

		Matrix clipSpaceCorrectionMatrix;
		clipSpaceCorrectionMatrix.m[5] = -1.0f;
		renderPass.cameraInfo.projectionMatrix = clipSpaceCorrectionMatrix * camera->GetProjectionMatrix();
		renderPass.cameraInfo.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		renderPass.cameraInfo.projectionViewMatrix = renderPass.cameraInfo.projectionMatrix * renderPass.cameraInfo.viewMatrix;
		renderPass.directionalShadowDepthTexture = nullptr;

		renderPass.cameraAmbientColor = camera->GetAmbientColor();

		renderPass.cameraClipDistance = Vector2(camera->GetClipNear(), camera->GetClipFar());
		renderPass.cameraFogDistance = Vector2(camera->GetFogNear(), camera->GetFogFar());
		renderPass.cameraFogColor0 = camera->GetFogColor0();
		renderPass.cameraFogColor1 = camera->GetFogColor1();

		Framebuffer *framebuffer = cameraRenderPass->GetFramebuffer();
		if(!framebuffer) return;

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

		const Array *nextRenderPasses = renderPass.renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop) {
			PostProcessingAPIStage *apiStage = nextPass->Downcast<PostProcessingAPIStage>();
			if(apiStage && apiStage->GetType() == PostProcessingAPIStage::Type::ResolveMSAA)
			{
				//MSAA framebuffer needs to be set before creating the drawables

				Framebuffer *framebuffer = apiStage->GetFramebuffer();
				VulkanSwapChain *newSwapChain = nullptr;
				newSwapChain = framebuffer->Downcast<VulkanFramebuffer>()->GetSwapChain();
				VulkanFramebuffer *vulkanFramebuffer = framebuffer->Downcast<VulkanFramebuffer>();

				_internals->renderPasses[_internals->currentRenderPassIndex].resolveFramebuffer = vulkanFramebuffer;
			}
		});

		// Create drawables
		function();

		size_t numberOfDrawables = _internals->renderPasses[_internals->currentRenderPassIndex].drawables.size();
		_internals->totalDrawableCount += numberOfDrawables;

		if(numberOfDrawables > 0) _internals->currentDrawableResourceIndex += 1;

		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop) {
			SubmitRenderPass(nextPass, renderPass, std::forward<Function>(function));
		});
	}

	void VulkanRenderer::SubmitRenderPass(RenderPass *renderPass, VulkanRenderPass &previousRenderPass, Function &&function)
	{
		_internals->currentPipelineState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;

		VulkanRenderPass vulkanRenderPass;
		vulkanRenderPass.drawables.clear();

		PostProcessingAPIStage *apiStage = renderPass->Downcast<PostProcessingAPIStage>();
		PostProcessingStage *ppStage = renderPass->Downcast<PostProcessingStage>();

		if(vulkanRenderPass.type != VulkanRenderPass::Type::ResolveMSAA && !ppStage && vulkanRenderPass.type != VulkanRenderPass::Type::Convert)
		{
			vulkanRenderPass.cameraInfo = previousRenderPass.cameraInfo;
			vulkanRenderPass.multiviewCameraInfo = previousRenderPass.multiviewCameraInfo;
			vulkanRenderPass.multiviewLayer = previousRenderPass.multiviewLayer;
		}
		else
		{
			vulkanRenderPass.multiviewLayer = 0;
		}
		
		vulkanRenderPass.directionalShadowDepthTexture = nullptr;

		vulkanRenderPass.renderPass = renderPass;
		vulkanRenderPass.previousRenderPass = previousRenderPass.renderPass;

		vulkanRenderPass.framebuffer = nullptr;
		vulkanRenderPass.resolveFramebuffer = nullptr;

		vulkanRenderPass.shaderHint = Shader::UsageHint::Default;
		vulkanRenderPass.overrideMaterial = ppStage ? ppStage->GetMaterial() : nullptr;

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

		if(vulkanRenderPass.type != VulkanRenderPass::Type::ResolveMSAA)
		{
			_internals->currentRenderPassIndex = _internals->renderPasses.size();
			_internals->renderPasses.push_back(vulkanRenderPass);

			if(ppStage || vulkanRenderPass.type == VulkanRenderPass::Type::Convert)
			{
				//Submit fullscreen quad drawable
				if(!_defaultPostProcessingDrawable)
				{
					Mesh *planeMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, -90.0f, 0.0f)), Vector3(0.0f), Vector2(1.0f, 1.0f));
					Material *planeMaterial = Material::WithShaders(GetDefaultShader(Shader::Type::Vertex, nullptr), GetDefaultShader(Shader::Type::Fragment, nullptr));

					_lock.Lock();
					_defaultPostProcessingDrawable = static_cast<VulkanDrawable*>(CreateDrawable());
					_defaultPostProcessingDrawable->mesh = planeMesh->Retain();
					_defaultPostProcessingDrawable->material = planeMaterial->Retain();
					_lock.Unlock();
				}

				SubmitDrawable(_defaultPostProcessingDrawable);

				size_t numberOfDrawables = _internals->renderPasses[_internals->currentRenderPassIndex].drawables.size();
				_internals->totalDrawableCount += numberOfDrawables;

				if(numberOfDrawables > 0)
					_internals->currentDrawableResourceIndex += 1;
			}
			else
			{
				// Create drawables
				function();
			}
		}
		else
		{
			_internals->renderPasses[_internals->currentRenderPassIndex].resolveFramebuffer = vulkanRenderPass.framebuffer;
		}

		const Array *nextRenderPasses = renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop) {
			SubmitRenderPass(nextPass, vulkanRenderPass, std::forward<Function>(function));
		});
	}

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
				return 48;
			case PrimitiveType::Matrix4x4:
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

		VulkanCommandBuffer *commandBuffer = StartResourcesCommandBuffer();

		_mipMapTextures->Enumerate<VulkanTexture>([&](VulkanTexture *texture, size_t index, bool &stop) {

		    //TODO: Fix mipmap generation for texture arrays
			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VulkanTexture::BarrierIntent::CopySource);
			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), 1, texture->GetDescriptor().mipMaps-1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VulkanTexture::BarrierIntent::CopyDestination);
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

				VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), i + 1, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VulkanTexture::BarrierIntent::CopySource);
			}

			VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), texture->GetVulkanImage(), 0, texture->GetDescriptor().mipMaps, 0, texture->GetDescriptor().depth, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VulkanTexture::BarrierIntent::ShaderSource);
		});

		EndResourcesCommandBuffer();

		_mipMapTextures->RemoveAllObjects();
	}

	GPUBuffer *VulkanRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		return (new VulkanGPUBuffer(this, nullptr, length, usageOptions));
	}
	GPUBuffer *VulkanRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		void *data = malloc(length);
		memcpy(data, bytes, length);
		return (new VulkanGPUBuffer(this, data, length, usageOptions));
	}

	VulkanDynamicBufferReference *VulkanRenderer::GetConstantBufferReference(size_t size, size_t index, GPUResource::UsageOptions usageOptions)
	{
		VulkanDynamicBufferReference *reference = _dynamicBufferPool->GetDynamicBufferReference(size, index, usageOptions);
		return reference;
	}

	void VulkanRenderer::UpdateDynamicBufferReference(VulkanDynamicBufferReference *reference, bool align)
	{
		LockGuard<Lockable> lock(_lock);
		return _dynamicBufferPool->UpdateDynamicBufferReference(reference, align);
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

	Texture *VulkanRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		VulkanTexture *texture = new VulkanTexture(descriptor, this);
		return texture;
	}

	Framebuffer *VulkanRenderer::CreateFramebuffer(const Vector2 &size)
	{
		return new VulkanFramebuffer(size, this);
	}

	void VulkanRenderer::FillUniformBuffer(Shader::ArgumentBuffer *argumentBuffer, VulkanDynamicBufferReference *dynamicBufferReference, VulkanDrawable *drawable)
	{
		GPUBuffer *gpuBuffer = dynamicBufferReference->dynamicBuffer->GetActiveBuffer();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer()) + dynamicBufferReference->offset;

		Material *overrideMaterial = _internals->renderPasses[_internals->currentRenderPassIndex].overrideMaterial;
		const Material::Properties &mergedMaterialProperties = drawable->material->GetMergedProperties(overrideMaterial);
		const VulkanRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		argumentBuffer->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {

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

				case Shader::UniformDescriptor::Identifier::InverseModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->inverseModelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::NormalMatrix:
				{
					Matrix normalMatrix = drawable->inverseModelMatrix.GetTransposed();
					std::memcpy(buffer + descriptor->GetOffset(), &normalMatrix.m[0], 12);
					std::memcpy(buffer + descriptor->GetOffset() + 16, &normalMatrix.m[4], 12);
					std::memcpy(buffer + descriptor->GetOffset() + 32, &normalMatrix.m[8], 12);
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewMatrix:
				{
					Matrix result = renderPass.cameraInfo.viewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
						for(int i = 0; i < viewCount; i++)
						{
							Matrix result = renderPass.multiviewCameraInfo[i].viewMatrix * drawable->modelMatrix;
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrix:
				{
					Matrix result = renderPass.cameraInfo.projectionViewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrixMultiview:
				{
                    if(renderPass.multiviewCameraInfo.size() > 0)
                    {
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            Matrix result = renderPass.multiviewCameraInfo[i].projectionViewMatrix * drawable->modelMatrix;
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
                        }
                    }
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.cameraInfo.viewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewMatrixMultiview:
				{
                    if(renderPass.multiviewCameraInfo.size() > 0)
                    {
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, renderPass.multiviewCameraInfo[i].viewMatrix.m, 64);
                        }
                    }
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.cameraInfo.projectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewProjectionMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, renderPass.multiviewCameraInfo[i].projectionViewMatrix.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::ProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.cameraInfo.projectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ProjectionMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, renderPass.multiviewCameraInfo[i].projectionMatrix.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrix:
				{
					Matrix result = renderPass.cameraInfo.inverseViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            Matrix result = renderPass.multiviewCameraInfo[i].inverseViewMatrix * drawable->inverseModelMatrix;
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrix:
				{
					Matrix result = renderPass.cameraInfo.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            Matrix result = renderPass.multiviewCameraInfo[i].inverseProjectionViewMatrix * drawable->inverseModelMatrix;
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.cameraInfo.inverseViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, renderPass.multiviewCameraInfo[i].inverseViewMatrix.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.cameraInfo.inverseProjectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, renderPass.multiviewCameraInfo[i].inverseProjectionViewMatrix.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.cameraInfo.inverseProjectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseProjectionMatrixMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 64 * i, renderPass.multiviewCameraInfo[i].inverseProjectionMatrix.m, 64);
                        }
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraPosition:
				{
					Vector4 cameraPosition(renderPass.cameraInfo.viewPosition, 0.0f);
					std::memcpy(buffer + descriptor->GetOffset(), &cameraPosition.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraPositionMultiview:
				{
					if(renderPass.multiviewCameraInfo.size() > 0)
					{
                        size_t viewCount = std::min(renderPass.multiviewCameraInfo.size(), descriptor->GetElementCount());
                        for(int i = 0; i < viewCount; i++)
                        {
                            std::memcpy(buffer + descriptor->GetOffset() + 16 * i, &renderPass.multiviewCameraInfo[i].viewPosition.x, 16);
                        }
					}
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
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.textureTileFactor, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AlphaToCoverageClamp:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &mergedMaterialProperties.alphaToCoverageClamp.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraClipDistance:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraClipDistance.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraFogDistance:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogDistance.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraAmbientColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraAmbientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraFogColor0:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogColor0.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraFogColor1:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogColor1.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					uint32 lightCount = std::min(renderPass.directionalLights.size(), descriptor->GetElementCount());
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					size_t lightCount = std::min(renderPass.directionalLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalLights[0], (16 + 16) * lightCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatricesCount:
				{
					//TODO: Limit matrixCount to descriptor->GetElementCount() of Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices
					uint32 matrixCount = renderPass.directionalShadowMatrices.size();
					std::memcpy(buffer + descriptor->GetOffset(), &matrixCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices:
				{
					size_t matrixCount = std::min(renderPass.directionalShadowMatrices.size(), descriptor->GetElementCount());
					if(matrixCount > 0)
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

				case Shader::UniformDescriptor::Identifier::PointLights:
				{
					size_t lightCount = std::min(renderPass.pointLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.pointLights[0], (12 + 4 + 16) * lightCount);
					}
					if(lightCount < descriptor->GetElementCount())
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 16) * lightCount, 0, (12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpotLights:
				{
					size_t lightCount = std::min(renderPass.spotLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.spotLights[0], (12 + 4 + 12 + 4 + 16) * lightCount);
					}
					if(lightCount < descriptor->GetElementCount())
                    {
                    	std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 12 + 4 + 16) * lightCount, 0, (12 + 4 + 12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
                    }
					break;
				}

				case Shader::UniformDescriptor::Identifier::BoneMatrices:
				{
					if(drawable->skeleton)
					{
						size_t matrixCount = std::min(drawable->skeleton->_matrices.size(), descriptor->GetElementCount());
						if(matrixCount > 0)
						{
							std::memcpy(buffer + descriptor->GetOffset(), &drawable->skeleton->_matrices[0].m[0], 64 * matrixCount);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::Custom:
				{
					Object *object = mergedMaterialProperties.GetCustomShaderUniform(descriptor->GetNameHash());
					if(object)
					{
						if(object->IsKindOfClass(Value::GetMetaClass()))
						{
							Value *value = object->Downcast<Value>();
							switch(value->GetValueType())
							{
								case TypeTranslator<Vector2>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector2))
									{
										Vector2 vector = value->GetValue<Vector2>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector3>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector3))
									{
										Vector3 vector = value->GetValue<Vector3>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector4>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector4))
									{
										Vector4 vector = value->GetValue<Vector4>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Matrix>::value:
								{
									if(descriptor->GetSize() == sizeof(Matrix))
									{
										Matrix matrix = value->GetValue<Matrix>();
										std::memcpy(buffer + descriptor->GetOffset(), &matrix.m[0], descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Quaternion>::value:
								{
									if(descriptor->GetSize() == sizeof(Quaternion))
									{
										Quaternion quaternion = value->GetValue<Quaternion>();
										std::memcpy(buffer + descriptor->GetOffset(), &quaternion.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Color>::value:
								{
									if(descriptor->GetSize() == sizeof(Color))
									{
										Color color = value->GetValue<Color>();
										std::memcpy(buffer + descriptor->GetOffset(), &color.r, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
						else
						{
							Number *number = object->Downcast<Number>();
							switch(number->GetType())
							{
								case Number::Type::Int8:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int8 value = number->GetInt8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int16:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int16 value = number->GetInt16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int32:
								{
									if(descriptor->GetSize() == sizeof(int32))
									{
										int32 value = number->GetInt32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint8:
								{
									if(descriptor->GetSize() == sizeof(uint8))
									{
										uint8 value = number->GetUint8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint16:
								{
									if(descriptor->GetSize() == sizeof(uint16))
									{
										uint16 value = number->GetUint16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint32:
								{
									if(descriptor->GetSize() == sizeof(uint32))
									{
										uint32 value = number->GetUint32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Float32:
								{
									if(descriptor->GetSize() == sizeof(float))
									{
										float value = number->GetFloatValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Boolean:
								{
									if(descriptor->GetSize() == sizeof(bool))
									{
										bool value = number->GetBoolValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
					}
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
		renderPass.directionalShadowMatrices.clear();
		_lock.Unlock();

		if(light->GetType() == Light::Type::DirectionalLight)
		{
            renderPass.directionalLights.push_back(VulkanDirectionalLight{ light->GetForward(), 0.0f, light->GetFinalColor() });

			//TODO: Allow more lights with shadows or prevent multiple light with shadows overwriting each other
			if(light->HasShadows())
			{
				bool isShadowCamera = false;
				light->GetShadowDepthCameras()->Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
					if (renderPass.framebuffer == camera->GetRenderPass()->GetFramebuffer())
					{
						stop = true;
						isShadowCamera = true;
					}
				});

				if(!isShadowCamera)
				{
					renderPass.directionalShadowDepthTexture = light->GetShadowDepthTexture()->Downcast<VulkanTexture>();
				}
				
				light->GetShadowDepthCameras()->Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
					Matrix clipSpaceCorrectionMatrix;
					clipSpaceCorrectionMatrix.m[5] = 1.0f;
					Matrix shadowMatrix = clipSpaceCorrectionMatrix * camera->GetProjectionMatrix();

					shadowMatrix = shadowMatrix * camera->GetWorldTransform().GetInverse();
					renderPass.directionalShadowMatrices.push_back(shadowMatrix);
				});

				renderPass.directionalShadowInfo = Vector2(1.0f / light->GetShadowParameters().resolution);
			}
		}
		else if(light->GetType() == Light::Type::PointLight)
		{
            renderPass.pointLights.push_back(VulkanPointLight{ light->GetWorldPosition(), light->GetRange(), light->GetFinalColor() });
		}
		else if(light->GetType() == Light::Type::SpotLight)
		{
            renderPass.spotLights.push_back(VulkanSpotLight{ light->GetWorldPosition(), light->GetRange(), light->GetForward(), light->GetAngleCos(), light->GetFinalColor() });
		}
	}

	void VulkanRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		VulkanDrawable *drawable = static_cast<VulkanDrawable *>(tdrawable);
		drawable->AddCameraSpecificsIfNeeded(_internals->currentDrawableResourceIndex);

		_lock.Lock();
		VulkanRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		_lock.Unlock();

		Material *material = drawable->material;
		if(drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].dirty || drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].camera != renderPass.cameraInfo.camera)
		{
			//TODO: Fix the camera situation...
			_lock.Lock();
			const VulkanPipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(material, drawable->mesh, renderPass.framebuffer, renderPass.resolveFramebuffer, renderPass.shaderHint, renderPass.overrideMaterial, renderPass.renderPass->GetFlags(), renderPass.multiviewCameraInfo.size());
			VulkanUniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState);
			_lock.Unlock();

			RN_ASSERT(pipelineState && uniformState, "Failed to create pipeline or uniform state for drawable!");
			drawable->UpdateRenderingState(_internals->currentDrawableResourceIndex, renderPass.cameraInfo.camera, pipelineState, uniformState);

			if(!drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet)
			{
				drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet = new BufferedDescriptorSet();
			}

			drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet->UpdateLayout(pipelineState->rootSignature->descriptorSetLayout, _currentFrame);
		}

		//Vertex and fragment shaders need to explicitly be marked to support instancing in the shader library json
		RN::Shader *vertexShader = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState->descriptor.vertexShader;
		RN::Shader *fragmentShader = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState->descriptor.fragmentShader;
		bool canUseInstancing = (!vertexShader || vertexShader->GetHasInstancing()) && (!fragmentShader || fragmentShader->GetHasInstancing());

		//TODO: Use binding and type arrays in vulkan root signatures pipeline layout instead
		//Check if uniform buffers are the same, the object can't be part of the same instanced draw call if it doesn't share the same buffers (because they are full for example)
		if(canUseInstancing && _internals->currentInstanceDrawable && drawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->vertexConstantBuffers.size() == _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->vertexConstantBuffers.size() && drawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->fragmentConstantBuffers.size() == _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->fragmentConstantBuffers.size())
		{
			canUseInstancing = true;
			for(int i = 0; i < drawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->vertexConstantBuffers.size() && canUseInstancing; i++)
			{
				if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->vertexConstantBuffers[i]->dynamicBuffer != _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->vertexConstantBuffers[i]->dynamicBuffer)
				{
					canUseInstancing = false;
				}
			}

			for(int i = 0; i < drawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->fragmentConstantBuffers.size() && canUseInstancing; i++)
			{
				if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->fragmentConstantBuffers[i]->dynamicBuffer != _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->fragmentConstantBuffers[i]->dynamicBuffer)
				{
					canUseInstancing = false;
				}
			}
		}

		if(canUseInstancing && renderPass.instanceSteps.size() > 0 && renderPass.instanceSteps.back() >= std::min(vertexShader->GetMaxInstanceCount(), fragmentShader? fragmentShader->GetMaxInstanceCount() : -1))
		{
			canUseInstancing = false;
		}

		_lock.Lock();
		if(_internals->currentPipelineState == drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState && drawable->mesh == _internals->currentInstanceDrawable->mesh && drawable->material->GetTextures()->IsEqual(_internals->currentInstanceDrawable->material->GetTextures()) && canUseInstancing)
		{
			renderPass.instanceSteps.back() += 1; //Increase counter if the rendering state is the same
		}
		else
		{
			_internals->currentPipelineState = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState;
			_internals->currentInstanceDrawable = drawable;
			renderPass.instanceSteps.push_back(1); //Add new entry if the rendering state changed

			//This stuff should only be needed per draw call and not for any additional instances... hopefully
			drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet->Advance(_currentFrame, _completedFrame);
			_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->textureCount;
			_internals->totalDescriptorTables += drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState->rootSignature->constantBufferCount;
		}

		// Push into the queue
		renderPass.drawables.push_back(drawable);
		_lock.Unlock();
	}

	void VulkanRenderer::UpdateDescriptorSets()
	{
		_internals->currentRenderPassIndex = 0;
		_internals->currentDrawableResourceIndex = 0;

		uint32 totalConstantBufferCount = 0;
		uint32 totalTextureCount = 0;

		for(const VulkanRenderPass &renderPass : _internals->renderPasses)
		{
        	if(renderPass.type != VulkanRenderPass::Type::Default && renderPass.type != VulkanRenderPass::Type::Convert)
        	{
				_internals->currentRenderPassIndex += 1;
        		continue;
        	}

        	if(renderPass.drawables.size() > 0)
        	{
				uint32 stepSize = 0;
				uint32 stepSizeIndex = 0;
				for(size_t i = 0; i < renderPass.drawables.size(); i+= stepSize)
				{
					stepSize = renderPass.instanceSteps[stepSizeIndex++];

					totalConstantBufferCount += renderPass.drawables[i]->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->vertexConstantBuffers.size();
                    totalConstantBufferCount += renderPass.drawables[i]->_cameraSpecifics[_internals->currentRenderPassIndex].uniformState->fragmentConstantBuffers.size();

                    const VulkanPipelineState *pipelineState = renderPass.drawables[i]->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState;

					totalTextureCount += pipelineState->rootSignature->textureCount;
					_internals->currentDrawableResourceIndex += 1;
				}
			}

			_internals->currentRenderPassIndex += 1;
		}

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.reserve(totalConstantBufferCount + totalTextureCount);
		std::vector<VkDescriptorBufferInfo> constantBufferDescriptorInfoArray;
		constantBufferDescriptorInfoArray.reserve(totalConstantBufferCount);
		std::vector<VkDescriptorImageInfo> imageBufferDescriptorInfoArray;
		imageBufferDescriptorInfoArray.reserve(totalTextureCount);

		_internals->currentRenderPassIndex = 0;
		_internals->currentDrawableResourceIndex = 0;

		for(VulkanRenderPass &renderPass : _internals->renderPasses)
		{
			renderPass.renderTargetsUsedInShader.clear();

			if(renderPass.type != VulkanRenderPass::Type::Default && renderPass.type != VulkanRenderPass::Type::Convert)
			{
				_internals->currentRenderPassIndex += 1;
				continue;
			}

			if(renderPass.drawables.size() > 0)
			{
				size_t stepSize = 0;
				uint32 stepSizeIndex = 0;
				for(size_t i = 0; i < renderPass.drawables.size(); i+= stepSize)
				{
					stepSize = renderPass.instanceSteps[stepSizeIndex++];

					VulkanDrawable *drawable = renderPass.drawables[i];
					const VulkanPipelineState *pipelineState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState;

					VkDescriptorSet descriptorSet = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet->GetActiveDescriptorSet();

					VulkanUniformState *uniformState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
					if(uniformState->instanceAttributesBuffer)
					{
                        //These are not actually part of the descripter sets, but filling them with data here anyway
						Shader::ArgumentBuffer *argument = uniformState->instanceAttributesArgumentBuffer;

						//Setup per instance uniforms as vertex data for all instances that are part of this draw call
						for(size_t instance = 0; instance < stepSize; instance += 1)
						{
							if(instance > 0 && argument->GetMaxInstanceCount() == 1) break;

							VulkanUniformState *instanceUniformState = renderPass.drawables[i + instance]->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
                            VulkanDynamicBufferReference *instanceAttributesBuffer = instanceUniformState->instanceAttributesBuffer;
							UpdateDynamicBufferReference(instanceAttributesBuffer, instance == 0);
							FillUniformBuffer(argument,  instanceAttributesBuffer, renderPass.drawables[i + instance]);
						}
					}

					size_t counter = 0;
					for(size_t bufferIndex = 0; bufferIndex < uniformState->vertexConstantBuffers.size(); bufferIndex += 1)
					{
						Shader::ArgumentBuffer *argument = uniformState->constantBufferToArgumentMapping[counter++];

						//Setup uniforms for all instances that are part of this draw call
						for(size_t instance = 0; instance < stepSize; instance += 1)
						{
							if(instance > 0 && argument->GetMaxInstanceCount() == 1) break;

						    VulkanUniformState *instanceUniformState = renderPass.drawables[i + instance]->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
							UpdateDynamicBufferReference(
									instanceUniformState->vertexConstantBuffers[bufferIndex],
									instance == 0);
							FillUniformBuffer(argument,  instanceUniformState->vertexConstantBuffers[bufferIndex], renderPass.drawables[i + instance]);
						}

                        VulkanDynamicBufferReference *constantBuffer = uniformState->vertexConstantBuffers[bufferIndex];

						GPUBuffer *gpuBuffer = constantBuffer->dynamicBuffer->GetActiveBuffer();
						VkDescriptorBufferInfo constantBufferDescriptorInfo = {};
						constantBufferDescriptorInfo.buffer = gpuBuffer->Downcast<VulkanGPUBuffer>()->GetVulkanBuffer();
						constantBufferDescriptorInfo.offset = constantBuffer->offset;
						constantBufferDescriptorInfo.range = constantBuffer->size * std::min(stepSize, argument->GetMaxInstanceCount());
						constantBufferDescriptorInfoArray.push_back(constantBufferDescriptorInfo);

						VkWriteDescriptorSet writeConstantDescriptorSet = {};
						writeConstantDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeConstantDescriptorSet.pNext = NULL;
						writeConstantDescriptorSet.dstSet = descriptorSet;
						writeConstantDescriptorSet.descriptorType = (argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						writeConstantDescriptorSet.dstBinding = argument->GetIndex();
						writeConstantDescriptorSet.pBufferInfo = &constantBufferDescriptorInfoArray[constantBufferDescriptorInfoArray.size()-1];
						writeConstantDescriptorSet.descriptorCount = 1;

						writeDescriptorSets.push_back(writeConstantDescriptorSet);
					}

                    for(size_t bufferIndex = 0; bufferIndex < uniformState->fragmentConstantBuffers.size(); bufferIndex += 1)
					{
						Shader::ArgumentBuffer *argument = uniformState->constantBufferToArgumentMapping[counter++];

                        //Setup uniforms for all instances that are part of this draw call
                        for(size_t instance = 0; instance < stepSize; instance += 1)
                        {
							if(instance > 0 && argument->GetMaxInstanceCount() == 1) break;

                            VulkanUniformState *instanceUniformState = renderPass.drawables[i + instance]->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
							UpdateDynamicBufferReference(
									instanceUniformState->fragmentConstantBuffers[bufferIndex],
									instance == 0);
                            FillUniformBuffer(argument,  instanceUniformState->fragmentConstantBuffers[bufferIndex], renderPass.drawables[i + instance]);
                        }

                        VulkanDynamicBufferReference *constantBuffer = uniformState->fragmentConstantBuffers[bufferIndex];

						GPUBuffer *gpuBuffer = constantBuffer->dynamicBuffer->GetActiveBuffer();
						VkDescriptorBufferInfo constantBufferDescriptorInfo = {};
						constantBufferDescriptorInfo.buffer = gpuBuffer->Downcast<VulkanGPUBuffer>()->GetVulkanBuffer();
						constantBufferDescriptorInfo.offset = constantBuffer->offset;
						constantBufferDescriptorInfo.range = constantBuffer->size * std::min(stepSize, argument->GetMaxInstanceCount());
						constantBufferDescriptorInfoArray.push_back(constantBufferDescriptorInfo);

						VkWriteDescriptorSet writeConstantDescriptorSet = {};
						writeConstantDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeConstantDescriptorSet.pNext = NULL;
						writeConstantDescriptorSet.dstSet = descriptorSet;
						writeConstantDescriptorSet.descriptorType = (argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						writeConstantDescriptorSet.dstBinding = argument->GetIndex();
						writeConstantDescriptorSet.pBufferInfo = &constantBufferDescriptorInfoArray[constantBufferDescriptorInfoArray.size()-1];
						writeConstantDescriptorSet.descriptorCount = 1;

						writeDescriptorSets.push_back(writeConstantDescriptorSet);
					}

					//TODO: Support vertex shader textures
					Shader *fragmentShader = pipelineState->descriptor.fragmentShader;
					if(fragmentShader)
					{
						const Shader::Signature *signature = fragmentShader->GetSignature();
						signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop) {

							VkImageView imageView = VK_NULL_HANDLE;
							if(argument->GetMaterialTextureIndex() == Shader::ArgumentTexture::IndexDirectionalShadowTexture && renderPass.directionalShadowDepthTexture)
							{
								const VulkanTexture *materialTexture = renderPass.directionalShadowDepthTexture;
								imageView = materialTexture->_imageView;
							}
							else if(argument->GetMaterialTextureIndex() == Shader::ArgumentTexture::IndexFramebufferTexture && renderPass.previousRenderPass && renderPass.previousRenderPass->GetFramebuffer())
							{
								VulkanFramebuffer *framebuffer = renderPass.previousRenderPass->GetFramebuffer()->Downcast<VulkanFramebuffer>();
								Texture *texture = framebuffer->GetColorTexture();
								if(texture)
								{
									const VulkanTexture *framebufferTexture = texture->Downcast<VulkanTexture>();
									imageView = framebufferTexture->_imageView;
								}
								else
								{
									return;
								}
							}
							else if(argument->GetMaterialTextureIndex() >= drawable->material->GetTextures()->GetCount())
							{
								stop = true;
								return;
							}
							else
							{
								VulkanTexture *materialTexture = drawable->material->GetTextures()->GetObjectAtIndex<VulkanTexture>(argument->GetMaterialTextureIndex());
								imageView = materialTexture->_imageView;

                                if(materialTexture->GetDescriptor().usageHint & Texture::UsageHint::RenderTarget)
                                {
									//Add render targets to list of textures that needs to be transitioned for this render pass
                                    renderPass.renderTargetsUsedInShader.push_back(materialTexture);
                                }
							}

							VkDescriptorImageInfo imageBufferDescriptorInfo = {};
							imageBufferDescriptorInfo.imageView = imageView;
							imageBufferDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							imageBufferDescriptorInfoArray.push_back(imageBufferDescriptorInfo);

							VkWriteDescriptorSet writeImageDescriptorSet = {};
							writeImageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							writeImageDescriptorSet.pNext = NULL;
							writeImageDescriptorSet.dstSet = descriptorSet;
							writeImageDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
							writeImageDescriptorSet.dstBinding = argument->GetIndex();
							writeImageDescriptorSet.pImageInfo = &imageBufferDescriptorInfoArray[imageBufferDescriptorInfoArray.size() - 1];
							writeImageDescriptorSet.descriptorCount = 1;

							writeDescriptorSets.push_back(writeImageDescriptorSet);
						});
					}
				}

				_internals->currentDrawableResourceIndex += 1;
			}

			_internals->currentRenderPassIndex += 1;
		}

		if(writeDescriptorSets.size() > 0)
		{
			vk::UpdateDescriptorSets(GetVulkanDevice()->GetDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void VulkanRenderer::RenderDrawable(VkCommandBuffer commandBuffer, VulkanDrawable *drawable, uint32 instanceCount)
	{
		const VulkanPipelineState *pipelineState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].pipelineState;
        const VulkanUniformState *uniformState = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].uniformState;
		const VulkanRootSignature *rootSignature = pipelineState->rootSignature;

		VkDescriptorSet descriptorSet = drawable->_cameraSpecifics[_internals->currentDrawableResourceIndex].descriptorSet->GetActiveDescriptorSet();
		vk::CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rootSignature->pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
		vk::CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState->state);

		VulkanGPUBuffer *buffer = static_cast<VulkanGPUBuffer *>(drawable->mesh->GetGPUVertexBuffer());
		VulkanGPUBuffer *indices = static_cast<VulkanGPUBuffer *>(drawable->mesh->GetGPUIndicesBuffer());
        VulkanGPUBuffer *instanceAttributesBuffer = uniformState->instanceAttributesBuffer? static_cast<VulkanGPUBuffer *>(uniformState->instanceAttributesBuffer->dynamicBuffer->GetActiveBuffer()) : nullptr;

		//IF positions are separated, they will be in the first part of the buffer, everything else will be bound as the second binding, per instance data if provided through attributes are bound as a third buffer
        VkDeviceSize offsets[3];
        VkBuffer vertexBuffers[3];
        int attributesBufferIndex = 0;

        offsets[attributesBufferIndex] = 0;
        vertexBuffers[attributesBufferIndex++] = buffer->_buffer;

        if(pipelineState->vertexAttributeBufferCount > 1)
        {
            offsets[attributesBufferIndex] = drawable->mesh->GetVertexPositionsSeparatedSize();
            vertexBuffers[attributesBufferIndex++] = buffer->_buffer;
        }
        if(instanceAttributesBuffer)
        {
            offsets[attributesBufferIndex] = uniformState->instanceAttributesBuffer->offset;
            vertexBuffers[attributesBufferIndex++] = instanceAttributesBuffer->GetVulkanBuffer();
        }

        // Bind mesh vertex buffer
		vk::CmdBindVertexBuffers(commandBuffer, 0, attributesBufferIndex, vertexBuffers, offsets);
		if(drawable->mesh->GetIndicesCount() > 0)
		{
            // Bind mesh index buffer
			vk::CmdBindIndexBuffer(commandBuffer, indices->_buffer, 0, drawable->mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices)->GetType() == PrimitiveType::Uint16? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
            // Render mesh vertex buffer using it's indices
			vk::CmdDrawIndexed(commandBuffer, drawable->mesh->GetIndicesCount(), instanceCount, 0, 0, 0);
		}
		else
		{
			vk::CmdDraw(commandBuffer, drawable->mesh->GetVerticesCount(), instanceCount, 0, 0);
		}

		_currentDrawableIndex += 1;
	}

	void VulkanRenderer::RenderAPIRenderPass(VulkanCommandBuffer *commandList, const VulkanRenderPass &renderPass)
	{
		//TODO: Handle multiple and not existing textures
/*		Texture *sourceColorTexture = renderPass.previousRenderPass->GetFramebuffer()->GetColorTexture(0);
		VulkanTexture *sourceD3DColorTexture = nullptr;
		D3D12_RESOURCE_STATES oldColorSourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		if(sourceColorTexture)
		{
			sourceD3DColorTexture = sourceColorTexture->Downcast<D3D12Texture>();
			oldColorSourceState = sourceD3DColorTexture->_currentState;
		}

		Texture *sourceDepthTexture = renderPass.previousRenderPass->GetFramebuffer()->GetDepthStencilTexture();
		D3D12Texture *sourceD3DDepthTexture = nullptr;
		D3D12_RESOURCE_STATES oldDepthSourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (sourceColorTexture)
		{
			sourceD3DDepthTexture = sourceDepthTexture->Downcast<D3D12Texture>();
			oldDepthSourceState = sourceD3DDepthTexture->_currentState;
		}

		D3D12Framebuffer *destinationFramebuffer = renderPass.renderPass->GetFramebuffer()->Downcast<RN::D3D12Framebuffer>();

		Texture *destinationColorTexture = destinationFramebuffer->GetColorTexture(0);
		D3D12Texture *destinationD3DColorTexture = nullptr;
		ID3D12Resource *destinationColorResource = nullptr;
		D3D12_RESOURCE_STATES oldColorDestinationState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		DXGI_FORMAT targetColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		if(destinationColorTexture)
		{
			destinationD3DColorTexture = destinationColorTexture->Downcast<D3D12Texture>();
			targetColorFormat = destinationD3DColorTexture->_srvDescriptor.Format;
			oldColorDestinationState = destinationD3DColorTexture->_currentState;
			destinationColorResource = destinationD3DColorTexture->_resource;
		}
		else
		{
			targetColorFormat = destinationFramebuffer->_colorTargets[0]->d3dTargetViewDesc.Format;
			destinationColorResource = destinationFramebuffer->GetSwapChainColorBuffer();
		}


		Texture *destinationDepthTexture = destinationFramebuffer->GetDepthStencilTexture();
		D3D12Texture *destinationD3DDepthTexture = nullptr;
		ID3D12Resource *destinationDepthResource = nullptr;
		D3D12_RESOURCE_STATES oldDepthDestinationState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		DXGI_FORMAT targetDepthFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		if(destinationDepthTexture)
		{
			destinationD3DDepthTexture = destinationDepthTexture->Downcast<D3D12Texture>();
			targetDepthFormat = destinationD3DDepthTexture->_srvDescriptor.Format;
			oldDepthDestinationState = destinationD3DDepthTexture->_currentState;
			destinationDepthResource = destinationD3DDepthTexture->_resource;
		}
		else if(destinationFramebuffer->GetSwapChain() && destinationFramebuffer->GetSwapChain()->HasDepthBuffer())
		{
			targetDepthFormat = destinationFramebuffer->_depthStencilTarget->d3dTargetViewDesc.Format;
			destinationDepthResource = destinationFramebuffer->GetSwapChainDepthBuffer();
		}

		switch(targetDepthFormat)
		{
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			{
				targetDepthFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			}
			case DXGI_FORMAT_D32_FLOAT:
			{
				targetDepthFormat = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			{
				targetDepthFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			}
		}

		if(renderPass.type == D3D12RenderPass::Type::ResolveMSAA)
		{
			sourceD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

			if(destinationColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST));
			}

			//TODO: Handle multiple subresources?
			commandList->GetCommandList()->ResolveSubresource(destinationColorResource, 0, sourceD3DColorTexture->_resource, 0, targetColorFormat);

			sourceD3DColorTexture->TransitionToState(commandList, oldColorSourceState);
			if(destinationD3DColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, oldColorDestinationState);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
			}

			if(sourceD3DDepthTexture && destinationDepthResource)
			{
				sourceD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

				if(destinationDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RESOLVE_DEST));
				}

				//TODO: Handle multiple subresources?
				commandList->GetCommandList()->ResolveSubresource(destinationDepthResource, 0, sourceD3DDepthTexture->_resource, 0, targetDepthFormat);

				sourceD3DDepthTexture->TransitionToState(commandList, oldDepthSourceState);
				if(destinationD3DDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, oldDepthDestinationState);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_COMMON));
				}
			}
		}
		else if(renderPass.type == D3D12RenderPass::Type::Blit)
		{
			sourceD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

			if(destinationColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));
			}

			//TODO: Handle multiple subresources and 3D/Arrays?
			CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(destinationColorResource, 0);
			CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(sourceD3DColorTexture->_resource, 0);
			Rect frame = renderPass.renderPass->GetFrame();
			commandList->GetCommandList()->CopyTextureRegion(&destinationLocation, frame.x, frame.y, 0, &sourceLocation, nullptr);

			sourceD3DColorTexture->TransitionToState(commandList, oldColorSourceState);
			if(destinationD3DColorTexture)
			{
				destinationD3DColorTexture->TransitionToState(commandList, oldColorDestinationState);
			}
			else
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationColorResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));
			}

			if(sourceD3DDepthTexture && destinationDepthResource)
			{
				sourceD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

				if(destinationDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
				}

				//TODO: Handle multiple subresources and 3D/Arrays?
				CD3DX12_TEXTURE_COPY_LOCATION destinationLocation(destinationDepthResource, 0);
				CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(sourceD3DDepthTexture->_resource, 0);
				Rect frame = renderPass.renderPass->GetFrame();
				commandList->GetCommandList()->CopyTextureRegion(&destinationLocation, frame.x, frame.y, 0, &sourceLocation, nullptr);

				sourceD3DDepthTexture->TransitionToState(commandList, oldDepthSourceState);
				if (destinationD3DDepthTexture)
				{
					destinationD3DDepthTexture->TransitionToState(commandList, oldDepthDestinationState);
				}
				else
				{
					commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(destinationDepthResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
				}
			}
		}*/
	}

	void VulkanRenderer::AddFrameFinishedCallback(std::function<void()> callback, size_t frameOffset)
	{
		_internals->frameResources.push_back({ _currentFrame + frameOffset, callback });
	}
}
