//
//  RNOpenXRVulkanSwapChain.cpp
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenXRVulkanSwapChain.h"
#include "RNOpenXRWindow.h"
#include "RNOpenXRInternals.h"

namespace RN
{
	RNDefineMeta(OpenXRVulkanSwapChain, VulkanSwapChain)

	OpenXRVulkanSwapChain::OpenXRVulkanSwapChain(const OpenXRWindow *window, const Window::SwapChainDescriptor &descriptor, const Vector2 &size, bool supportFoveation) : OpenXRSwapChain(window, SwapChainType::Vulkan), _swapchainImages(nullptr), _swapchainFoveationImages(nullptr), _swapChainFoveationImagesSize(nullptr)
	{
		_descriptor = descriptor;
		_descriptor.depthStencilFormat = Texture::Format::Invalid;
		_descriptor.colorFormat = Texture::Format::RGBA_8_SRGB; //TODO: Don´t hardcode the format here?

		_size = size;

		XrSwapchainCreateInfo swapchainCreateInfo;
		swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		swapchainCreateInfo.next = nullptr;
		swapchainCreateInfo.createFlags = 0;
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB; //TODO: Use the format from the descriptor here!
		swapchainCreateInfo.sampleCount = 1;
		swapchainCreateInfo.width = _size.x;
		swapchainCreateInfo.height = _size.y;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.arraySize = _descriptor.layerCount;
		swapchainCreateInfo.mipCount = 1;

		//Enable foveated rendering
		XrSwapchainCreateInfoFoveationFB foveationSwapChainCreateInfo;
		foveationSwapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB;
		foveationSwapChainCreateInfo.next = nullptr;
		foveationSwapChainCreateInfo.flags = XR_SWAPCHAIN_CREATE_FOVEATION_FRAGMENT_DENSITY_MAP_BIT_FB;
		if(_xrWindow->_supportsFoveatedRendering && supportFoveation)
		{
			RNDebug("Foveated Rendering is enabled.");
			swapchainCreateInfo.next = &foveationSwapChainCreateInfo;
		}

		if(!XR_SUCCEEDED(xrCreateSwapchain(_xrWindow->_internals->session, &swapchainCreateInfo, &_internals->swapchain)))
		{
		   RN_ASSERT(false, "failed creating swapchain");
		}

		uint32 numberOfSwapChainImages = 0;
		xrEnumerateSwapchainImages(_internals->swapchain, 0, &numberOfSwapChainImages, nullptr);

		XrSwapchainImageVulkanKHR *swapchainImages = new XrSwapchainImageVulkanKHR[numberOfSwapChainImages];
		_swapchainImages = new VkImage[numberOfSwapChainImages];

		XrSwapchainImageFoveationVulkanFB *swapchainFoveationImages = nullptr;
		if(_xrWindow->_supportsFoveatedRendering && supportFoveation)
		{
			swapchainFoveationImages = new XrSwapchainImageFoveationVulkanFB[numberOfSwapChainImages];
			_swapchainFoveationImages = new VkImage[numberOfSwapChainImages];
			_swapChainFoveationImagesSize = new Vector2[numberOfSwapChainImages];
		}
		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			swapchainImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
			swapchainImages[i].next = nullptr;

			if(swapchainFoveationImages)
			{
				swapchainFoveationImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_FOVEATION_VULKAN_FB;
				swapchainFoveationImages[i].next = nullptr;

				swapchainImages[i].next = &swapchainFoveationImages[i];
			}
		}
		xrEnumerateSwapchainImages(_internals->swapchain, numberOfSwapChainImages, &numberOfSwapChainImages, (XrSwapchainImageBaseHeader*)swapchainImages);
		_descriptor.bufferCount = numberOfSwapChainImages;

		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			_swapchainImages[i] = swapchainImages[i].image;

			if(swapchainFoveationImages)
			{
				_swapchainFoveationImages[i] = swapchainFoveationImages[i].image;
				_swapChainFoveationImagesSize[i].x = swapchainFoveationImages[i].width;
				_swapChainFoveationImagesSize[i].y = swapchainFoveationImages[i].height;
			}
		}
		delete[] swapchainImages;

		if(swapchainFoveationImages)
		{
			delete[] swapchainFoveationImages;
		}

		for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore = VK_NULL_HANDLE;
			VkSemaphore renderSemaphore = VK_NULL_HANDLE;
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		_framebuffer = new VulkanFramebuffer(_size, _descriptor.layerCount, this, renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat, _swapchainFoveationImages? Texture::Format::RG_8 : Texture::Format::Invalid);
	}

	OpenXRVulkanSwapChain::~OpenXRVulkanSwapChain()
	{
		_framebuffer->Release();
		xrDestroySwapchain(_internals->swapchain);
		delete[] _swapchainImages;
		if(_swapchainFoveationImages)
		{
			delete[] _swapchainFoveationImages;
			delete[] _swapChainFoveationImagesSize;
		}
		if(_internals->currentFoveationProfile != XR_NULL_HANDLE)
		{
			_xrWindow->_internals->DestroyFoveationProfileFB(_internals->currentFoveationProfile);
		}
	}

	void OpenXRVulkanSwapChain::AcquireBackBuffer()
	{
		if(!_isActive) return;

		if(!_presentEvent)
		{
			RNDebug("Acquire");
		}

        XrSwapchainImageAcquireInfo swapchainImageAcquireInfo;
        swapchainImageAcquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        swapchainImageAcquireInfo.next = nullptr;

        uint32_t imageIndex = 0;
        xrAcquireSwapchainImage(_internals->swapchain, &swapchainImageAcquireInfo, &imageIndex);
        _semaphoreIndex = _frameIndex = imageIndex;

		XrSwapchainImageWaitInfo swapchainImageWaitInfo;
		swapchainImageWaitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
		swapchainImageWaitInfo.next = nullptr;
		swapchainImageWaitInfo.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(_internals->swapchain, &swapchainImageWaitInfo);
	}

    void OpenXRVulkanSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{

	}

    void OpenXRVulkanSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{

	}

    void OpenXRVulkanSwapChain::PresentBackBuffer(VkQueue queue)
	{
		if(!_isActive) return;

        XrSwapchainImageReleaseInfo swapchainImageReleaseInfo;
        swapchainImageReleaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
        swapchainImageReleaseInfo.next = nullptr;
        xrReleaseSwapchainImage(_internals->swapchain, &swapchainImageReleaseInfo);

		_hasContent = true;

		if(_presentEvent)
		{
			_presentEvent();
		}
	}

    VkImage OpenXRVulkanSwapChain::GetVulkanColorBuffer(int i) const
	{
		return _swapchainImages[i];
	}

    VkImage OpenXRVulkanSwapChain::GetVulkanDepthBuffer(int i) const
	{
		return nullptr;
	}

	VkImage OpenXRVulkanSwapChain::GetVulkanFragmentDensityBuffer(int i, uint32 &width, uint32 &height) const
	{
		if(!_swapchainFoveationImages) return nullptr;

		width = _swapChainFoveationImagesSize[i].x;
		height = _swapChainFoveationImagesSize[i].y;
		return _swapchainFoveationImages[i];
	}

	void OpenXRVulkanSwapChain::SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic)
	{
		if(!_swapchainFoveationImages) return;

		if(_internals->currentFoveationProfile != XR_NULL_HANDLE)
		{
			_xrWindow->_internals->DestroyFoveationProfileFB(_internals->currentFoveationProfile);
		}

		XrFoveationLevelProfileCreateInfoFB foveationLevelProfileCreateInfo;
		foveationLevelProfileCreateInfo.type = XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB;
		foveationLevelProfileCreateInfo.next = nullptr;
		foveationLevelProfileCreateInfo.level = (XrFoveationLevelFB)level;
		foveationLevelProfileCreateInfo.dynamic = dynamic? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB : XR_FOVEATION_DYNAMIC_DISABLED_FB;
		foveationLevelProfileCreateInfo.verticalOffset = 0.0f;

		XrFoveationProfileCreateInfoFB foveationProfileCreateInfo;
		foveationProfileCreateInfo.type = XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB;
		foveationProfileCreateInfo.next = &foveationLevelProfileCreateInfo;
		_xrWindow->_internals->CreateFoveationProfileFB(_xrWindow->_internals->session, &foveationProfileCreateInfo, &_internals->currentFoveationProfile);

		XrSwapchainStateFoveationFB swapchainStateFoveation;
		swapchainStateFoveation.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
		swapchainStateFoveation.next = nullptr;
		swapchainStateFoveation.flags = 0;
		swapchainStateFoveation.profile = _internals->currentFoveationProfile;
		_xrWindow->_internals->UpdateSwapchainFB(_internals->swapchain, (XrSwapchainStateBaseHeaderFB *) & swapchainStateFoveation);
	}

	void OpenXRVulkanSwapChain::ResizeSwapChain(const Vector2& size)
	{
		_size = size;
		//_framebuffer->WillUpdateSwapChain(); //As all it does is free the swap chain d3d buffer resources, it would free the targetTexture resource and should't be called in this case...
/*		SafeRelease(_targetTexture);
		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(_descriptor.colorFormat, _size.x, _size.y, false);
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		textureDescriptor.depth = _descriptor.layerCount;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);

		textureDescriptor.depth = 1;
		_outputTexture[0] = _renderer->CreateTextureWithDescriptor(textureDescriptor);
		_outputTexture[1] = _renderer->CreateTextureWithDescriptor(textureDescriptor);

		_framebuffer->DidUpdateSwapChain(_size, _descriptor.layerCount, _descriptor.colorFormat, _descriptor.depthStencilFormat);*/
	}

	Framebuffer *OpenXRVulkanSwapChain::GetSwapChainFramebuffer() const
	{
		return GetFramebuffer();
	}
}
