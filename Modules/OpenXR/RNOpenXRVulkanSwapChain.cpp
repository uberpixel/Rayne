//
//  RNOpenXRVulkanSwapChain.cpp
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenXRVulkanSwapChain.h"
#include "RNOpenXRWindow.h"
#include "RNVulkanInternals.h"
#include "RNOpenXRInternals.h"

namespace RN
{
	RNDefineMeta(OpenXRVulkanSwapChain, VulkanSwapChain)

	OpenXRVulkanSwapChain::OpenXRVulkanSwapChain(const OpenXRWindow *window, const Window::SwapChainDescriptor &descriptor, const Vector2 &size) : _window(window), _internals(new OpenXRSwapchainInternals())
	{
		_descriptor = descriptor;
		_descriptor.depthStencilFormat = Texture::Format::Invalid;
		_descriptor.colorFormat = Texture::Format::RGBA_8_SRGB;
		_descriptor.layerCount = 2;

		_size = size;

		uint32 numberOfSupportedSwapChainFormats = 0;
		xrEnumerateSwapchainFormats(window->_internals->session, 0, &numberOfSupportedSwapChainFormats, nullptr);

		int64_t *supportedSwapChainFormats = new int64_t[numberOfSupportedSwapChainFormats];
		xrEnumerateSwapchainFormats(window->_internals->session, numberOfSupportedSwapChainFormats, &numberOfSupportedSwapChainFormats, supportedSwapChainFormats);

		for(int i = 0; i < numberOfSupportedSwapChainFormats; i++)
		{
			RNDebug("Supported swap chain format: " << supportedSwapChainFormats[i]);
		}

		delete[] supportedSwapChainFormats;

        XrSwapchainCreateInfo swapchainCreateInfo;
        swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapchainCreateInfo.next = nullptr;
        swapchainCreateInfo.createFlags = 0;
        swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        swapchainCreateInfo.sampleCount = 1;
        swapchainCreateInfo.width = _size.x;
        swapchainCreateInfo.height = _size.y;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.arraySize = _descriptor.layerCount;
        swapchainCreateInfo.mipCount = 1;

        if(!XR_SUCCEEDED(xrCreateSwapchain(_window->_internals->session, &swapchainCreateInfo, &_internals->swapchain)))
        {
            RN_ASSERT(false, "failed creating swapchain");
        }

		uint32 numberOfSwapChainImages = 0;
		xrEnumerateSwapchainImages(_internals->swapchain, 0, &numberOfSwapChainImages, nullptr);

		XrSwapchainImageVulkanKHR *swapchainImages = new XrSwapchainImageVulkanKHR[numberOfSwapChainImages];
		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			swapchainImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
			swapchainImages[i].next = nullptr;
		}
		xrEnumerateSwapchainImages(_internals->swapchain, numberOfSwapChainImages, &numberOfSwapChainImages, (XrSwapchainImageBaseHeader*)swapchainImages);
		_descriptor.bufferCount = numberOfSwapChainImages;

		_swapchainImages = new VkImage[numberOfSwapChainImages];
		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			_swapchainImages[i] = swapchainImages[i].image;
		}
		delete[] swapchainImages;

		for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore = VK_NULL_HANDLE;
			VkSemaphore renderSemaphore = VK_NULL_HANDLE;
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		_framebuffer = new VulkanFramebuffer(_size, _descriptor.layerCount, this, renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat, Texture::Format::Invalid);
	}

	OpenXRVulkanSwapChain::~OpenXRVulkanSwapChain()
	{
		xrDestroySwapchain(_internals->swapchain);
		delete[] _swapchainImages;
		delete _internals;
	}

	void OpenXRVulkanSwapChain::AcquireBackBuffer()
	{
        XrSwapchainImageAcquireInfo swapchainImageAcquireInfo;
        swapchainImageAcquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        swapchainImageAcquireInfo.next = nullptr;

        uint32_t imageIndex = 0;
        xrAcquireSwapchainImage(_internals->swapchain, &swapchainImageAcquireInfo, &imageIndex);
        _semaphoreIndex = _frameIndex = imageIndex;

		XrSwapchainImageWaitInfo swapchainImageWaitInfo;
		swapchainImageWaitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
		swapchainImageWaitInfo.next = nullptr;
		swapchainImageWaitInfo.timeout = 100000;
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
        XrSwapchainImageReleaseInfo swapchainImageReleaseInfo;
        swapchainImageReleaseInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
        swapchainImageReleaseInfo.next = nullptr;
        xrReleaseSwapchainImage(_internals->swapchain, &swapchainImageReleaseInfo);

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
		//VkImage image;
		//vrapi_GetTextureSwapChainBufferFoveationVulkan(_colorSwapChain, i, &image, &width, &height);
		return nullptr;//image;
	}

/*	ovrMatrix4f OpenXRVulkanSwapChain::GetTanAngleMatrixForEye(uint8 eye)
    {
    	ovrMatrix4f *projection = &_hmdState.Eye[eye].ProjectionMatrix;

    	const ovrMatrix4f tanAngleMatrix =
    	{ {
    		{ 0.5f * projection->M[0][0], 0.0f, 0.5f * projection->M[0][2] - 0.5f, 0.0f },
    		{ 0.0f, -0.5f * projection->M[1][1], -0.5f * projection->M[1][2] - 0.5f, 0.0f },
    		{ 0.0f, 0.0f, -1.0f, 0.0f },
    		// Store the values to convert a clip-Z to a linear depth in the unused matrix elements.
    		{ projection->M[2][2], projection->M[2][3], projection->M[3][2], 1.0f }
    	} };

    	return tanAngleMatrix;
    }*/
}
