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
#include "RNVulkanShader.h"
#include "RNVulkanShaderLibrary.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanSwapChain.h"
#include "RNVulkanTexture.h"
#include "RNVulkanTextureInfo.h"
#include "RNVulkanDynamicGPUBuffer.h"
#include "RNVulkanStaticGPUBuffer.h"
#include "../../../Source/Scene/RNLight.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace RN
{
	RNDefineMeta(VulkanRenderer, Renderer)

	static_assert(sizeof(DrawIndirectArguments) == sizeof(VkDrawIndirectCommand), "DrawIndirectArguments must match VkDrawIndirectCommand");
	static_assert(sizeof(DrawIndexedIndirectArguments) == sizeof(VkDrawIndexedIndirectCommand), "DrawIndexedIndirectArguments must match VkDrawIndexedIndirectCommand");

	String *VulkanRenderer::GetBackendFrameStatistics() const
	{
		const uint64 bytesPerMegabyte = 1024ull * 1024ull;
		auto bytesToMegabytes = [bytesPerMegabyte](uint64 bytes) -> uint64 {
			return (bytes + bytesPerMegabyte - 1) / bytesPerMegabyte;
		};

		auto estimateTextureBytes = [](const Texture::Descriptor &descriptor) -> uint64 {
			const VulkanTextureInfo::FormatInfo &formatInfo = VulkanTextureInfo::GetFormatInfo(descriptor.format);
			if(formatInfo.bytesPerBlock == 0)
				return 0;

			const uint32 blockWidth = std::max<uint32>(1, formatInfo.blockWidth);
			const uint32 blockHeight = std::max<uint32>(1, formatInfo.blockHeight);
			const uint32 imageLayers = descriptor.type == Texture::Type::Type3D ? 1 : descriptor.depth;
			const uint32 sampleCount = std::max<uint32>(1, descriptor.sampleCount);
			uint64 totalBytes = 0;

			for(uint32 mipmap = 0; mipmap < descriptor.mipMaps; mipmap++)
			{
				const uint32 mipWidth = descriptor.GetWidthForMipMapLevel(mipmap);
				const uint32 mipHeight = descriptor.GetHeightForMipMapLevel(mipmap);
				const uint32 mipDepth = descriptor.type == Texture::Type::Type3D ? std::max<uint32>(1, descriptor.depth >> mipmap) : 1;
				const uint32 blocksPerRow = std::max<uint32>(1, (mipWidth + blockWidth - 1) / blockWidth);
				const uint32 blockRows = std::max<uint32>(1, (mipHeight + blockHeight - 1) / blockHeight);
				totalBytes += static_cast<uint64>(blocksPerRow) * blockRows * mipDepth * imageLayers * sampleCount * formatInfo.bytesPerBlock;
			}

			return totalBytes;
		};

		VmaTotalStatistics allocatorStatistics;
		vmaCalculateStatistics(_internals->memoryAllocator, &allocatorStatistics);

		const VmaStatistics &totalStatistics = allocatorStatistics.total.statistics;
		const uint64 unusedBytes = totalStatistics.blockBytes - totalStatistics.allocationBytes;
		VmaBudget heapBudgets[VK_MAX_MEMORY_HEAPS];
		vmaGetHeapBudgets(_internals->memoryAllocator, heapBudgets);

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vk::GetPhysicalDeviceMemoryProperties(GetVulkanDevice()->GetPhysicalDevice(), &memoryProperties);

		std::ostringstream statsStream;
		statsStream << "\n  vulkan memory | used=" << bytesToMegabytes(totalStatistics.allocationBytes)
			<< "MiB reserved=" << bytesToMegabytes(totalStatistics.blockBytes)
			<< "MiB unused=" << bytesToMegabytes(unusedBytes)
			<< "MiB allocations=" << totalStatistics.allocationCount
			<< " blocks=" << totalStatistics.blockCount;

		uint32 swapchainFramebufferCount = 0;
		uint32 swapchainColorImageCount = 0;
		uint32 swapchainFragmentDensityImageCount = 0;
		uint64 swapchainColorBytes = 0;
		uint64 swapchainFragmentDensityBytes = 0;
		std::ostringstream swapchainStatsStream;
		for(VulkanFramebuffer *framebuffer : VulkanFramebuffer::GetLiveFramebuffers())
		{
			if(!framebuffer->_swapChain)
				continue;

			const uint32 swapchainIndex = swapchainFramebufferCount++;
			uint64 framebufferColorBytes = 0;
			uint64 framebufferFragmentDensityBytes = 0;
			for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
			{
				swapchainColorImageCount++;
				const uint64 targetBytes = estimateTextureBytes(targetView->targetView.texture->GetDescriptor());
				swapchainColorBytes += targetBytes;
				framebufferColorBytes += targetBytes;
			}

			for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_fragmentDensityTargets)
			{
				swapchainFragmentDensityImageCount++;
				const uint64 targetBytes = estimateTextureBytes(targetView->targetView.texture->GetDescriptor());
				swapchainFragmentDensityBytes += targetBytes;
				framebufferFragmentDensityBytes += targetBytes;
			}

			swapchainStatsStream << "\n    swapchain " << swapchainIndex << " |";
			if(!framebuffer->_colorTargets.empty())
			{
				const Texture::Descriptor &descriptor = framebuffer->_colorTargets[0]->targetView.texture->GetDescriptor();
				swapchainStatsStream << " colorImages=" << framebuffer->_colorTargets.size()
					<< " " << descriptor.width << "x" << descriptor.height
					<< " layers=" << VulkanTextureInfo::GetImageLayerCount(descriptor)
					<< " samples=" << static_cast<uint32>(descriptor.sampleCount)
					<< " vkFormat=" << static_cast<uint32>(VulkanTextureInfo::GetFormat(descriptor.format))
					<< " logical=" << bytesToMegabytes(framebufferColorBytes) << "MiB";
			}

			if(!framebuffer->_fragmentDensityTargets.empty())
			{
				const Texture::Descriptor &descriptor = framebuffer->_fragmentDensityTargets[0]->targetView.texture->GetDescriptor();
				swapchainStatsStream << " fragmentDensityImages=" << framebuffer->_fragmentDensityTargets.size()
					<< " " << descriptor.width << "x" << descriptor.height
					<< " layers=" << VulkanTextureInfo::GetImageLayerCount(descriptor)
					<< " samples=" << static_cast<uint32>(descriptor.sampleCount)
					<< " vkFormat=" << static_cast<uint32>(VulkanTextureInfo::GetFormat(descriptor.format))
					<< " logical=" << bytesToMegabytes(framebufferFragmentDensityBytes) << "MiB";
			}
		}

		if(swapchainFramebufferCount > 0)
		{
			statsStream << "\n  vulkan swapchains | framebuffers=" << swapchainFramebufferCount
				<< " colorImages=" << swapchainColorImageCount
				<< " logicalColor=" << bytesToMegabytes(swapchainColorBytes)
				<< "MiB fragmentDensityImages=" << swapchainFragmentDensityImageCount
				<< " logicalFragmentDensity=" << bytesToMegabytes(swapchainFragmentDensityBytes)
				<< "MiB"
				<< swapchainStatsStream.str();
		}

		for(uint32 heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; heapIndex++)
		{
			const VmaStatistics &heapStatistics = allocatorStatistics.memoryHeap[heapIndex].statistics;
			if(heapStatistics.blockBytes == 0 && heapStatistics.allocationBytes == 0 && heapBudgets[heapIndex].usage == 0)
				continue;

			statsStream << "\n    heap " << heapIndex
				<< " | used=" << bytesToMegabytes(heapStatistics.allocationBytes)
				<< "MiB reserved=" << bytesToMegabytes(heapStatistics.blockBytes)
				<< "MiB usage=" << bytesToMegabytes(heapBudgets[heapIndex].usage)
				<< "MiB budget=" << bytesToMegabytes(heapBudgets[heapIndex].budget)
				<< "MiB allocations=" << heapStatistics.allocationCount
				<< " blocks=" << heapStatistics.blockCount;
		}

		for(uint32 typeIndex = 0; typeIndex < memoryProperties.memoryTypeCount; typeIndex++)
		{
			const VmaStatistics &typeStatistics = allocatorStatistics.memoryType[typeIndex].statistics;
			if(typeStatistics.blockBytes == 0 && typeStatistics.allocationBytes == 0)
				continue;

			const VkMemoryPropertyFlags flags = memoryProperties.memoryTypes[typeIndex].propertyFlags;
			statsStream << "\n    type " << typeIndex
				<< " heap=" << memoryProperties.memoryTypes[typeIndex].heapIndex
				<< " used=" << bytesToMegabytes(typeStatistics.allocationBytes)
				<< "MiB reserved=" << bytesToMegabytes(typeStatistics.blockBytes)
				<< "MiB allocations=" << typeStatistics.allocationCount
				<< " blocks=" << typeStatistics.blockCount
				<< " flags=";

			bool hasFlag = false;
			auto appendFlag = [&statsStream, &hasFlag, flags](VkMemoryPropertyFlagBits flag, const char *name) {
				if(flags & flag)
				{
					if(hasFlag) statsStream << ",";
					statsStream << name;
					hasFlag = true;
				}
			};
			appendFlag(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "deviceLocal");
			appendFlag(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, "hostVisible");
			appendFlag(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "hostCoherent");
			appendFlag(VK_MEMORY_PROPERTY_HOST_CACHED_BIT, "hostCached");
			appendFlag(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, "lazy");
			if(!hasFlag) statsStream << "none";
		}

		return RNSTR(statsStream.str());
	}

	bool VulkanRenderer::ShouldInheritViews(RenderPass::ViewMode viewMode, bool isSubpass, bool hasInheritedViewState, bool destinationSupportsViewState) const
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

	bool VulkanRenderer::SupportsViewState(const VulkanFramebuffer *framebuffer, uint8 multiviewLayer, uint8 multiviewCount) const
	{
		if(multiviewLayer == 0 && multiviewCount == 0)
			return true;
		if(!framebuffer)
			return false;

		auto supportsTargetView = [multiviewLayer, multiviewCount](const VulkanFramebuffer::VulkanTargetView *targetView) {
			if(!targetView)
				return true;

			const uint32 requiredLayerCount = (multiviewCount > 0) ? multiviewCount : 1;
			const VkImageViewCreateInfo &descriptor = targetView->vulkanTargetViewDescriptor;
			switch(descriptor.viewType)
			{
				case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
				case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
				case VK_IMAGE_VIEW_TYPE_3D:
					return multiviewLayer + requiredLayerCount <= descriptor.subresourceRange.layerCount;

				default:
					return false;
			}
		};

		for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
		{
			if(!supportsTargetView(targetView))
				return false;
		}

		if(!supportsTargetView(framebuffer->_depthStencilTarget))
			return false;

		for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_fragmentDensityTargets)
		{
			if(!supportsTargetView(targetView))
				return false;
		}

		return !(framebuffer->_colorTargets.empty() && !framebuffer->_depthStencilTarget && framebuffer->_fragmentDensityTargets.empty());
	}

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
		_commandBufferResourcesPool(new Array()),
		_defaultPostProcessingDrawable(nullptr),
		_fallbackGlobalBuffer(nullptr),
		_fallbackGlobalTexture(nullptr),
		_activeFrameSubmission(nullptr),
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
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		RNVulkanValidate(vk::CreateCommandPool(device->GetDevice(), &cmdPoolInfo, nullptr, &_commandPool));

		//Create additional command pool for resource loading
		RNVulkanValidate(vk::CreateCommandPool(device->GetDevice(), &cmdPoolInfo, nullptr, &_commandPoolSynchronised));

		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneVulkan:/Shaders.json"));
		_dynamicBufferPool = new VulkanDynamicBufferPool();

		const size_t fallbackGlobalBufferLength = 16 * 1024;
		uint8 fallbackGlobalBufferData[fallbackGlobalBufferLength] = {};
		_fallbackGlobalBuffer = new VulkanStaticGPUBuffer(this, fallbackGlobalBufferData, fallbackGlobalBufferLength, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::WriteOnly);

		Texture::Descriptor fallbackGlobalTextureDescriptor = Texture::Descriptor::With2DTextureAndFormat(Texture::Format::RGBA_8, 1, 1, false);
		_fallbackGlobalTexture = CreateTextureWithDescriptor(fallbackGlobalTextureDescriptor);
		uint8 fallbackGlobalTextureData[4] = {};
		_fallbackGlobalTexture->SetData(0, fallbackGlobalTextureData, 4, 1);

		_internals->descriptorPool.Init(this);

		_internals->stateCoordinator.LoadPipelineCache(Kernel::GetSharedInstance()->GetApplication()->GetBuildNumber(), device, GetAllocatorCallback());

		#if defined(RN_PROFILE_TRACY)
			_internals->tracyCommandBuffer = GetCommandBuffer()->Retain();
			_internals->tracyVulkanCtx = RN_PROFILE_VULKAN_DECLARE_CONTEXT(device->GetInstance()->GetInstance(), device->GetDevice(), device->GetPhysicalDevice(), _workQueue, _internals->tracyCommandBuffer->GetCommandBuffer(), vk::GetInstanceProcAddr, vk::GetDeviceProcAddr);
		#else
			_internals->tracyCommandBuffer = nullptr;
			_internals->tracyVulkanCtx = nullptr;
		#endif

		StartRenderThread();
	}

	VulkanRenderer::~VulkanRenderer()
	{
		StopRenderThread();

		SafeRelease(_mainWindow);
		FlushAllDeletedDrawables();

		delete _defaultPostProcessingDrawable;

		SafeRelease(_fallbackGlobalTexture);
		SafeRelease(_fallbackGlobalBuffer);
		SafeRelease(_defaultShaderLibrary);

		auto drainFrameResources = [&]() {
			while(!_internals->frameResources.empty())
			{
				std::vector<VulkanFrameResource> frameResources;
				frameResources.swap(_internals->frameResources);

				for(VulkanFrameResource &frameResource : frameResources)
				{
					if(frameResource.finishedCallback)
					{
						frameResource.finishedCallback();
					}
				}
			}
		};

#if defined(RN_PROFILE_TRACY)
		if(_internals->tracyVulkanCtx)
		{
			RN_PROFILE_VULKAN_DESTROY_CONTEXT(_internals->tracyVulkanCtx);
			_internals->tracyVulkanCtx = nullptr;
		}
#endif
		SafeRelease(_internals->tracyCommandBuffer);

		_internals->stateCoordinator.DestroyPipelineCache(GetVulkanDevice(), GetAllocatorCallback());

		SafeRelease(_mipMapTextures);

		// Pending drawable callbacks release dynamic buffer references, so run them
		// before destroying the pool they point into.
		drainFrameResources();

		delete _dynamicBufferPool;
		_dynamicBufferPool = nullptr;

		// Pool destruction queues buffer frees; finish those before VMA allocator
		// teardown.
		drainFrameResources();

		vmaDestroyAllocator(_internals->memoryAllocator);
	}

	void VulkanRenderer::ResetDrawBindStateCache()
	{
		_internals->drawBindStateCache.pipeline = VK_NULL_HANDLE;
		_internals->drawBindStateCache.pipelineLayout = VK_NULL_HANDLE;
		_internals->drawBindStateCache.descriptorSet = VK_NULL_HANDLE;
		_internals->drawBindStateCache.vertexBufferCount = 0;
		for(uint8 i = 0; i < 3; i++)
		{
			_internals->drawBindStateCache.vertexBuffers[i] = VK_NULL_HANDLE;
			_internals->drawBindStateCache.vertexOffsets[i] = 0;
		}
		_internals->drawBindStateCache.hasIndexBufferBinding = false;
		_internals->drawBindStateCache.indexBuffer = VK_NULL_HANDLE;
		_internals->drawBindStateCache.indexOffset = 0;
		_internals->drawBindStateCache.indexType = VK_INDEX_TYPE_UINT16;
	}

	VulkanFrameSubmission &VulkanRenderer::GetActiveFrameSubmission()
	{
		AssertOnSubmissionThread();
		RN_ASSERT(_activeFrameSubmission, "No active Vulkan frame submission");
		return *_activeFrameSubmission;
	}

	VkRenderPass VulkanRenderer::GetVulkanRenderPass(const RenderFrame &renderFrame, const VulkanRenderPass *renderPass)
	{
		return _internals->stateCoordinator.GetRenderPassState(renderFrame, renderPass, renderPass->multiviewCount)->renderPass;
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
		RN_PROFILE_SCOPE();
		_currentResourcesCommandBufferLock.Lock();
		if(!_currentResourcesCommandBuffer)
		{
			VulkanCommandBuffer *commandBuffer = nullptr;
			for(int i = 0; i < _commandBufferResourcesPool->GetCount(); i++)
			{
				commandBuffer = static_cast<VulkanCommandBuffer*>(_commandBufferResourcesPool->GetObjectAtIndex(i));
				if(commandBuffer->_frameValue < _completedFrame && _completedFrame != -1) break;
			}

			if(!commandBuffer || commandBuffer->_frameValue >= _completedFrame || _completedFrame == -1)
			{
				commandBuffer = new VulkanCommandBuffer(GetVulkanDevice()->GetDevice(), _commandPoolSynchronised);

				VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandPool = _commandPoolSynchronised;
				commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				commandBufferAllocateInfo.commandBufferCount = 1;

				RNVulkanValidate(vk::AllocateCommandBuffers(GetVulkanDevice()->GetDevice(), &commandBufferAllocateInfo, &commandBuffer->_commandBuffer));

				_commandBufferResourcesPool->AddObject(commandBuffer);
				commandBuffer->Release();
			}
			else
			{
				commandBuffer->Reset();
			}

			_currentResourcesCommandBuffer = commandBuffer; //already retained at as part of the pool array, not doing any memory management on this!
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

	void VulkanRenderer::SubmitPendingResourceCommandBuffers()
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		_currentResourcesCommandBufferLock.Lock();
		VulkanCommandBuffer *resourcesCommandBuffer = _currentResourcesCommandBuffer;
		if(resourcesCommandBuffer)
		{
			resourcesCommandBuffer->End();
			resourcesCommandBuffer->_frameValue = _currentFrame;
		}
		_currentResourcesCommandBuffer = nullptr; //Always stays inside it's pool array, so just don't do any retain release
		_currentResourcesCommandBufferLock.Unlock();

		_lock.Lock();
		if(_submittedCommandBuffers->GetCount() > 0 || resourcesCommandBuffer)
		{
			std::vector<VkCommandBuffer> buffers;

			buffers.reserve(_submittedCommandBuffers->GetCount() + 1);
			if(resourcesCommandBuffer)
			{
				buffers.push_back(resourcesCommandBuffer->_commandBuffer);
			}
			_submittedCommandBuffers->Enumerate<VulkanCommandBuffer>([&](VulkanCommandBuffer *buffer, int i, bool &stop){
				buffer->_frameValue = _currentFrame;
				buffers.push_back(buffer->_commandBuffer);
				_executedCommandBuffers->AddObject(buffer);
			});

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = buffers.size();
			submitInfo.pCommandBuffers = buffers.data();

			{
				RN_PROFILE_ATRACE_SCOPE_N("VK QueueSubmit Resource Uploads");
				RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, VK_NULL_HANDLE));
			}

			//RNVulkanValidate(vk::DeviceWaitIdle(GetVulkanDevice()->GetDevice()));
			_submittedCommandBuffers->RemoveAllObjects();
		}
		_lock.Unlock();
	}

	Window *VulkanRenderer::CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor, void *hwnd)
	{
		VulkanWindow *window = new VulkanWindow(size, screen, this, descriptor, hwnd);

		if(!_mainWindow)
			_mainWindow = window->Retain();

		return window;
	}

	void VulkanRenderer::SetMainWindow(Window *window)
	{
		if(_mainWindow == window)
			return;

		SafeRelease(_mainWindow);
		_mainWindow = SafeRetain(window);
	}

	Window *VulkanRenderer::GetMainWindow()
	{
		return _mainWindow;
	}

	void VulkanRenderer::UpdateFrameFences()
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		//Check fence status
		int index = 0;
		int freeFenceIndex = -1;
		for(VkFence fence : _frameFences)
		{
			if(_frameFenceValues[index] != -1)
			{
				VkResult status = vk::GetFenceStatus(GetVulkanDevice()->GetDevice(), fence);
				if(status < 0)
				{
					RNVulkanValidate(status);
					index += 1;
					continue;
				}
				if(status == VK_SUCCESS)
				{
					const size_t completedFrameValue = _frameFenceValues[index];
					if(_completedFrame == static_cast<size_t>(-1) || completedFrameValue > _completedFrame)
					{
						_completedFrame = completedFrameValue;
					}
					ReleaseFrameResources(completedFrameValue);
					_internals->descriptorPool.ResetFramePool(this, index);
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
			_frameFenceValues.push_back(-1);

			freeFenceIndex = _frameFences.size() - 1;
		}

		_currentFrameFenceIndex = freeFenceIndex;
		_internals->descriptorPool.SetActiveFramePool(this, _currentFrameFenceIndex);
	}

	void VulkanRenderer::ReleaseFrameResources(uint32 frame)
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		//Delete command lists that finished execution on the graphics card (the command allocator needs to be alive the whole time)
		for(int i = _executedCommandBuffers->GetCount() - 1; i >= 0; i--)
		{
			VulkanCommandBuffer *commandBuffer = _executedCommandBuffers->GetObjectAtIndex<VulkanCommandBuffer>(i);
			if(commandBuffer->_frameValue <= frame) //Will be added to the executed list AFTER ReleaseFrameResources is called, so this check is fine
			{
				_commandBufferPool->AddObject(commandBuffer);
				_executedCommandBuffers->RemoveObjectAtIndex(i);
				commandBuffer->Reset();
			}
		}

		//Free other frame resources such as unused framebuffers and imageviews
		std::vector<std::function<void()>> finishedCallbacks;
		Lock();
		std::vector<VulkanFrameResource> &frameResources = _internals->frameResources;
		size_t retainedResourceCount = 0;
		for(size_t i = 0; i < frameResources.size(); i++)
		{
			VulkanFrameResource &frameResource = frameResources[i];
			if(frameResource.frame < frame) //Might be added to the frame resources just before this call, without finishing using them, so just using < here to keep around for one more frame
			{
				if(frameResource.finishedCallback)
				{
					finishedCallbacks.push_back(std::move(frameResource.finishedCallback));
				}
			}
			else
			{
				if(retainedResourceCount != i)
				{
					frameResources[retainedResourceCount] = std::move(frameResource);
				}
				retainedResourceCount++;
			}
		}
		frameResources.erase(frameResources.begin() + retainedResourceCount, frameResources.end());
		Unlock();

		for(auto iterator = finishedCallbacks.rbegin(); iterator != finishedCallbacks.rend(); iterator++)
		{
			(*iterator)();
		}
	}

	void VulkanRenderer::Render(Function &&function)
	{
		RN_PROFILE_SCOPE();

		QueueFrameSubmission(std::move(function));
	}

	void VulkanRenderer::StartRenderThread()
	{
		_internals->renderThread = new Thread([this]() {
#if RN_PLATFORM_ANDROID
			// Keep the render thread attached so presentation calls do not attach and detach every frame.
			const AndroidState *androidState = Kernel::GetSharedInstance()->GetAndroidState();
			JavaVM *javaVM = androidState? androidState->GetJavaVM() : nullptr;
			bool didAttachCurrentThread = false;
			ScopeGuard javaThreadGuard([javaVM, &didAttachCurrentThread]() {
				if(didAttachCurrentThread)
					javaVM->DetachCurrentThread();
			});

			if(javaVM)
			{
				JNIEnv *env = nullptr;
				if(javaVM->GetEnv(reinterpret_cast<void **>(&env), RN_JNI_VERSION_1_6) == JNI_EDETACHED)
				{
					char threadName[] = "RN::VulkanRender";
					JavaVMAttachArgs attachArgs = {RN_JNI_VERSION_1_6, threadName, nullptr};
					didAttachCurrentThread = (javaVM->AttachCurrentThread(&env, &attachArgs) == JNI_OK);
					if(didAttachCurrentThread)
						Thread::SetCurrentThreadName(threadName);
				}
			}
#endif

			while(true)
			{
				AutoreleasePool pool;
				if(!ConsumeRenderThreadWork())
					return;
			}
		}, false);
		_internals->renderThread->SetName(RNCSTR("RN::VulkanRender"));
		_internals->renderThread->Start();
		while(!_internals->renderThread->IsRunning())
			std::this_thread::yield();
	}

	void VulkanRenderer::StopRenderThread()
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

	void VulkanRenderer::AssertOnSubmissionThread()
	{
#if RN_BUILD_DEBUG
		std::thread::id currentThread = std::this_thread::get_id();
		if(!_internals->hasSubmissionThread)
		{
			_internals->submissionThread = currentThread;
			_internals->hasSubmissionThread = true;
		}

		RN_DEBUG_ASSERT(_internals->submissionThread == currentThread, "Vulkan frame submission must stay on one submission thread");
		RN_DEBUG_ASSERT(!_internals->renderThread || !_internals->renderThread->OnThread(), "Vulkan frame submission must not run on the render thread");
#endif
	}

	void VulkanRenderer::AssertOnRenderThread() const
	{
#if RN_BUILD_DEBUG
		RN_DEBUG_ASSERT(IsOnRenderThread(), "Vulkan render work must run on the render thread");
#endif
	}

	bool VulkanRenderer::IsOnRenderThread() const
	{
		return _internals->renderThread && _internals->renderThread->OnThread();
	}

	void VulkanRenderer::ScheduleRenderThreadWork(Function &&function)
	{
		if(IsOnRenderThread())
		{
			function();
			return;
		}

		_internals->renderThreadQueue.PushTask(std::move(function));
	}

	void VulkanRenderer::SynchronizeRenderThread()
	{
		if(IsOnRenderThread())
			return;

		_internals->renderThreadQueue.Synchronize();
	}

	void VulkanRenderer::QueueFrameSubmission(Function &&function)
	{
		RN_PROFILE_SCOPE();
		AssertOnSubmissionThread();

		VulkanFrameSubmission submission;
		if(!_internals->renderThreadQueue.WaitForSpace())
			return;

		BuildFrameSubmission(submission, std::move(function));
		_internals->renderThreadQueue.Push(std::move(submission));
	}

	bool VulkanRenderer::ConsumeRenderThreadWork()
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();

		VulkanFrameSubmission submission;
		Function task;
		using WorkType = RenderThreadQueue<VulkanFrameSubmission>::WorkType;
		WorkType workType = _internals->renderThreadQueue.Pop(submission, task);
		if(workType == WorkType::None)
			return false;

		if(workType == WorkType::Task)
		{
			task();
			return true;
		}

		RN_PROFILE_ATRACE_SCOPE_N("RN RenderThread Execute Render Frame");
		if(!submission.renderFrame.BeginPresentationStatesOnRenderThread())
		{
			FinishRenderFrameSubmission(submission.renderFrame);
			return true;
		}

		if(PrepareRenderFrame(submission))
		{
			RenderFrameSubmission(submission);
			PrintFrameStatistics(submission.renderFrame);
		}
		else
		{
			submission.renderFrame.CancelPresentationStatesOnRenderThread();
		}

		FinishRenderFrameSubmission(submission.renderFrame);
		return true;
	}

	void VulkanRenderer::BuildFrameSubmission(VulkanFrameSubmission &submission, Function &&function)
	{
		RN_PROFILE_SCOPE();
		RN_PROFILE_ATRACE_SCOPE_N("RN MainThread Build Render Frame");
		AssertOnSubmissionThread();

		//SubmitCamera is called for each camera and creates draw items per camera
		BeginRenderFrameSubmission(submission.renderFrame);
		RenderFrame *previousRenderFrame = SetActiveRenderFrame(&submission.renderFrame);
		ScopeGuard activeRenderFrameGuard([this, previousRenderFrame]() {
			SetActiveRenderFrame(previousRenderFrame);
		});
		VulkanFrameSubmission *previousSubmission = _activeFrameSubmission;
		_activeFrameSubmission = &submission;
		function();
		_activeFrameSubmission = previousSubmission;
		submission.PruneSkippedRenderPasses();
	}

	void VulkanRenderer::RenderFrameSubmission(const VulkanFrameSubmission &submission)
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();

		for(VulkanSwapChain *swapChain : submission.swapChains)
		{
			swapChain->AcquireBackBuffer();
		}

		_currentCommandBuffer = GetCommandBuffer();
		_currentCommandBuffer->Retain();
		_currentCommandBuffer->Begin();
		ResetDrawBindStateCache();

		if(submission.swapChains.size() > 0 || submission.renderPasses.size() > 0)
		{
			VkCommandBuffer commandBuffer = _currentCommandBuffer->GetCommandBuffer();
			RN_PROFILE_VULKAN_SCOPE_CMD(_internals->tracyVulkanCtx, commandBuffer);

			for(VulkanSwapChain *swapChain : submission.swapChains)
			{
				swapChain->Prepare(commandBuffer);
			}

			for(const VulkanRenderPass &renderPass : submission.renderPasses)
			{
				if(renderPass.type == VulkanRenderPass::Type::Compute)
				{
					RenderComputePass(_currentCommandBuffer, submission, renderPass);
					continue;
				}

				if(!renderPass.UsesDrawItems())
				{
					RenderAPIRenderPass(_currentCommandBuffer, renderPass);
					continue;
				}

				//Set textures layout for reading for render targets that are used in this frame
				for(VulkanTexture *vulkanTexture : renderPass.shaderWriteTexturesUsedAsSampledImages)
				{
					vulkanTexture->TransitionToUsage(commandBuffer, VulkanTexture::LayoutUsage::ShaderRead);
				}

				for(VulkanTexture *vulkanTexture : renderPass.renderTargetsUsedInShader)
				{
					vulkanTexture->TransitionToUsage(commandBuffer, VulkanTexture::LayoutUsage::ShaderRead);
				}

				//Set previous framebuffer texture layout for reading
				if(renderPass.previousStoredFramebuffer)
				{
					Texture *texture = renderPass.previousStoredFramebuffer->GetColorTexture(0);
					if(texture)
					{
						VulkanTexture *vulkanTexture = texture->Downcast<VulkanTexture>();
						vulkanTexture->TransitionToUsage(commandBuffer, VulkanTexture::LayoutUsage::ShaderRead);
					}
				}

				if(renderPass.subpasses.size() > 0)
				{
					const RenderPass::DrawSnapshot &drawSnapshot = submission.renderFrame.GetPass(renderPass.renderFramePassIndex).GetDrawSnapshot();
					const RenderPass::SubpassSnapshot &subpass = drawSnapshot.GetSubpass();
					//Determine first/last usage of each color attachment across subpasses to choose appropriate initial/final layouts
					uint32 numColorAttachments = renderPass.framebuffer->GetColorTargetCount();

					for(uint32 ci = 0; ci < numColorAttachments; ++ci)
					{
						Texture *t = renderPass.framebuffer->GetColorTexture(ci);
						if(!t) continue;

						VulkanTexture *vulkanTexture = t->Downcast<VulkanTexture>();
						const auto colorAttachment = subpass.GetColorAttachment(ci);
						vulkanTexture->TransitionToUsage(commandBuffer, colorAttachment.GetFirstUseIsRead() ? VulkanTexture::LayoutUsage::ShaderRead : VulkanTexture::LayoutUsage::RenderTarget);
					}

					if(renderPass.framebuffer->GetDepthStencilTexture())
					{
						Texture *t = renderPass.framebuffer->GetDepthStencilTexture();
						if(t)
						{
							bool depthFirstIsReadOnly = subpass.GetDepthFirstUseIsRead();

							VulkanTexture *vulkanTexture = t->Downcast<VulkanTexture>();
							vulkanTexture->TransitionToUsage(commandBuffer, depthFirstIsReadOnly ? VulkanTexture::LayoutUsage::ShaderRead : VulkanTexture::LayoutUsage::RenderTarget);
						}
					}

					SetupRendertargets(commandBuffer, submission, renderPass);

					uint32 counter = 0;
					for(const VulkanRenderPass &subpass : renderPass.subpasses)
					{
						//TODO: Sort drawables by camera and root signature? Maybe not...
						//Draw drawables
						RN_DEBUG_ASSERT(subpass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
						const VulkanPreparedRenderPass &preparedPass = submission.preparedRenderPasses[subpass.preparedRenderPassIndex];
						const std::vector<VulkanPreparedDrawItem> &drawItems = preparedPass.drawItems;
						uint32 stepSize = 0;
						uint32 stepSizeIndex = 0;
						for(size_t i = 0; i < drawItems.size(); i += stepSize)
						{
							stepSize = preparedPass.instanceSteps[stepSizeIndex++];
							RenderDrawable(commandBuffer, drawItems[i], stepSize);
						}

						//RNDebug("draw calls: " << preparedPass.instanceSteps.size());

						counter++;

						if(counter < renderPass.subpasses.size())
						{
							vk::CmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
						}
					}
				}
				else
				{
					RN_DEBUG_ASSERT(renderPass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
					const VulkanPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderPass.preparedRenderPassIndex];
					const std::vector<VulkanPreparedDrawItem> &drawItems = preparedPass.drawItems;
					if(!drawItems.empty())
					{
						SetupRendertargets(commandBuffer, submission, renderPass);

						//TODO: Sort drawables by camera and root signature? Maybe not...
						//Draw drawables
						uint32 stepSize = 0;
						uint32 stepSizeIndex = 0;
						for(size_t i = 0; i < drawItems.size(); i += stepSize)
						{
							stepSize = preparedPass.instanceSteps[stepSizeIndex++];
							RenderDrawable(commandBuffer, drawItems[i], stepSize);
						}

						//RNDebug("draw calls: " << preparedPass.instanceSteps.size());
					}
				}

				RN_DEBUG_ASSERT(renderPass.subpasses.size() > 0 || renderPass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
				const bool didRecordRenderPass = renderPass.subpasses.size() > 0 || !submission.preparedRenderPasses[renderPass.preparedRenderPassIndex].drawItems.empty();
				if(didRecordRenderPass)
				{
					vk::CmdEndRenderPass(commandBuffer);
				}

				// Update tracked layouts for attachments to match final layouts of this render pass
				if(didRecordRenderPass)
				{
					VulkanFramebuffer *fb = renderPass.framebuffer;
					if(fb)
					{
						const RenderPass::DrawSnapshot &drawSnapshot = submission.renderFrame.GetPass(renderPass.renderFramePassIndex).GetDrawSnapshot();
						const RenderPass::SubpassSnapshot &subpass = drawSnapshot.GetSubpass();
						uint32 numColorAttachments = fb->GetColorTargetCount();

						for(uint32 ci = 0; ci < numColorAttachments; ++ci)
						{
							Texture *t = fb->GetColorTexture(ci);
							if(!t) continue;
							VulkanTexture *vt = t->Downcast<VulkanTexture>();
							vt->AdoptLayoutUsage(subpass.GetColorAttachment(ci).GetLastUseIsRead() ? VulkanTexture::LayoutUsage::ShaderRead : VulkanTexture::LayoutUsage::RenderTarget);
						}

						// Depth-stencil
						Texture *dt = fb->GetDepthStencilTexture();
						if(dt)
						{
							bool depthLastIsReadOnly = subpass.GetDepthLastUseIsRead();

							VulkanTexture *dvt = dt->Downcast<VulkanTexture>();
							dvt->AdoptLayoutUsage(depthLastIsReadOnly ? VulkanTexture::LayoutUsage::ShaderRead : VulkanTexture::LayoutUsage::RenderTarget);
						}
					}
				}

				//Set textures layout for writing for render targets that are used in this frame
				for(VulkanTexture *vulkanTexture : renderPass.renderTargetsUsedInShader)
				{
					vulkanTexture->TransitionToUsage(commandBuffer, VulkanTexture::LayoutUsage::RenderTarget);
				}

				//Set previous framebuffer texture layout for writing
				if(renderPass.previousStoredFramebuffer)
				{
					Texture *texture = renderPass.previousStoredFramebuffer->GetColorTexture(0);
					if(texture)
					{
						VulkanTexture *vulkanTexture = texture->Downcast<VulkanTexture>();
						vulkanTexture->TransitionToUsage(commandBuffer, VulkanTexture::LayoutUsage::RenderTarget);
					}
				}

			}

			for(VulkanSwapChain *swapChain : submission.swapChains)
			{
				swapChain->Finalize(commandBuffer);
			}
		}

		_dynamicBufferPool->FlushAllBuffers();

		//Prepare command buffer submission
		std::vector<VkSemaphore> presentSemaphores;
		std::vector<VkPipelineStageFlags> presentSemaphoresWaitStages;
		std::vector<VkSemaphore> renderSemaphores;

		for(VulkanSwapChain *swapChain : submission.swapChains)
		{
			VkSemaphore presentSemaphore = swapChain->GetCurrentPresentSemaphore();
			VkSemaphore renderSemaphore = swapChain->GetCurrentRenderSemaphore();
			if(presentSemaphore != VK_NULL_HANDLE)
			{
				presentSemaphores.push_back(presentSemaphore);
				presentSemaphoresWaitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			}
			if(renderSemaphore != VK_NULL_HANDLE) renderSemaphores.push_back(renderSemaphore);
		}

		RN_PROFILE_VULKAN_COLLECT(_internals->tracyVulkanCtx, _currentCommandBuffer->GetCommandBuffer());

		_currentCommandBuffer->End();
		SubmitCommandBuffer(_currentCommandBuffer);
		_currentCommandBuffer->Release();
		_currentCommandBuffer = nullptr;

		std::vector<VkCommandBuffer> buffers;
		_lock.Lock();
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

		submitInfo.pWaitDstStageMask = presentSemaphoresWaitStages.data();

		_frameFenceValues[_currentFrameFenceIndex] = _currentFrame;
		{
			RN_PROFILE_ATRACE_SCOPE_N("VK QueueSubmit Frame");
			RNVulkanValidate(vk::QueueSubmit(_workQueue, 1, &submitInfo, _frameFences[_currentFrameFenceIndex]));
		}

		for(VulkanSwapChain *swapChain : submission.swapChains)
		{
			swapChain->PresentBackBuffer(_workQueue);
		}

		submission.renderFrame.EndPresentationStatesOnRenderThread();

		RN_PROFILE_FRAME_TRACY();

		_currentFrame ++;
	}

	void VulkanRenderer::SetupRendertargets(VkCommandBuffer commandBuffer, const VulkanFrameSubmission &submission, const VulkanRenderPass &renderPass)
	{
		RN_PROFILE_SCOPE();
		RN_PROFILE_VULKAN_SCOPE_CMD_N(_internals->tracyVulkanCtx, commandBuffer, "SetupRendertargets");
		const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
		const RenderPass::DrawSnapshot &drawSnapshot = framePass.GetDrawSnapshot();

		//TODO: Call PrepareAsRendertargetForFrame() only once per framebuffer per frame, find new solution for setting things up for msaa while reusing a framebuffer?
		{
			RN_PROFILE_VULKAN_SCOPE_CMD_N(_internals->tracyVulkanCtx, commandBuffer, "PrepareRendertargetForFrame");
			renderPass.framebuffer->PrepareAsRendertargetForFrame(submission.renderFrame, &renderPass);
		}
		{
			RN_PROFILE_VULKAN_SCOPE_CMD_N(_internals->tracyVulkanCtx, commandBuffer, "SetAsRendertarget");
			renderPass.framebuffer->SetAsRendertarget(commandBuffer, renderPass.resolveFramebuffer, framePass.GetDrawSnapshot());
		}
		ResetDrawBindStateCache();

		// Follow-up passes render into their own target frame, not the previous camera frame.
		Rect cameraRect = drawSnapshot.IsSubpass() ? framePass.GetCameraSnapshot().GetFrame() : drawSnapshot.GetFrame();

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
		SubmitCamera(GetActiveFrameSubmission(), camera, std::move(function));
	}

	void VulkanRenderer::SubmitCamera(VulkanFrameSubmission &frameSubmission, Camera *camera, Function &&function)
	{
		RN_PROFILE_SCOPE();
		std::vector<Camera *> multiviewSnapshotCameras;

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

						multiviewSnapshotCameras.push_back(multiviewCamera);
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

						RN::Function cameraSubmission = RN::MakeFunction([&function](){ function(); });
						if(increment == 1)
						{
							_currentMultiviewFallbackRenderPass = camera->GetRenderPass();
							SubmitCamera(frameSubmission, multiviewCameras->GetObjectAtIndex<Camera>(i), std::move(cameraSubmission));
							_currentMultiviewFallbackRenderPass = nullptr;
						}
						else
						{
							SubmitCamera(frameSubmission, camera, std::move(cameraSubmission));
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

					RN::Function cameraSubmission = RN::MakeFunction([&function](){ function(); });
					SubmitCamera(frameSubmission, multiviewCamera, std::move(cameraSubmission));

					_currentMultiviewLayer = 0;
					_currentMultiviewCount = 0;
					_currentMultiviewFallbackRenderPass = nullptr;
				});

				return;
			}
		}

		size_t previousRenderPassIndex = frameSubmission.renderPasses.size();
		frameSubmission.activeRenderPassIndex = previousRenderPassIndex;

		FramePass *cameraFramePass = _currentMultiviewFallbackRenderPass ? static_cast<FramePass *>(_currentMultiviewFallbackRenderPass) : camera->GetRootFramePass();
		SubmitRootFramePass(frameSubmission, camera, cameraFramePass, multiviewSnapshotCameras);

		frameSubmission.activeRenderPassIndex = previousRenderPassIndex;

		for(size_t i = previousRenderPassIndex; i < frameSubmission.renderPasses.size(); i++)
		{
			VulkanRenderPass &submittedRenderPass = frameSubmission.renderPasses[i];
			submittedRenderPass.subpassSignature = 0;
			if(submittedRenderPass.subpasses.size() > 0)
			{
				uint32 colorAttachmentCount = submittedRenderPass.framebuffer->GetColorTargetCount();
				bool hasDepth = (submittedRenderPass.framebuffer->_depthStencilTarget != nullptr);

				uint64 signature = 0;
				for(size_t si = 0; si < submittedRenderPass.subpasses.size(); si++)
				{
					const RenderPass::SubpassSnapshot &subpassSnapshot = frameSubmission.renderFrame.GetPass(submittedRenderPass.subpasses[si].renderFramePassIndex).GetDrawSnapshot().GetSubpass();
					for(uint32 ci = 0; ci < colorAttachmentCount; ci++)
					{
						const auto colorAttachment = subpassSnapshot.GetColorAttachment(ci);
						if(colorAttachment.GetWrites()) signature ^= (0x9e3779b97f4a7c15ull + ((static_cast<uint64>(si) << 32) ^ ci));
						if(colorAttachment.GetReads()) signature ^= (0x85ebca6b + ((static_cast<uint64>(si) << 33) ^ ci));
					}
					if(hasDepth)
					{
						if(subpassSnapshot.GetWritesDepthStencil()) signature ^= (0x27d4eb2f + (static_cast<uint64>(si) << 1));
						else if(subpassSnapshot.GetReadsDepthStencil()) signature ^= (0x165667b1 + (static_cast<uint64>(si) << 1));
					}
				}
				submittedRenderPass.subpassSignature = signature ^ (static_cast<uint64>(submittedRenderPass.subpasses.size()) * 0x9e3779b97f4a7c15ull);
			}
		}

		const size_t submittedRenderPassEndIndex = frameSubmission.renderPasses.size();

		// Run once to submit all scene nodes; SubmitDrawable will route to all matching passes
		BeginCameraPassAttachmentSnapshots();
		ScopeGuard cameraPassAttachmentSnapshotsGuard([this]() {
			FinishCameraPassAttachmentSnapshots();
		});
		{
			_lock.Lock();
			ScopeGuard submissionLockGuard([this]() {
				_lock.Unlock();
			});
			function();
		}

		auto addCameraPassAttachmentSnapshots = [&](VulkanRenderPass &submittedRenderPass) {
			if(!submittedRenderPass.UsesDrawItems())
				return;

			AddCameraPassAttachmentSnapshots(submittedRenderPass.renderFramePassIndex);
		};

		for(size_t i = previousRenderPassIndex; i < submittedRenderPassEndIndex; i++)
		{
			VulkanRenderPass &submittedRenderPass = frameSubmission.renderPasses[i];
			addCameraPassAttachmentSnapshots(submittedRenderPass);
			for(VulkanRenderPass &subpass : submittedRenderPass.subpasses)
			{
				addCameraPassAttachmentSnapshots(subpass);
			}
		}
	}

	size_t VulkanRenderer::SubmitRootRenderPass(VulkanFrameSubmission &frameSubmission, Camera *camera, RenderPass *cameraRenderPass, const std::vector<Camera *> &multiviewSnapshotCameras)
	{
		RN_PROFILE_SCOPE();

		cameraRenderPass->UpdateSubpassChain();
		const size_t frameStatisticsIndex = frameSubmission.renderFrame.AddCameraStatistics();

		RenderPassResources *renderPassResources = cameraRenderPass->GetRenderResources(this);
		const RenderPass::DrawSnapshot &rootDrawSnapshot = renderPassResources->GetDrawSnapshot();
		Framebuffer *framebuffer = rootDrawSnapshot.GetFramebuffer();
		if(!framebuffer) return RenderFrame::InvalidPassIndex;

		VulkanRenderPass renderPass;
		renderPass.previousStoredFramebuffer = nullptr;
		renderPass.multiviewLayer = _currentMultiviewLayer;
		renderPass.type = VulkanRenderPass::Type::Default;
		renderPass.renderPass = cameraRenderPass;
		renderPass.frameStatisticsIndex = frameStatisticsIndex;
		renderPass.resolveFramebuffer = nullptr;
		renderPass.shaderHint = rootDrawSnapshot.GetShaderHint();
		renderPass.multiviewCount = (_currentMultiviewCount > 0) ? _currentMultiviewCount : static_cast<uint8>(multiviewSnapshotCameras.size());
		renderPass.renderFramePassIndex = frameSubmission.renderFrame.AddPass(rootDrawSnapshot, renderPassResources->GetOverrideMaterialSnapshot(), renderPassResources->GetIdentity(), renderPassResources->GetOverrideMaterialSnapshotVersion());

		Matrix clipSpaceCorrectionMatrix;
		clipSpaceCorrectionMatrix.m[5] = -1.0f;
		RenderFrame::CameraSnapshot cameraSnapshot = RenderFrame::CameraSnapshot::WithCamera(camera, rootDrawSnapshot.GetFrame(), clipSpaceCorrectionMatrix);
		RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(renderPass.renderFramePassIndex);
		framePass.SetCameraSnapshot(cameraSnapshot);
		for(Camera *multiviewCamera : multiviewSnapshotCameras)
		{
			framePass.AddMultiviewCameraSnapshot(RenderFrame::CameraSnapshot::WithCamera(multiviewCamera, rootDrawSnapshot.GetFrame(), clipSpaceCorrectionMatrix));
		}

		renderPass.framebuffer = framebuffer->Downcast<VulkanFramebuffer>();
		frameSubmission.AddSwapChain(renderPass.framebuffer->GetSwapChain());

		const size_t renderPassIndex = frameSubmission.renderPasses.size();
		frameSubmission.activeRenderPassIndex = renderPassIndex;
		renderPass.subpasses.reserve(cameraRenderPass->GetSubpassCount());
		frameSubmission.renderPasses.push_back(renderPass);
		return renderPassIndex;
	}

	void VulkanRenderer::SubmitRootFramePass(VulkanFrameSubmission &frameSubmission, Camera *camera, FramePass *framePass, const std::vector<Camera *> &multiviewSnapshotCameras)
	{
		RN_PROFILE_SCOPE();

		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		if(renderPass)
		{
			const size_t rootRenderPassIndex = SubmitRootRenderPass(frameSubmission, camera, renderPass, multiviewSnapshotCameras);
			if(rootRenderPassIndex == RenderFrame::InvalidPassIndex) return;

			renderPass->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				SubmitFramePass(frameSubmission, camera, nextPass, frameSubmission.renderPasses[rootRenderPassIndex], &multiviewSnapshotCameras);
			});
			return;
		}

		ComputePass *computePass = framePass->Downcast<ComputePass>();
		if(computePass)
		{
			SubmitComputePass(frameSubmission, computePass, nullptr, camera, &multiviewSnapshotCameras);
			computePass->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				SubmitRootFramePass(frameSubmission, camera, nextPass, multiviewSnapshotCameras);
			});
			return;
		}

		RN_ASSERT(false, "Vulkan renderer only supports RenderPass and ComputePass frame pass nodes");
	}

	void VulkanRenderer::SubmitFramePass(VulkanFrameSubmission &frameSubmission, Camera *camera, FramePass *framePass, VulkanRenderPass &previousRenderPass, const std::vector<Camera *> *multiviewSnapshotCameras)
	{
		RN_PROFILE_SCOPE();

		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		if(renderPass)
		{
			SubmitRenderPass(frameSubmission, camera, renderPass, previousRenderPass, multiviewSnapshotCameras);
			return;
		}

		ComputePass *computePass = framePass->Downcast<ComputePass>();
		if(computePass)
		{
			SubmitComputePass(frameSubmission, computePass, &previousRenderPass, camera, multiviewSnapshotCameras);
			computePass->GetNextFramePasses()->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
				SubmitFramePass(frameSubmission, camera, nextPass, previousRenderPass, multiviewSnapshotCameras);
			});
			return;
		}

		RN_ASSERT(false, "Vulkan renderer only supports RenderPass and ComputePass frame pass nodes");
	}

	void VulkanRenderer::SubmitComputePass(VulkanFrameSubmission &frameSubmission, ComputePass *computePass, VulkanRenderPass *previousRenderPass, Camera *camera, const std::vector<Camera *> *multiviewSnapshotCameras)
	{
		RN_PROFILE_SCOPE();

		VulkanRenderPass vulkanComputePass;
		vulkanComputePass.type = VulkanRenderPass::Type::Compute;
		vulkanComputePass.renderPass = nullptr;
		vulkanComputePass.computePass = computePass;
		vulkanComputePass.previousStoredFramebuffer = previousRenderPass ? (previousRenderPass->resolveFramebuffer ? previousRenderPass->resolveFramebuffer : previousRenderPass->framebuffer) : nullptr;
		vulkanComputePass.framebuffer = nullptr;
		vulkanComputePass.resolveFramebuffer = nullptr;
		computePass->GetDispatchSnapshot(vulkanComputePass.computeDispatch);
		if(previousRenderPass && previousRenderPass->renderFramePassIndex != RenderFrame::InvalidPassIndex)
		{
			const RenderFrame::Pass &previousFramePass = frameSubmission.renderFrame.GetPass(previousRenderPass->renderFramePassIndex);
			vulkanComputePass.computeCameraSnapshot = previousFramePass.GetCameraSnapshot();
			vulkanComputePass.computeMultiviewCameraSnapshots = previousFramePass.GetMultiviewCameraSnapshots();
		}
		else if(camera)
		{
			RenderPassResources *renderPassResources = camera->GetRenderPass()->GetRenderResources(this);
			const RenderPass::DrawSnapshot &drawSnapshot = renderPassResources->GetDrawSnapshot();
			Matrix clipSpaceCorrectionMatrix;
			clipSpaceCorrectionMatrix.m[5] = -1.0f;
			vulkanComputePass.computeCameraSnapshot = RenderFrame::CameraSnapshot::WithCamera(camera, drawSnapshot.GetFrame(), clipSpaceCorrectionMatrix);
			if(multiviewSnapshotCameras)
			{
				for(Camera *multiviewCamera : *multiviewSnapshotCameras)
				{
					vulkanComputePass.computeMultiviewCameraSnapshots.push_back(RenderFrame::CameraSnapshot::WithCamera(multiviewCamera, drawSnapshot.GetFrame(), clipSpaceCorrectionMatrix));
				}
			}
		}

		frameSubmission.renderPasses.push_back(vulkanComputePass);
	}

	void VulkanRenderer::SubmitRenderPass(VulkanFrameSubmission &frameSubmission, Camera *camera, RenderPass *renderPass, VulkanRenderPass &previousRenderPass, const std::vector<Camera *> *multiviewSnapshotCameras)
	{
		RN_PROFILE_SCOPE();

		renderPass->UpdateSubpassChain();
		const Array *nextFramePasses = renderPass->GetNextFramePasses();

		PostProcessingAPIStage *apiStage = renderPass->Downcast<PostProcessingAPIStage>();
		bool isPostProcessingStage = renderPass->IsKindOfClass(PostProcessingStage::GetMetaClass());
		RenderPassResources *renderPassResources = renderPass->GetRenderResources(this);
		const RenderPass::DrawSnapshot &drawSnapshot = renderPassResources->GetDrawSnapshot();

		VulkanRenderPass vulkanRenderPass;

		vulkanRenderPass.type = VulkanRenderPass::Type::Default;
		if(apiStage)
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
					break;
				}
			}
		}

		vulkanRenderPass.renderPass = renderPass;
		vulkanRenderPass.frameStatisticsIndex = previousRenderPass.frameStatisticsIndex;
		vulkanRenderPass.previousStoredFramebuffer = nullptr;

		vulkanRenderPass.framebuffer = nullptr;
		vulkanRenderPass.resolveFramebuffer = nullptr;

		vulkanRenderPass.shaderHint = drawSnapshot.GetShaderHint();

		if(previousRenderPass.renderPass)
		{
			vulkanRenderPass.previousStoredFramebuffer = previousRenderPass.resolveFramebuffer ? previousRenderPass.resolveFramebuffer : previousRenderPass.framebuffer;
		}

		if(!drawSnapshot.IsSubpass())
		{
			Framebuffer *framebuffer = drawSnapshot.GetFramebuffer();
			vulkanRenderPass.framebuffer = framebuffer->Downcast<VulkanFramebuffer>();
			frameSubmission.AddSwapChain(vulkanRenderPass.framebuffer->GetSwapChain());
		}

		const RenderFrame::Pass &previousFramePass = frameSubmission.renderFrame.GetPass(previousRenderPass.renderFramePassIndex);
		const uint8 inheritedMultiviewLayer = previousRenderPass.multiviewLayer;
		const uint8 inheritedMultiviewCount = previousRenderPass.multiviewCount;
		const bool hasInheritedViewState = inheritedMultiviewLayer > 0 || inheritedMultiviewCount > 0;
		const bool destinationSupportsViewState = drawSnapshot.IsSubpass() ? false : SupportsViewState(vulkanRenderPass.framebuffer, inheritedMultiviewLayer, inheritedMultiviewCount);
		const bool shouldInheritViews = ShouldInheritViews(renderPass->GetViewMode(), drawSnapshot.IsSubpass(), hasInheritedViewState, destinationSupportsViewState);

		vulkanRenderPass.multiviewLayer = shouldInheritViews ? inheritedMultiviewLayer : 0;
		vulkanRenderPass.multiviewCount = shouldInheritViews ? inheritedMultiviewCount : 0;

		if(vulkanRenderPass.type != VulkanRenderPass::Type::ResolveMSAA)
		{
			vulkanRenderPass.renderFramePassIndex = frameSubmission.renderFrame.AddPass(drawSnapshot, renderPassResources->GetOverrideMaterialSnapshot(), renderPassResources->GetIdentity(), renderPassResources->GetOverrideMaterialSnapshotVersion());
			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(vulkanRenderPass.renderFramePassIndex);
			framePass.SetCameraSnapshot(previousFramePass.GetCameraSnapshot());
			if(shouldInheritViews)
			{
				for(const RenderFrame::CameraSnapshot &multiviewCameraSnapshot : previousFramePass.GetMultiviewCameraSnapshots())
				{
					framePass.AddMultiviewCameraSnapshot(multiviewCameraSnapshot);
				}
			}
			if(drawSnapshot.IsSubpass())
			{
				previousRenderPass.subpasses.push_back(vulkanRenderPass);
			}
			else
			{
				frameSubmission.activeRenderPassIndex = frameSubmission.renderPasses.size();
				vulkanRenderPass.subpasses.reserve(renderPass->GetSubpassCount());
				frameSubmission.renderPasses.push_back(vulkanRenderPass);

				// Inject a default fullscreen quad for post processing passes so we don't redraw the whole scene.
				if(isPostProcessingStage || vulkanRenderPass.type == VulkanRenderPass::Type::Convert)
				{
					if(!_defaultPostProcessingDrawable)
					{
						Mesh *planeMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f + 180.0f, 0.0f)), Vector3(0.0f), Vector2(1.0f, 1.0f));
						Material *planeMaterial = Material::WithShaders(GetDefaultShader(Shader::Type::Vertex, nullptr), GetDefaultShader(Shader::Type::Fragment, nullptr));
						planeMaterial->SetDepthWriteEnabled(false);
						planeMaterial->SetDepthMode(DepthMode::Always);
						planeMaterial->SetOverride(Material::Override::DepthWrite | Material::Override::GroupDepth);

						_lock.Lock();
						_defaultPostProcessingDrawable = static_cast<VulkanDrawable*>(CreateDrawable());
						_defaultPostProcessingDrawable->SetSources(planeMesh, planeMaterial, nullptr);
						_lock.Unlock();
					}

					SubmitDrawable(frameSubmission, _defaultPostProcessingDrawable, nullptr);
				}
			}

		}
		else
		{
			frameSubmission.renderPasses[frameSubmission.activeRenderPassIndex].resolveFramebuffer = vulkanRenderPass.framebuffer;
		}

		nextFramePasses->Enumerate<FramePass>([&](FramePass *nextPass, size_t index, bool &stop) {
			SubmitFramePass(frameSubmission, camera, nextPass, frameSubmission.renderPasses[frameSubmission.activeRenderPassIndex], multiviewSnapshotCameras);
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

		return 1;
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
		return 1;
	}

	void VulkanRenderer::CreateMipMapForTexture(VulkanTexture *texture)
	{
		LockGuard<Lockable> lock(_lock);
		_mipMapTextures->AddObject(texture);
	}

	void VulkanRenderer::CreateMipMaps()
	{
		RN_PROFILE_SCOPE();
		AssertOnRenderThread();
		Array *mipMapTextures = nullptr;
		{
			LockGuard<Lockable> lock(_lock);
			if(_mipMapTextures->GetCount() > 0)
			{
				mipMapTextures = new Array(_mipMapTextures);
				_mipMapTextures->RemoveAllObjects();
			}
		}

		if(!mipMapTextures)
			return;

		VulkanCommandBuffer *commandBuffer = StartResourcesCommandBuffer();

		mipMapTextures->Enumerate<VulkanTexture>([&](VulkanTexture *texture, size_t index, bool &stop) {
			texture->GenerateMipMaps(commandBuffer->GetCommandBuffer());
		});

		EndResourcesCommandBuffer();

		mipMapTextures->Release();
	}

	GPUBuffer *VulkanRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool isStreamable)
	{
		if(isStreamable)
		{
			return new VulkanDynamicGPUBuffer(this, length, usageOptions);
		}

		return (new VulkanStaticGPUBuffer(this, nullptr, length, usageOptions, accessOptions));
	}

	VulkanDynamicBufferReference *VulkanRenderer::GetConstantBufferReference(size_t size, size_t index, GPUResource::UsageOptions usageOptions)
	{
		AssertOnRenderThread();
		VulkanDynamicBufferReference *reference = _dynamicBufferPool->GetDynamicBufferReference(size, index, usageOptions);
		return reference;
	}

	void VulkanRenderer::UpdateDynamicBufferReference(VulkanDynamicBufferReference *reference, bool align)
	{
		AssertOnRenderThread();
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

	Texture *VulkanRenderer::CreateTextureWithExternalMemory(const Texture::Descriptor &descriptor, const Texture::ExternalMemoryDescriptor &externalMemoryDescriptor)
	{
		return new VulkanTexture(descriptor, this, externalMemoryDescriptor);
	}

	Framebuffer *VulkanRenderer::CreateFramebuffer(const Vector2 &size)
	{
		return new VulkanFramebuffer(size, this);
	}

	const std::vector<VkTilePropertiesQCOM> *VulkanRenderer::GetFramebufferTileProperties(const Framebuffer *framebuffer) const
	{
		const VulkanFramebuffer *vulkanFramebuffer = framebuffer ? framebuffer->Downcast<VulkanFramebuffer>() : nullptr;
		if(!vulkanFramebuffer) return nullptr;

		const std::vector<VkTilePropertiesQCOM> &tileProperties = vulkanFramebuffer->GetCurrentVariantTileProperties();
		return tileProperties.empty() ? nullptr : &tileProperties;
	}

	void VulkanRenderer::FillUniformBuffer(Shader::ArgumentBuffer *argumentBuffer, VulkanDynamicBufferReference *dynamicBufferReference, const RenderFrame::DrawItem &drawItem, const Material::Properties &mergedMaterialProperties, const RenderFrame::Pass &framePass)
	{
		uint8 *buffer = reinterpret_cast<uint8 *>(dynamicBufferReference->dynamicBuffer->GetBuffer()) + dynamicBufferReference->offset;
		FillDrawUniformBuffer(argumentBuffer, buffer, drawItem, mergedMaterialProperties, framePass);
	}

	Drawable *VulkanRenderer::CreateDrawable()
	{
		VulkanDrawable *newDrawable = new VulkanDrawable();
		return newDrawable;
	}

	void VulkanRenderer::DeleteDrawable(Drawable *drawable)
	{
		QueueDrawableDeletion(drawable);
	}

	void VulkanRenderer::SubmitLight(const Light *light)
	{
		VulkanFrameSubmission &frameSubmission = GetActiveFrameSubmission();

		// Distribute the light to all passes belonging to the current camera range
		size_t startIndex = frameSubmission.activeRenderPassIndex;

		auto submitLightToRenderPass = [&](VulkanRenderPass &renderPass) {
			if(!renderPass.UsesDrawItems()) return;
			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(renderPass.renderFramePassIndex);

			framePass.AddLight(light);
		};

		for(size_t pi = startIndex; pi < frameSubmission.renderPasses.size(); pi++)
		{
			VulkanRenderPass &renderPass = frameSubmission.renderPasses[pi];
			submitLightToRenderPass(renderPass);
			for(VulkanRenderPass &subpass : renderPass.subpasses)
			{
				submitLightToRenderPass(subpass);
			}
		}
	}

	void VulkanRenderer::WarmupDrawable(Mesh *mesh, Material *material, Camera *camera)
	{
		Renderer::WarmupDrawable(mesh, material, camera);
		if(!mesh || !material || !camera) return;

		AssertOnSubmissionThread();

		Mesh::DrawSnapshot meshSnapshot;
		mesh->GetDrawSnapshot(meshSnapshot);

		Material::DrawSnapshot materialSnapshot;
		material->GetDrawSnapshot(materialSnapshot);
		uint64 materialSnapshotVersion = material->GetDrawSnapshotVersion();

		StrongRef<Camera> cameraRef(camera);
		VulkanFrameSubmission submission;
		RenderFrame *previousRenderFrame = SetActiveRenderFrame(&submission.renderFrame);
		ScopeGuard activeRenderFrameGuard([this, previousRenderFrame]() {
			SetActiveRenderFrame(previousRenderFrame);
		});
		SubmitCamera(submission, camera, RN::MakeFunction([](){}));
		if(submission.renderPasses.empty())
			return;

		ScheduleRenderThreadWork([this, submission = std::move(submission), meshSnapshot = std::move(meshSnapshot), materialSnapshot = std::move(materialSnapshot), materialSnapshotVersion, cameraRef = std::move(cameraRef)]() mutable {
			cameraRef.Get(); // Keep the camera-owned render pass resources alive until the warmup task is consumed.
			WarmupDrawableOnRenderThread(submission, meshSnapshot, materialSnapshot, materialSnapshotVersion);
		});
	}

	void VulkanRenderer::WarmupDrawableOnRenderThread(const VulkanFrameSubmission &submission, const Mesh::DrawSnapshot &meshSnapshot, const Material::DrawSnapshot &materialSnapshot, uint64 materialSnapshotVersion)
	{
		AssertOnRenderThread();

		auto warmupRenderPass = [&](const VulkanRenderPass &rootRenderPass, const VulkanRenderPass &renderPass, uint32 subpassIndex) {
			if(!renderPass.UsesDrawItems() || !rootRenderPass.framebuffer)
				return;

			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
			const Material::DrawSnapshot *overrideMaterialSnapshot = framePass.GetOverrideMaterialSnapshot();
			Drawable::MergedMaterialSnapshot mergedMaterialSnapshot;
			mergedMaterialSnapshot.Update(materialSnapshot, materialSnapshotVersion, renderPass.shaderHint, overrideMaterialSnapshot, framePass.GetOverrideMaterialCacheIdentity(), framePass.GetOverrideMaterialSnapshotVersion());
			_internals->stateCoordinator.GetRenderPipelineState(mergedMaterialSnapshot.GetVertexShader(), mergedMaterialSnapshot.GetFragmentShader(), meshSnapshot, mergedMaterialSnapshot.GetPipelineProperties(), submission.renderFrame, &rootRenderPass, subpassIndex, framePass.GetMultiviewCameraCount());
		};

		for(const VulkanRenderPass &renderPass : submission.renderPasses)
		{
			if(renderPass.previousStoredFramebuffer)
				continue;

			if(renderPass.subpasses.size() > 0)
			{
				for(size_t i = 0; i < renderPass.subpasses.size(); i += 1)
				{
					warmupRenderPass(renderPass, renderPass.subpasses[i], static_cast<uint32>(i));
				}
			}
			else
			{
				warmupRenderPass(renderPass, renderPass, 0);
			}
		}
	}

	bool VulkanRenderer::PrepareRenderFrame(VulkanFrameSubmission &submission)
	{
		RN_PROFILE_SCOPE();
		RN_PROFILE_ATRACE_SCOPE_N("VK Prepare Render Frame");
		AssertOnRenderThread();
		_internals->stateCoordinator.SavePipelineCache(Kernel::GetSharedInstance()->GetApplication()->GetBuildNumber(), GetVulkanDevice()); //This won't do anything if no new pipelines were loaded

		UpdateFrameFences(); //Releases resources of frames that finished

		const bool hasCompletedFrame = (_completedFrame != static_cast<size_t>(-1));
		if((!hasCompletedFrame && _currentFrame > 4) || (hasCompletedFrame && (_currentFrame - _completedFrame > 4)))
		{
			//RNDebug("Too many frames in-flight, ignore this one");
			return false; //Don't submit a new frame if there are already 5 frames in flight
		}

		PrepareRendererAttachments(submission.renderFrame);
		CreateMipMaps();
		SubmitPendingResourceCommandBuffers();

		submission.preparedRenderPasses.clear();
		submission.preparedRenderPasses.reserve(submission.renderFrame.GetPassCount());

		auto ensureRenderPassResources = [&](VulkanRenderPass &renderSubPass) {
			renderSubPass.preparedRenderPassIndex = RenderFrame::InvalidPassIndex;

			if(!renderSubPass.UsesDrawItems())
			{
				return;
			}

			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderSubPass.renderFramePassIndex);
			const std::vector<size_t> &drawItemIndices = framePass.GetDrawItemIndices();
			renderSubPass.preparedRenderPassIndex = submission.preparedRenderPasses.size();
			submission.preparedRenderPasses.emplace_back();

			for(size_t drawItemIndex : drawItemIndices)
			{
				const RenderFrame::DrawItem &drawItem = submission.renderFrame.GetDrawItem(drawItemIndex);
				VulkanDrawable *drawable = static_cast<VulkanDrawable *>(drawItem.GetSourceDrawableForPreparation());
				drawable->EnsureRenderResources(renderSubPass.preparedRenderPassIndex);
			}
		};

		auto appendPreparedDrawItem = [](VulkanPreparedRenderPass &preparedPass, const RenderFrame::DrawItem &drawItem, VulkanDrawable::RenderResources &renderResources, RenderFrame::CameraStatistics &statistics) {
			VulkanPreparedDrawItem preparedDrawItem;
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

		auto getPipelineKey = [](const RenderFrame::DrawItem &drawItem, const VulkanRenderPass &renderPass, const VulkanRenderPass &renderSubPass, const Drawable::MergedMaterialSnapshot &mergedMaterialSnapshot, uint8 renderViewCount, uint32 subpassIndex) {
			Drawable::PipelineKey pipelineKey;
			pipelineKey.meshPipelineHash = drawItem.GetMesh().GetPipelineHash();
			pipelineKey.framebuffer = renderPass.framebuffer;
			pipelineKey.vertexShader = mergedMaterialSnapshot.GetVertexShader();
			pipelineKey.fragmentShader = mergedMaterialSnapshot.GetFragmentShader();
			pipelineKey.materialProperties = mergedMaterialSnapshot.GetPipelineProperties();
			pipelineKey.renderPass = renderSubPass.renderPass;
			pipelineKey.renderPassSignature = renderPass.subpassSignature;
			pipelineKey.renderViewCount = renderViewCount;
			pipelineKey.subpassIndex = subpassIndex;
			return pipelineKey;
		};

		for(VulkanRenderPass &renderPass : submission.renderPasses)
		{
			if(renderPass.type == VulkanRenderPass::Type::Compute)
			{
				Shader *computeShader = renderPass.computeDispatch.GetShader();
				RN_ASSERT(computeShader && computeShader->GetType() == Shader::Type::Compute, "Vulkan compute pass requires a compute shader");
				renderPass.computePipelineState = _internals->stateCoordinator.GetComputePipelineState(computeShader);
				renderPass.computeDescriptorSet = _internals->descriptorPool.Allocate(this, renderPass.computePipelineState->rootSignature->descriptorSetLayout);
				if(computeShader && computeShader->GetSignature())
				{
					computeShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop) {
						if(buffer->GetSource() != Shader::ArgumentBuffer::Source::Draw)
							return;
						if(buffer->GetType() != Shader::ArgumentBuffer::Type::UniformBuffer)
							return;

						size_t totalSize = buffer->GetTotalUniformSize();
						if(totalSize > 0)
						{
							renderPass.computeUniformState.computeConstantBuffers.push_back(GetConstantBufferReference(totalSize, buffer->GetIndex())->Retain());
						}
					});
				}
				continue;
			}

			if(renderPass.UsesDrawItems() && renderPass.subpasses.size() > 0)
			{
				renderPass.preparedRenderPassIndex = RenderFrame::InvalidPassIndex;
				for(VulkanRenderPass &renderSubPass : renderPass.subpasses)
				{
					ensureRenderPassResources(renderSubPass);
				}
			}
			else
			{
				ensureRenderPassResources(renderPass);
			}
		}

		auto prepareRenderPass = [&](VulkanRenderPass &renderPass, VulkanRenderPass &renderSubPass, uint32 subpassIndex) {
			if(renderSubPass.preparedRenderPassIndex == RenderFrame::InvalidPassIndex)
				return;

			VulkanPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderSubPass.preparedRenderPassIndex];
			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderSubPass.renderFramePassIndex);
			const std::vector<size_t> &drawItemIndices = framePass.GetDrawItemIndices();
			if(drawItemIndices.empty())
				return;

			preparedPass.drawItems.reserve(drawItemIndices.size());

			RenderFrame::CameraStatistics &statistics = submission.renderFrame.GetCameraStatistics(renderSubPass.frameStatisticsIndex);

			for(size_t drawItemIndex : drawItemIndices)
			{
				const RenderFrame::DrawItem &drawItem = submission.renderFrame.GetDrawItem(drawItemIndex);
				VulkanDrawable *drawable = static_cast<VulkanDrawable *>(drawItem.GetSourceDrawableForPreparation());
				VulkanDrawable::RenderResources &renderResources = drawable->GetRenderResources(renderSubPass.preparedRenderPassIndex);

				const Material::DrawSnapshot *overrideMaterialSnapshot = framePass.GetOverrideMaterialSnapshot();
				renderResources.mergedMaterialSnapshot.Update(drawItem.GetMaterial(), drawItem.GetMaterialSnapshotVersion(), renderSubPass.shaderHint, overrideMaterialSnapshot, framePass.GetOverrideMaterialCacheIdentity(), framePass.GetOverrideMaterialSnapshotVersion());
				Drawable::PipelineKey pipelineKey = getPipelineKey(drawItem, renderPass, renderSubPass, renderResources.mergedMaterialSnapshot, framePass.GetMultiviewCameraCount(), subpassIndex);

				if(!renderResources.pipelineState || renderResources.pipelineKey != pipelineKey)
				{
					const VulkanPipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(pipelineKey.vertexShader, pipelineKey.fragmentShader, drawItem.GetMesh(), pipelineKey.materialProperties, submission.renderFrame, &renderPass, subpassIndex, pipelineKey.renderViewCount);
					VulkanUniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState);

					RN_ASSERT(pipelineState && uniformState, "Failed to create pipeline or uniform state for drawable!");
					drawable->UpdateRenderingState(renderResources, pipelineState, uniformState, pipelineKey);
				}

				appendPreparedDrawItem(preparedPass, drawItem, renderResources, statistics);
			}

			if(framePass.GetCameraSnapshot().GetSortInstancable())
			{
				std::sort(preparedPass.drawItems.begin(), preparedPass.drawItems.end(), [](const VulkanPreparedDrawItem &a, const VulkanPreparedDrawItem &b) { return a.instancingSortKey < b.instancingSortKey; });
			}

			const VulkanPreparedDrawItem *currentInstanceDrawItem = nullptr;
			for(VulkanPreparedDrawItem &preparedDrawItem : preparedPass.drawItems)
			{
				const bool isCompatible = currentInstanceDrawItem && preparedDrawItem.instancingSortKey.CanInstanceWith(currentInstanceDrawItem->instancingSortKey);
				const bool hasInstanceCapacity = currentInstanceDrawItem && preparedPass.instanceSteps.back() < preparedDrawItem.renderResources->maxInstanceCount;
				if(hasInstanceCapacity && isCompatible)
				{
					preparedPass.instanceSteps.back() += 1;
					continue;
				}

				currentInstanceDrawItem = &preparedDrawItem;
				preparedPass.instanceSteps.push_back(1);
				statistics.numberOfDrawCalls += 1;
				VulkanDrawable::RenderResources &renderResources = *preparedDrawItem.renderResources;
				renderResources.descriptorSet = _internals->descriptorPool.Allocate(this, renderResources.pipelineState->rootSignature->descriptorSetLayout);
			}
		};

		for(VulkanRenderPass &renderPass : submission.renderPasses)
		{
			if(!renderPass.UsesDrawItems())
			{
				continue;
			}
			else if(renderPass.subpasses.size() > 0)
			{
				uint32 subpassIndex = 0;
				for(VulkanRenderPass &renderSubPass : renderPass.subpasses)
				{
					prepareRenderPass(renderPass, renderSubPass, subpassIndex++);
				}
			}
			else
			{
				prepareRenderPass(renderPass, renderPass, 0);
			}
		}

		// Do this after pipeline preparation so newly created uniform references have backing buffers.
		_dynamicBufferPool->Update(this, _currentFrame, _completedFrame);
		UpdateDescriptorSets(submission);
		return true;
	}

	void VulkanRenderer::SubmitDrawable(Drawable *drawable, const SceneNode *node)
	{
		SubmitDrawable(GetActiveFrameSubmission(), drawable, node);
	}

	void VulkanRenderer::SubmitDrawable(Drawable *drawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID, int32 renderPriority)
	{
		SubmitDrawable(GetActiveFrameSubmission(), drawable, modelMatrix, inverseModelMatrix, renderGroup, sourceNodeUID, renderPriority);
	}

	void VulkanRenderer::SubmitDrawable(VulkanFrameSubmission &frameSubmission, Drawable *sourceDrawable, const SceneNode *node)
	{
		VulkanDrawable *drawable = static_cast<VulkanDrawable *>(sourceDrawable);
		uint16 renderGroup = node ? node->GetRenderGroup() : 0xffff;
		size_t drawItemIndex = RenderFrame::InvalidDrawItemIndex;

		auto submitDrawable = [&](VulkanRenderPass &renderSubPass) {
			if(!renderSubPass.UsesDrawItems())
				return;

			if((renderSubPass.type == VulkanRenderPass::Type::Convert || renderSubPass.renderPass->IsKindOfClass(PostProcessingStage::GetMetaClass())) && drawable != _defaultPostProcessingDrawable)
				return;

			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(renderSubPass.renderFramePassIndex);
			const RenderPass::DrawSnapshot &renderSubPassDrawSnapshot = framePass.GetDrawSnapshot();
			if((renderGroup & renderSubPassDrawSnapshot.GetRenderGroupMask()) == 0)
				return;

			if(drawItemIndex == RenderFrame::InvalidDrawItemIndex)
				drawItemIndex = frameSubmission.renderFrame.AddDrawItem(drawable, node, framePass);
			framePass.AddDrawItemIndex(drawItemIndex);
		};

		for(size_t pi = frameSubmission.activeRenderPassIndex; pi < frameSubmission.renderPasses.size(); pi += 1)
		{
			VulkanRenderPass &renderPass = frameSubmission.renderPasses[pi];
			if(!renderPass.UsesDrawItems())
			{
				continue;
			}
			else if(renderPass.subpasses.size() > 0)
			{
				for(VulkanRenderPass &renderSubPass : renderPass.subpasses)
				{
					submitDrawable(renderSubPass);
				}
			}
			else
			{
				submitDrawable(renderPass);
			}
		}
	}

	void VulkanRenderer::SubmitDrawable(VulkanFrameSubmission &frameSubmission, Drawable *sourceDrawable, const Matrix &modelMatrix, const Matrix &inverseModelMatrix, uint16 renderGroup, uint64 sourceNodeUID, int32 renderPriority)
	{
		VulkanDrawable *drawable = static_cast<VulkanDrawable *>(sourceDrawable);
		size_t drawItemIndex = RenderFrame::InvalidDrawItemIndex;

		auto submitDrawable = [&](VulkanRenderPass &renderSubPass) {
			if(!renderSubPass.UsesDrawItems())
				return;

			if((renderSubPass.type == VulkanRenderPass::Type::Convert || renderSubPass.renderPass->IsKindOfClass(PostProcessingStage::GetMetaClass())) && drawable != _defaultPostProcessingDrawable)
				return;

			RenderFrame::Pass &framePass = frameSubmission.renderFrame.GetPass(renderSubPass.renderFramePassIndex);
			const RenderPass::DrawSnapshot &renderSubPassDrawSnapshot = framePass.GetDrawSnapshot();
			if((renderGroup & renderSubPassDrawSnapshot.GetRenderGroupMask()) == 0)
				return;

			if(drawItemIndex == RenderFrame::InvalidDrawItemIndex)
				drawItemIndex = frameSubmission.renderFrame.AddDrawItem(drawable, modelMatrix, inverseModelMatrix, sourceNodeUID, renderPriority);
			framePass.AddDrawItemIndex(drawItemIndex);
		};

		for(size_t pi = frameSubmission.activeRenderPassIndex; pi < frameSubmission.renderPasses.size(); pi += 1)
		{
			VulkanRenderPass &renderPass = frameSubmission.renderPasses[pi];
			if(!renderPass.UsesDrawItems())
			{
				continue;
			}
			else if(renderPass.subpasses.size() > 0)
			{
				for(VulkanRenderPass &renderSubPass : renderPass.subpasses)
				{
					submitDrawable(renderSubPass);
				}
			}
			else
			{
				submitDrawable(renderPass);
			}
		}
	}

	void VulkanRenderer::UpdateDescriptorSets(VulkanFrameSubmission &submission)
	{
		RN_PROFILE_SCOPE();

		uint32 totalConstantBufferCount = 0;
		uint32 totalTextureCount = 0;
		uint32 totalSubpassInputCount = 0;

		auto resolveVulkanBuffer = [&](Shader::ArgumentBuffer *argument, GPUBuffer *buffer, const char *missingMessage, const char *invalidMessage) -> GPUBuffer * {
			RN_DEBUG_ASSERT(buffer, "%s '%s' at buffer binding %u", missingMessage, argument->GetName()->GetUTF8String(), argument->GetIndex());

			if(buffer)
			{
				VulkanGPUBuffer *vulkanBuffer = buffer->Downcast<VulkanGPUBuffer>();
				RN_DEBUG_ASSERT(vulkanBuffer, "%s '%s' at buffer binding %u", invalidMessage, argument->GetName()->GetUTF8String(), argument->GetIndex());
				if(vulkanBuffer)
					return buffer;
			}

			return _fallbackGlobalBuffer;
		};

		auto getPassResourceBuffer = [&](const RenderFrame::Pass &framePass, Shader::ArgumentBuffer *argument) -> GPUBuffer * {
			return resolveVulkanBuffer(argument, framePass.GetPassResourceBuffer(argument->GetNameHash()), "Missing pass resource buffer", "Pass resource buffer must be a Vulkan buffer");
		};

		auto getFrameGlobalBuffer = [&](Shader::ArgumentBuffer *argument) -> GPUBuffer * {
			return resolveVulkanBuffer(argument, submission.renderFrame.GetGlobalBuffer(argument->GetNameHash()), "Missing frame global buffer", "Frame global buffer must be a Vulkan buffer");
		};

		auto enumeratePassBufferDescriptors = [&](const RenderFrame::Pass &framePass, Shader *shader, auto &&callback) {
			if(!shader || !shader->GetSignature()) return;

			shader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop) {
				if(argument->GetSource() != Shader::ArgumentBuffer::Source::Pass)
					return;

				callback(argument, getPassResourceBuffer(framePass, argument));
			});
		};

		auto enumerateGlobalBufferDescriptors = [&](Shader *shader, auto &&callback) {
			if(!shader || !shader->GetSignature()) return;

			shader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop) {
				if(argument->GetSource() != Shader::ArgumentBuffer::Source::Frame)
					return;

				callback(argument, getFrameGlobalBuffer(argument));
			});
		};

		auto countPassBufferDescriptors = [&](const RenderFrame::Pass &framePass, Shader *shader) {
			enumeratePassBufferDescriptors(framePass, shader, [&](Shader::ArgumentBuffer *, GPUBuffer *) {
				totalConstantBufferCount += 1;
			});
		};

		auto countGlobalBufferDescriptors = [&](Shader *shader) {
			enumerateGlobalBufferDescriptors(shader, [&](Shader::ArgumentBuffer *, GPUBuffer *) {
				totalConstantBufferCount += 1;
			});
		};

		auto accumulateDescriptorCounts = [&](const VulkanRenderPass &renderPass) {
			RN_DEBUG_ASSERT(renderPass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
			const VulkanPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderPass.preparedRenderPassIndex];
			const std::vector<VulkanPreparedDrawItem> &drawItems = preparedPass.drawItems;
			if(drawItems.empty())
				return;

			const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
			uint32 stepSize = 0;
			uint32 stepSizeIndex = 0;
			for(size_t i = 0; i < drawItems.size(); i += stepSize)
			{
				stepSize = preparedPass.instanceSteps[stepSizeIndex++];

				const auto &resources = *drawItems[i].renderResources;
				const VulkanUniformState *uniformState = resources.uniformState;
				const VulkanPipelineState *pipelineState = resources.pipelineState;

				totalConstantBufferCount += uniformState->vertexConstantBuffers.size();
				totalConstantBufferCount += uniformState->fragmentConstantBuffers.size();

				countPassBufferDescriptors(framePass, pipelineState->descriptor.vertexShader);
				countPassBufferDescriptors(framePass, pipelineState->descriptor.fragmentShader);
				countGlobalBufferDescriptors(pipelineState->descriptor.vertexShader);
				countGlobalBufferDescriptors(pipelineState->descriptor.fragmentShader);

				totalTextureCount += pipelineState->rootSignature->textureCount;
				totalSubpassInputCount += pipelineState->rootSignature->subpassInputCount;
			}
		};

		auto accumulateComputeDescriptorCounts = [&](const VulkanRenderPass &computePass) {
			Shader *computeShader = computePass.computeDispatch.GetShader();
			if(!computeShader || !computeShader->GetSignature())
				return;

			totalConstantBufferCount += computeShader->GetSignature()->GetBuffers()->GetCount();
			totalTextureCount += computeShader->GetSignature()->GetTextures()->GetCount();
		};

		for(const VulkanRenderPass &renderPass : submission.renderPasses)
		{
			RN_PROFILE_SCOPE();
			if(renderPass.type == VulkanRenderPass::Type::Compute)
			{
				accumulateComputeDescriptorCounts(renderPass);
				continue;
			}

			if(!renderPass.UsesDrawItems())
			{
				continue;
			}

			if(renderPass.subpasses.size() > 0)
			{
				for(const VulkanRenderPass &subpass : renderPass.subpasses)
				{
					accumulateDescriptorCounts(subpass);
				}
			}
			else
			{
				accumulateDescriptorCounts(renderPass);
			}
		}

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.reserve(totalConstantBufferCount + totalTextureCount + totalSubpassInputCount);
		std::vector<VkDescriptorBufferInfo> constantBufferDescriptorInfoArray;
		constantBufferDescriptorInfoArray.reserve(totalConstantBufferCount);
		std::vector<VkDescriptorImageInfo> imageBufferDescriptorInfoArray;
		imageBufferDescriptorInfoArray.reserve(totalTextureCount);
		std::vector<VkDescriptorImageInfo> subpassInputDescriptorInfoArray;
		subpassInputDescriptorInfoArray.reserve(totalSubpassInputCount);

		auto addBufferDescriptor = [&](VkDescriptorSet descriptorSet, Shader::ArgumentBuffer *argument, GPUBuffer *gpuBuffer, size_t offset, size_t range) {
			VkDescriptorBufferInfo constantBufferDescriptorInfo = {};
			constantBufferDescriptorInfo.buffer = gpuBuffer->Downcast<VulkanGPUBuffer>()->GetVulkanBuffer();
			constantBufferDescriptorInfo.offset = offset;
			constantBufferDescriptorInfo.range = range;
			constantBufferDescriptorInfoArray.push_back(constantBufferDescriptorInfo);

			VkWriteDescriptorSet writeConstantDescriptorSet = {};
			writeConstantDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeConstantDescriptorSet.pNext = NULL;
			writeConstantDescriptorSet.dstSet = descriptorSet;
			writeConstantDescriptorSet.descriptorType = (argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeConstantDescriptorSet.dstBinding = argument->GetIndex();
			writeConstantDescriptorSet.pBufferInfo = &constantBufferDescriptorInfoArray.back();
			writeConstantDescriptorSet.descriptorCount = 1;

			writeDescriptorSets.push_back(writeConstantDescriptorSet);
		};

		auto addPassBufferDescriptors = [&](VkDescriptorSet descriptorSet, const RenderFrame::Pass &framePass, Shader *shader) {
			enumeratePassBufferDescriptors(framePass, shader, [&](Shader::ArgumentBuffer *argument, GPUBuffer *passBuffer) {
				addBufferDescriptor(descriptorSet, argument, passBuffer, 0, passBuffer->GetLength());
			});
		};

		auto addGlobalBufferDescriptors = [&](VkDescriptorSet descriptorSet, Shader *shader) {
			enumerateGlobalBufferDescriptors(shader, [&](Shader::ArgumentBuffer *argument, GPUBuffer *globalBuffer) {
				addBufferDescriptor(descriptorSet, argument, globalBuffer, 0, globalBuffer->GetLength());
			});
		};

		auto addComputeDescriptors = [&](VulkanRenderPass &computePass) {
			Shader *computeShader = computePass.computeDispatch.GetShader();
			if(!computeShader || !computeShader->GetSignature())
				return;

			VkDescriptorSet descriptorSet = computePass.computeDescriptorSet;
			size_t dynamicUniformIndex = 0;

			computeShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop) {
				if(argument->GetSource() == Shader::ArgumentBuffer::Source::Draw && argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer && argument->GetTotalUniformSize() > 0)
				{
					RN_DEBUG_ASSERT(dynamicUniformIndex < computePass.computeUniformState.computeConstantBuffers.size(), "Missing compute uniform buffer");
					if(dynamicUniformIndex < computePass.computeUniformState.computeConstantBuffers.size())
					{
						VulkanDynamicBufferReference *constantBuffer = computePass.computeUniformState.computeConstantBuffers[dynamicUniformIndex++];
						_dynamicBufferPool->UpdateDynamicBufferReference(constantBuffer, true);
						uint8 *buffer = reinterpret_cast<uint8 *>(constantBuffer->dynamicBuffer->GetBuffer()) + constantBuffer->offset;
						std::memset(buffer, 0, constantBuffer->size);

						const RenderFrame::CameraSnapshot &cameraSnapshot = computePass.computeCameraSnapshot;
						const std::vector<RenderFrame::CameraSnapshot> &multiviewCameraSnapshots = computePass.computeMultiviewCameraSnapshots;

						argument->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
							const std::vector<uint8> *uniform = computePass.computeDispatch.GetUniform(descriptor->GetNameHash());
							if(uniform)
							{
								size_t copySize = std::min(uniform->size(), descriptor->GetSize());
								std::memcpy(buffer + descriptor->GetOffset(), uniform->data(), copySize);
							}
							else
							{
								FillCommonUniform(descriptor, buffer, &cameraSnapshot, &multiviewCameraSnapshots);
							}
						});

						GPUBuffer *gpuBuffer = constantBuffer->dynamicBuffer->GetActiveGPUBuffer();
						addBufferDescriptor(descriptorSet, argument, gpuBuffer, constantBuffer->offset, constantBuffer->size);
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

				buffer = resolveVulkanBuffer(argument, buffer, "Missing compute resource buffer", "Compute resource buffer must be a Vulkan buffer");
				addBufferDescriptor(descriptorSet, argument, buffer, 0, buffer->GetLength());
			});

			computeShader->GetSignature()->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop) {
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
							texture = computePass.previousStoredFramebuffer->GetColorTexture(0);
						RN_DEBUG_ASSERT(texture, "Missing previous framebuffer texture for compute texture '%s' at texture binding %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
						break;

					case Shader::ArgumentTexture::Source::Material:
					case Shader::ArgumentTexture::Source::Pass:
					case Shader::ArgumentTexture::Source::SubpassInput:
						texture = computePass.computeDispatch.GetResourceTexture(argument->GetNameHash());
						break;
				}

				VulkanTexture *vulkanTexture = texture ? texture->Downcast<VulkanTexture>() : nullptr;
				RN_DEBUG_ASSERT(vulkanTexture, "Missing compute resource texture '%s' at texture binding %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
				if(!vulkanTexture)
				{
					vulkanTexture = _fallbackGlobalTexture->Downcast<VulkanTexture>();
				}

				VkDescriptorImageInfo imageBufferDescriptorInfo = {};
				imageBufferDescriptorInfo.imageView = vulkanTexture->_imageView;
				imageBufferDescriptorInfo.imageLayout = argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageBufferDescriptorInfoArray.push_back(imageBufferDescriptorInfo);

				VkWriteDescriptorSet writeImageDescriptorSet = {};
				writeImageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeImageDescriptorSet.pNext = NULL;
				writeImageDescriptorSet.dstSet = descriptorSet;
				writeImageDescriptorSet.descriptorType = argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				writeImageDescriptorSet.dstBinding = argument->GetIndex();
				writeImageDescriptorSet.pImageInfo = &imageBufferDescriptorInfoArray[imageBufferDescriptorInfoArray.size() - 1];
				writeImageDescriptorSet.descriptorCount = 1;

				writeDescriptorSets.push_back(writeImageDescriptorSet);
			});
		};

		auto updateDescriptorSets = [&](const VulkanRenderPass &renderPass, VulkanRenderPass &rootRenderPass) {
			RN_DEBUG_ASSERT(renderPass.preparedRenderPassIndex < submission.preparedRenderPasses.size(), "Invalid prepared render pass index");
			const VulkanPreparedRenderPass &preparedPass = submission.preparedRenderPasses[renderPass.preparedRenderPassIndex];
			const std::vector<VulkanPreparedDrawItem> &drawItems = preparedPass.drawItems;
			if(!drawItems.empty())
			{
				const RenderFrame::Pass &framePass = submission.renderFrame.GetPass(renderPass.renderFramePassIndex);
				std::vector<uint32> subpassInputColorIndices;
				bool subpassReadsDepthStencilAttachment = false;
				VulkanFramebuffer *rootFramebuffer = rootRenderPass.framebuffer;
				std::vector<VkImageView> subpassInputColorViews;
				VkImageView subpassInputDepthView = VK_NULL_HANDLE;

				if(rootFramebuffer && rootRenderPass.subpasses.size() > 0)
				{
					const RenderPass::SubpassSnapshot &subpassSnapshot = framePass.GetDrawSnapshot().GetSubpass();
					uint32 totalColorAttachments = rootFramebuffer->_swapChain ? 1 : static_cast<uint32>(rootFramebuffer->_colorTargets.size());
					for(uint32 ci = 0; ci < totalColorAttachments; ci++)
					{
						if(subpassSnapshot.GetColorAttachment(ci).GetReads())
						{
							subpassInputColorIndices.push_back(ci);
						}
					}
					subpassReadsDepthStencilAttachment = subpassSnapshot.GetReadsDepthStencil();

					if(subpassInputColorIndices.size() > 0)
					{
						subpassInputColorViews.reserve(subpassInputColorIndices.size());
						for(uint32 colorIndex : subpassInputColorIndices)
						{
							Texture *texture = rootFramebuffer->GetColorTexture(colorIndex);
							VulkanTexture *framebufferTexture = texture ? texture->Downcast<VulkanTexture>() : nullptr;
							subpassInputColorViews.push_back(framebufferTexture ? framebufferTexture->_imageView : VK_NULL_HANDLE);
						}
					}

					if(subpassReadsDepthStencilAttachment)
					{
						Texture *depthTexture = rootFramebuffer->GetDepthStencilTexture();
						VulkanTexture *depthFramebufferTexture = depthTexture ? depthTexture->Downcast<VulkanTexture>() : nullptr;
						subpassInputDepthView = depthFramebufferTexture ? depthFramebufferTexture->_imageView : VK_NULL_HANDLE;
					}
				}

				VkImageView previousPassColorView = VK_NULL_HANDLE;
				if(rootRenderPass.previousStoredFramebuffer)
				{
					VulkanFramebuffer *previousFramebuffer = rootRenderPass.previousStoredFramebuffer;
					if(previousFramebuffer)
					{
						Texture *previousColorTexture = previousFramebuffer->GetColorTexture();
						if(previousColorTexture)
						{
							VulkanTexture *previousColorVulkanTexture = previousColorTexture->Downcast<VulkanTexture>();
							previousPassColorView = previousColorVulkanTexture ? previousColorVulkanTexture->_imageView : VK_NULL_HANDLE;
						}
					}
				}

				size_t stepSize = 0;
				uint32 stepSizeIndex = 0;
				for(size_t i = 0; i < drawItems.size(); i += stepSize)
				{
					stepSize = preparedPass.instanceSteps[stepSizeIndex++];

					const VulkanPreparedDrawItem &preparedDrawItem = drawItems[i];
					const VulkanDrawable::RenderResources &renderResource = *preparedDrawItem.renderResources;

					const VulkanPipelineState *pipelineState = renderResource.pipelineState;

					VkDescriptorSet descriptorSet = renderResource.descriptorSet;

					VulkanUniformState *uniformState = renderResource.uniformState;
					if(uniformState->instanceAttributesBuffer)
					{
						//These are not actually part of the descripter sets, but filling them with data here anyway
						Shader::ArgumentBuffer *argument = uniformState->instanceAttributesArgumentBuffer;
						const size_t maxInstanceCount = argument->GetMaxInstanceCount();
						const size_t instanceCount = (maxInstanceCount == 0) ? stepSize : std::min(stepSize, maxInstanceCount);

						//Setup per instance uniforms as vertex data for all instances that are part of this draw call
						for(size_t instance = 0; instance < instanceCount; instance += 1)
						{
							const VulkanPreparedDrawItem &instancePreparedDrawItem = drawItems[i + instance];
							const RenderFrame::DrawItem &instanceDrawItem = *instancePreparedDrawItem.drawItem;
							const VulkanDrawable::RenderResources &instanceRenderResources = *instancePreparedDrawItem.renderResources;
							VulkanUniformState *instanceUniformState = instanceRenderResources.uniformState;
							VulkanDynamicBufferReference *instanceAttributesBuffer = instanceUniformState->instanceAttributesBuffer;
							_dynamicBufferPool->UpdateDynamicBufferReference(instanceAttributesBuffer, instance == 0);
							FillUniformBuffer(argument, instanceAttributesBuffer, instanceDrawItem, instanceRenderResources.mergedMaterialSnapshot.GetProperties(), framePass);
						}
					}

					size_t counter = 0;
					for(size_t bufferIndex = 0; bufferIndex < uniformState->vertexConstantBuffers.size(); bufferIndex += 1)
					{
						Shader::ArgumentBuffer *argument = uniformState->constantBufferToArgumentMapping[counter++];
						const size_t maxInstanceCount = argument->GetMaxInstanceCount();
						const size_t instanceCount = (maxInstanceCount == 0) ? stepSize : std::min(stepSize, maxInstanceCount);

						//Setup uniforms for all instances that are part of this draw call
						for(size_t instance = 0; instance < instanceCount; instance += 1)
						{
							const VulkanPreparedDrawItem &instancePreparedDrawItem = drawItems[i + instance];
							const RenderFrame::DrawItem &instanceDrawItem = *instancePreparedDrawItem.drawItem;
							const VulkanDrawable::RenderResources &instanceRenderResources = *instancePreparedDrawItem.renderResources;
							VulkanUniformState *instanceUniformState = instanceRenderResources.uniformState;
							_dynamicBufferPool->UpdateDynamicBufferReference(instanceUniformState->vertexConstantBuffers[bufferIndex], instance == 0);
							FillUniformBuffer(argument, instanceUniformState->vertexConstantBuffers[bufferIndex], instanceDrawItem, instanceRenderResources.mergedMaterialSnapshot.GetProperties(), framePass);
						}

						VulkanDynamicBufferReference *constantBuffer = uniformState->vertexConstantBuffers[bufferIndex];

						GPUBuffer *gpuBuffer = constantBuffer->dynamicBuffer->GetActiveGPUBuffer();
						addBufferDescriptor(descriptorSet, argument, gpuBuffer, constantBuffer->offset, constantBuffer->size * instanceCount);
					}

					for(size_t bufferIndex = 0; bufferIndex < uniformState->fragmentConstantBuffers.size(); bufferIndex += 1)
					{
						Shader::ArgumentBuffer *argument = uniformState->constantBufferToArgumentMapping[counter++];
						const size_t maxInstanceCount = argument->GetMaxInstanceCount();
						const size_t instanceCount = (maxInstanceCount == 0) ? stepSize : std::min(stepSize, maxInstanceCount);

						//Setup uniforms for all instances that are part of this draw call
						for(size_t instance = 0; instance < instanceCount; instance += 1)
						{
							const VulkanPreparedDrawItem &instancePreparedDrawItem = drawItems[i + instance];
							const RenderFrame::DrawItem &instanceDrawItem = *instancePreparedDrawItem.drawItem;
							const VulkanDrawable::RenderResources &instanceRenderResources = *instancePreparedDrawItem.renderResources;
							VulkanUniformState *instanceUniformState = instanceRenderResources.uniformState;
							_dynamicBufferPool->UpdateDynamicBufferReference(
								instanceUniformState->fragmentConstantBuffers[bufferIndex],
								instance == 0);
							FillUniformBuffer(argument, instanceUniformState->fragmentConstantBuffers[bufferIndex], instanceDrawItem, instanceRenderResources.mergedMaterialSnapshot.GetProperties(), framePass);
						}

						VulkanDynamicBufferReference *constantBuffer = uniformState->fragmentConstantBuffers[bufferIndex];

						GPUBuffer *gpuBuffer = constantBuffer->dynamicBuffer->GetActiveGPUBuffer();
						addBufferDescriptor(descriptorSet, argument, gpuBuffer, constantBuffer->offset, constantBuffer->size * instanceCount);
					}

					addPassBufferDescriptors(descriptorSet, framePass, pipelineState->descriptor.vertexShader);
					addPassBufferDescriptors(descriptorSet, framePass, pipelineState->descriptor.fragmentShader);
					addGlobalBufferDescriptors(descriptorSet, pipelineState->descriptor.vertexShader);
					addGlobalBufferDescriptors(descriptorSet, pipelineState->descriptor.fragmentShader);

					std::vector<Shader::ArgumentTexture *> writtenTextureArguments;
					auto getWrittenTextureArgument = [&](uint32 binding) -> Shader::ArgumentTexture * {
						for(Shader::ArgumentTexture *writtenArgument : writtenTextureArguments)
						{
							if(writtenArgument->GetIndex() == binding)
								return writtenArgument;
						}

						return nullptr;
					};
					auto usesSameTextureDescriptor = [](Shader::ArgumentTexture *first, Shader::ArgumentTexture *second) {
						if(first->GetType() != second->GetType())
							return false;
						if(first->GetSource() != second->GetSource())
							return false;

						switch(first->GetSource())
						{
							case Shader::ArgumentTexture::Source::Material:
								return first->GetMaterialTextureIndex() == second->GetMaterialTextureIndex();

							case Shader::ArgumentTexture::Source::Pass:
							case Shader::ArgumentTexture::Source::Frame:
								return first->GetNameHash() == second->GetNameHash();

							case Shader::ArgumentTexture::Source::Framebuffer:
								return true;

							case Shader::ArgumentTexture::Source::SubpassInput:
								return first->GetMaterialTextureIndex() == second->GetMaterialTextureIndex();
						}

						return false;
					};

					auto addFragmentSubpassInputDescriptors = [&](Shader *fragmentShader) {
						if(!fragmentShader || !fragmentShader->GetSignature())
							return;

						const Shader::Signature *signature = fragmentShader->GetSignature();
						signature->GetSubpassInputs()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop) {
							uint8 materialTextureIndex = argument->GetMaterialTextureIndex();
							bool isDepthInput = materialTextureIndex >= 128;
							materialTextureIndex = isDepthInput ? materialTextureIndex - 128 : materialTextureIndex;

							VkImageView imageView = VK_NULL_HANDLE;

							if(isDepthInput)
							{
								// Ensure this subpass reads depth
								if(!subpassReadsDepthStencilAttachment || !subpassInputDepthView)
								{
									stop = true;
									return;
								}

								imageView = subpassInputDepthView;
							}
							else
							{
								// Map color input ordinal to actual color attachment index via cached views
								if(materialTextureIndex >= subpassInputColorViews.size())
								{
									stop = true;
									return;
								}
								imageView = subpassInputColorViews[materialTextureIndex];
								if(imageView == VK_NULL_HANDLE)
								{
									stop = true;
									return;
								}
							}

							VkDescriptorImageInfo inputAttachmentDescriptorInfo = {};
							inputAttachmentDescriptorInfo.imageLayout = isDepthInput ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							inputAttachmentDescriptorInfo.imageView = imageView;
							subpassInputDescriptorInfoArray.push_back(inputAttachmentDescriptorInfo);

							VkWriteDescriptorSet writeImageDescriptorSet = {};
							writeImageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							writeImageDescriptorSet.pNext = NULL;
							writeImageDescriptorSet.dstSet = descriptorSet;
							writeImageDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
							writeImageDescriptorSet.dstBinding = argument->GetIndex();
							writeImageDescriptorSet.pImageInfo = &subpassInputDescriptorInfoArray[subpassInputDescriptorInfoArray.size() - 1];
							writeImageDescriptorSet.descriptorCount = 1;

							writeDescriptorSets.push_back(writeImageDescriptorSet);
						});
					};

					auto addShaderTextureDescriptors = [&](Shader *shader) {
						if(!shader || !shader->GetSignature())
							return;

						const Shader::Signature *signature = shader->GetSignature();
						const Array *textures = renderResource.mergedMaterialSnapshot.GetTextures();
						auto trackShaderTextureUsage = [&](Shader::ArgumentTexture *argument, Texture *texture, VulkanTexture *vulkanTexture) {
							if(!texture || !vulkanTexture) return;

							const Texture::UsageHint usageHint = vulkanTexture->GetDescriptor().usageHint;
							if(usageHint & Texture::UsageHint::RenderTarget)
							{
								rootRenderPass.renderTargetsUsedInShader.push_back(vulkanTexture);
								return;
							}

							if(argument->GetType() == Shader::ArgumentTexture::Type::Sampled && (usageHint & Texture::UsageHint::ShaderWrite))
							{
								rootRenderPass.shaderWriteTexturesUsedAsSampledImages.push_back(vulkanTexture);
							}
						};
						signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop) {
							Shader::ArgumentTexture *writtenArgument = getWrittenTextureArgument(argument->GetIndex());
							if(writtenArgument)
							{
								RN_ASSERT(usesSameTextureDescriptor(writtenArgument, argument), "Sampled texture binding %u is used for incompatible texture arguments '%s' and '%s'", argument->GetIndex(), writtenArgument->GetName()->GetUTF8String(), argument->GetName()->GetUTF8String());
								return;
							}

							VkImageView imageView = VK_NULL_HANDLE;
							switch(argument->GetSource())
							{
								case Shader::ArgumentTexture::Source::Frame:
								{
									Texture *globalTexture = submission.renderFrame.GetGlobalTexture(argument->GetNameHash());
									VulkanTexture *vulkanTexture = globalTexture ? globalTexture->Downcast<VulkanTexture>() : nullptr;
									RN_DEBUG_ASSERT(vulkanTexture, "Missing frame global texture '%s' at texture binding %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
									if(!vulkanTexture)
									{
										vulkanTexture = _fallbackGlobalTexture->Downcast<VulkanTexture>();
									}

									imageView = vulkanTexture->_imageView;
									trackShaderTextureUsage(argument, globalTexture, vulkanTexture);
									break;
								}

								case Shader::ArgumentTexture::Source::Pass:
								{
									Texture *passTexture = framePass.GetPassResourceTexture(argument->GetNameHash());
									VulkanTexture *vulkanTexture = passTexture ? passTexture->Downcast<VulkanTexture>() : nullptr;
									RN_DEBUG_ASSERT(vulkanTexture, "Missing pass resource texture '%s' at texture binding %u", argument->GetName()->GetUTF8String(), argument->GetIndex());
									if(!vulkanTexture)
									{
										vulkanTexture = _fallbackGlobalTexture->Downcast<VulkanTexture>();
									}

									imageView = vulkanTexture->_imageView;
									trackShaderTextureUsage(argument, passTexture, vulkanTexture);
									break;
								}

								case Shader::ArgumentTexture::Source::Framebuffer:
								{
									if(!previousPassColorView)
									{
										return;
									}
									imageView = previousPassColorView;
									break;
								}

								case Shader::ArgumentTexture::Source::Material:
								{
									uint8 materialTextureIndex = argument->GetMaterialTextureIndex();
									if(!textures || materialTextureIndex >= textures->GetCount())
									{
										stop = true;
										return;
									}

									Object *textureObject = textures->GetObjectAtIndex(materialTextureIndex);

									VulkanTexture *vulkanTexture = nullptr;
									if(textureObject->IsKindOfClass(VulkanTexture::GetMetaClass()))
									{
										vulkanTexture = static_cast<VulkanTexture *>(textureObject);
									}
									else
									{
										VulkanFramebuffer *framebuffer = static_cast<VulkanFramebuffer *>(textureObject);
										// Prevent binding the current framebuffer as a sampled texture
										if(rootFramebuffer && framebuffer == rootFramebuffer)
										{
											return; // skip this texture argument
										}
										size_t textureIndex = 0;
										if(framebuffer->GetSwapChain()) textureIndex = framebuffer->GetSwapChain()->GetFrameIndex();
										vulkanTexture = framebuffer->GetColorTexture(textureIndex)->Downcast<VulkanTexture>();
									}

									imageView = vulkanTexture->_imageView;
									trackShaderTextureUsage(argument, vulkanTexture, vulkanTexture);
									break;
								}

								case Shader::ArgumentTexture::Source::SubpassInput:
								{
									RN_DEBUG_ASSERT(false, "Subpass input texture argument '%s' must be bound through subpass inputs", argument->GetName()->GetUTF8String());
									return;
								}
							}

							VkDescriptorImageInfo imageBufferDescriptorInfo = {};
							imageBufferDescriptorInfo.imageView = imageView;
							imageBufferDescriptorInfo.imageLayout = argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
							imageBufferDescriptorInfoArray.push_back(imageBufferDescriptorInfo);

							VkWriteDescriptorSet writeImageDescriptorSet = {};
							writeImageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							writeImageDescriptorSet.pNext = NULL;
							writeImageDescriptorSet.dstSet = descriptorSet;
							writeImageDescriptorSet.descriptorType = argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
							writeImageDescriptorSet.dstBinding = argument->GetIndex();
							writeImageDescriptorSet.pImageInfo = &imageBufferDescriptorInfoArray[imageBufferDescriptorInfoArray.size() - 1];
							writeImageDescriptorSet.descriptorCount = 1;

							writeDescriptorSets.push_back(writeImageDescriptorSet);
							writtenTextureArguments.push_back(argument);
						});
					};

					addFragmentSubpassInputDescriptors(pipelineState->descriptor.fragmentShader);
					addShaderTextureDescriptors(pipelineState->descriptor.vertexShader);
					addShaderTextureDescriptors(pipelineState->descriptor.fragmentShader);
				}
			}
		};

		for(VulkanRenderPass &renderPass : submission.renderPasses)
		{
			RN_PROFILE_SCOPE();
			renderPass.renderTargetsUsedInShader.clear();
			renderPass.shaderWriteTexturesUsedAsSampledImages.clear();

			if(renderPass.type == VulkanRenderPass::Type::Compute)
			{
				addComputeDescriptors(renderPass);
				continue;
			}

			if(!renderPass.UsesDrawItems())
			{
				continue;
			}

			if(renderPass.subpasses.size() > 0)
			{
				for(VulkanRenderPass &subpass : renderPass.subpasses)
				{
					updateDescriptorSets(subpass, renderPass);
				}
			}
			else
			{
				updateDescriptorSets(renderPass, renderPass);
			}
		}

		if(writeDescriptorSets.size() > 0)
		{
			vk::UpdateDescriptorSets(GetVulkanDevice()->GetDevice(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void VulkanRenderer::RenderDrawable(VkCommandBuffer commandBuffer, const VulkanPreparedDrawItem &preparedDrawItem, uint32 instanceCount)
	{
		RN_PROFILE_VULKAN_SCOPE_CMD_N(_internals->tracyVulkanCtx, commandBuffer, "Draw");

		const RenderFrame::DrawItem &drawItem = *preparedDrawItem.drawItem;
		const VulkanDrawable::RenderResources &renderResource = *preparedDrawItem.renderResources;
		const VulkanPipelineState *pipelineState = renderResource.pipelineState;
		const VulkanUniformState *uniformState = renderResource.uniformState;
		const VulkanRootSignature *rootSignature = pipelineState->rootSignature;
		const Mesh::DrawSnapshot &mesh = drawItem.GetMesh();
		const Mesh::BufferSnapshot &meshBuffers = drawItem.GetMeshBuffers();
		const Drawable::IndirectDrawSnapshot &indirectDrawSnapshot = drawItem.GetIndirectDrawSnapshot();
		const bool hasIndirectDraw = indirectDrawSnapshot.IsValid();
		const bool usesIndexedDraw = hasIndirectDraw ? indirectDrawSnapshot.GetType() == Drawable::IndirectDrawType::DrawIndexed : mesh.GetIndicesCount() > 0;
		VulkanGPUBuffer *indirectBuffer = nullptr;
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
			RN_DEBUG_ASSERT(indirectCommandStride <= 0xffffffffu, "Indirect draw command stride exceeds Vulkan limits");
			RN_DEBUG_ASSERT(indirectDrawSnapshot.GetArgumentBufferOffset() + indirectCommandRange <= indirectDrawSnapshot.GetArgumentBuffer()->GetLength(), "Indirect draw argument buffer range is out of bounds");

			GPUBuffer *activeBuffer = indirectDrawSnapshot.GetArgumentBuffer()->GetActiveBuffer();
			indirectBuffer = activeBuffer ? activeBuffer->Downcast<VulkanGPUBuffer>() : nullptr;
			RN_DEBUG_ASSERT(indirectBuffer, "Indirect draw argument buffer must be a Vulkan buffer");
		}

		VkDescriptorSet descriptorSet = renderResource.descriptorSet;
		if(_internals->drawBindStateCache.pipelineLayout != rootSignature->pipelineLayout || _internals->drawBindStateCache.descriptorSet != descriptorSet)
		{
			vk::CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rootSignature->pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			_internals->drawBindStateCache.pipelineLayout = rootSignature->pipelineLayout;
			_internals->drawBindStateCache.descriptorSet = descriptorSet;
		}
		if(_internals->drawBindStateCache.pipeline != pipelineState->state)
		{
			vk::CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState->state);
			_internals->drawBindStateCache.pipeline = pipelineState->state;
		}

		VulkanGPUBuffer *buffer = static_cast<VulkanGPUBuffer *>(meshBuffers.GetVertexBuffer());
		VulkanGPUBuffer *indices = static_cast<VulkanGPUBuffer *>(meshBuffers.GetIndicesBuffer());
		VulkanGPUBuffer *instanceAttributesBuffer = uniformState->instanceAttributesBuffer ? static_cast<VulkanGPUBuffer *>(uniformState->instanceAttributesBuffer->dynamicBuffer->GetActiveGPUBuffer()) : nullptr;

		//IF positions are separated, they will be in the first part of the buffer, everything else will be bound as the second binding, per instance data if provided through attributes are bound as a third buffer
		VkDeviceSize offsets[3];
		VkBuffer vertexBuffers[3];
		int attributesBufferIndex = 0;

		offsets[attributesBufferIndex] = 0;
		vertexBuffers[attributesBufferIndex++] = buffer->GetVulkanBuffer();

		if(pipelineState->vertexAttributeBufferCount > 1)
		{
			offsets[attributesBufferIndex] = mesh.GetVertexPositionsSeparatedSize();
			vertexBuffers[attributesBufferIndex++] = buffer->GetVulkanBuffer();
		}
		if(instanceAttributesBuffer)
		{
			offsets[attributesBufferIndex] = uniformState->instanceAttributesBuffer->offset;
			vertexBuffers[attributesBufferIndex++] = instanceAttributesBuffer->GetVulkanBuffer();
		}

		bool needsVertexBufferBind = _internals->drawBindStateCache.vertexBufferCount != attributesBufferIndex;
		for(int i = 0; i < attributesBufferIndex && !needsVertexBufferBind; i++)
		{
			needsVertexBufferBind = (_internals->drawBindStateCache.vertexBuffers[i] != vertexBuffers[i] || _internals->drawBindStateCache.vertexOffsets[i] != offsets[i]);
		}
		if(needsVertexBufferBind)
		{
			vk::CmdBindVertexBuffers(commandBuffer, 0, attributesBufferIndex, vertexBuffers, offsets);
			_internals->drawBindStateCache.vertexBufferCount = attributesBufferIndex;
			for(int i = 0; i < attributesBufferIndex; i++)
			{
				_internals->drawBindStateCache.vertexBuffers[i] = vertexBuffers[i];
				_internals->drawBindStateCache.vertexOffsets[i] = offsets[i];
			}
		}
		if(usesIndexedDraw)
		{
			VkBuffer indexBuffer = indices->GetVulkanBuffer();
			VkIndexType indexType = mesh.GetIndexType() == PrimitiveType::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
			if(!_internals->drawBindStateCache.hasIndexBufferBinding || _internals->drawBindStateCache.indexBuffer != indexBuffer || _internals->drawBindStateCache.indexOffset != 0 || _internals->drawBindStateCache.indexType != indexType)
			{
				// Bind mesh index buffer
				vk::CmdBindIndexBuffer(commandBuffer, indexBuffer, 0, indexType);
				_internals->drawBindStateCache.hasIndexBufferBinding = true;
				_internals->drawBindStateCache.indexBuffer = indexBuffer;
				_internals->drawBindStateCache.indexOffset = 0;
				_internals->drawBindStateCache.indexType = indexType;
			}

			if(hasIndirectDraw)
			{
				if(indirectBuffer)
				{
					const uint32 indirectDrawCount = indirectDrawSnapshot.GetDrawCount();
					const uint32 indirectStride = static_cast<uint32>(indirectCommandStride);
					if(indirectDrawCount == 1 || GetVulkanDevice()->GetSupportsMultiDrawIndirect())
					{
						vk::CmdDrawIndexedIndirect(commandBuffer, indirectBuffer->GetVulkanBuffer(), indirectDrawSnapshot.GetArgumentBufferOffset(), indirectDrawCount, indirectStride);
					}
					else
					{
						for(uint32 drawIndex = 0; drawIndex < indirectDrawCount; drawIndex++)
						{
							vk::CmdDrawIndexedIndirect(commandBuffer, indirectBuffer->GetVulkanBuffer(), indirectDrawSnapshot.GetArgumentBufferOffset() + indirectCommandStride * drawIndex, 1, indirectStride);
						}
					}
				}
			}
			else
			{
				// Render mesh vertex buffer using it's indices
				vk::CmdDrawIndexed(commandBuffer, mesh.GetIndicesCount(), instanceCount, 0, 0, 0);
			}
		}
		else
		{
			if(hasIndirectDraw)
			{
				if(indirectBuffer)
				{
					const uint32 indirectDrawCount = indirectDrawSnapshot.GetDrawCount();
					const uint32 indirectStride = static_cast<uint32>(indirectCommandStride);
					if(indirectDrawCount == 1 || GetVulkanDevice()->GetSupportsMultiDrawIndirect())
					{
						vk::CmdDrawIndirect(commandBuffer, indirectBuffer->GetVulkanBuffer(), indirectDrawSnapshot.GetArgumentBufferOffset(), indirectDrawCount, indirectStride);
					}
					else
					{
						for(uint32 drawIndex = 0; drawIndex < indirectDrawCount; drawIndex++)
						{
							vk::CmdDrawIndirect(commandBuffer, indirectBuffer->GetVulkanBuffer(), indirectDrawSnapshot.GetArgumentBufferOffset() + indirectCommandStride * drawIndex, 1, indirectStride);
						}
					}
				}
			}
			else
			{
				vk::CmdDraw(commandBuffer, mesh.GetVerticesCount(), instanceCount, 0, 0);
			}
		}
	}

	void VulkanRenderer::RenderComputePass(VulkanCommandBuffer *commandList, const VulkanFrameSubmission &submission, const VulkanRenderPass &computePass)
	{
		RN_PROFILE_VULKAN_SCOPE_CMD_N(_internals->tracyVulkanCtx, commandList->GetCommandBuffer(), "Compute");

		RN_DEBUG_ASSERT(computePass.computePipelineState, "Missing compute pipeline state");
		if(!computePass.computePipelineState) return;

		VkCommandBuffer commandBuffer = commandList->GetCommandBuffer();
		const VulkanComputePipelineState *pipelineState = computePass.computePipelineState;
		const VulkanRootSignature *rootSignature = pipelineState->rootSignature;
		const VkDescriptorSet descriptorSet = computePass.computeDescriptorSet;

		Shader *computeShader = computePass.computeDispatch.GetShader();
		if(computeShader && computeShader->GetSignature())
		{
			computeShader->GetSignature()->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop) {
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
							texture = computePass.previousStoredFramebuffer->GetColorTexture(0);
						break;

					case Shader::ArgumentTexture::Source::Material:
					case Shader::ArgumentTexture::Source::Pass:
					case Shader::ArgumentTexture::Source::SubpassInput:
						texture = computePass.computeDispatch.GetResourceTexture(argument->GetNameHash());
						break;
				}

				VulkanTexture *vulkanTexture = texture ? texture->Downcast<VulkanTexture>() : nullptr;
				if(vulkanTexture)
				{
					vulkanTexture->TransitionToUsage(commandBuffer, argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VulkanTexture::LayoutUsage::ShaderWrite : VulkanTexture::LayoutUsage::ShaderRead);
				}
			});
		}

		vk::CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineState->state);
		vk::CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, rootSignature->pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		bool dispatched = false;
		const std::vector<ComputePass::DispatchRegion> &dispatchRegions = computePass.computeDispatch.GetDispatchRegions();
		for(const ComputePass::DispatchRegion &dispatchRegion : dispatchRegions)
		{
			const ComputePass::DispatchSize &groupCount = dispatchRegion.groupCount;
			const ComputePass::DispatchOffset &groupOffset = dispatchRegion.groupOffset;
			if(groupOffset.x > 0 || groupOffset.y > 0 || groupOffset.z > 0)
			{
				vk::CmdDispatchBase(commandBuffer, groupOffset.x, groupOffset.y, groupOffset.z, groupCount.x, groupCount.y, groupCount.z);
			}
			else
			{
				vk::CmdDispatch(commandBuffer, groupCount.x, groupCount.y, groupCount.z);
			}

			dispatched = true;
		}
		if(!dispatched) return;

		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = NULL;
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
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
		Lock();
		_internals->frameResources.push_back({ _currentFrame + frameOffset, callback });
		Unlock();
	}
}
