//
//  RNOpenXRMetalSwapChain.cpp
//  Rayne-OpenXR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenXRMetalSwapChain.h"
#include "RNOpenXRWindow.h"
#include "RNOpenXRInternals.h"

namespace RN
{
	RNDefineMeta(OpenXRMetalSwapChain, MetalSwapChain)

	OpenXRMetalSwapChain::OpenXRMetalSwapChain(const OpenXRWindow *window, OpenXRCompositorLayer *layer, const Window::SwapChainDescriptor &descriptor, const Vector2 &size) : OpenXRSwapChain(window, layer, SwapChainType::Metal), _swapchainImages(nullptr)
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
		swapchainCreateInfo.format = MTLPixelFormatRGBA8Unorm_sRGB; //TODO: Use the format from the descriptor here!
		swapchainCreateInfo.sampleCount = 1;
		swapchainCreateInfo.width = _size.x;
		swapchainCreateInfo.height = _size.y;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.arraySize = _descriptor.layerCount;
		swapchainCreateInfo.mipCount = 1;

		if(!XR_SUCCEEDED(xrCreateSwapchain(_xrWindow->_internals->session, &swapchainCreateInfo, &_internals->swapchain)))
		{
		   RN_ASSERT(false, "failed creating swapchain");
		}

		uint32 numberOfSwapChainImages = 0;
		xrEnumerateSwapchainImages(_internals->swapchain, 0, &numberOfSwapChainImages, nullptr);

		XrSwapchainImageMetalKHR *swapchainImages = new XrSwapchainImageMetalKHR[numberOfSwapChainImages];
		_swapchainImages = new void*[numberOfSwapChainImages];

		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			swapchainImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
			swapchainImages[i].next = nullptr;
		}
		xrEnumerateSwapchainImages(_internals->swapchain, numberOfSwapChainImages, &numberOfSwapChainImages, (XrSwapchainImageBaseHeader*)swapchainImages);
		_descriptor.bufferCount = numberOfSwapChainImages;

		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			_swapchainImages[i] = swapchainImages[i].texture;
		}
		delete[] swapchainImages;

		/*for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore = VK_NULL_HANDLE;
			VkSemaphore renderSemaphore = VK_NULL_HANDLE;
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}*/

		MetalRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<MetalRenderer>();
		_framebuffer = new MetalFramebuffer(_size, this, _descriptor.colorFormat, _descriptor.depthStencilFormat);
	}

	OpenXRMetalSwapChain::~OpenXRMetalSwapChain()
	{
		_framebuffer->Release();
		xrDestroySwapchain(_internals->swapchain);
		delete[] _swapchainImages;
	}

	void OpenXRMetalSwapChain::AcquireBackBuffer()
	{
		if(!_isActive) return;

		_layer->UpdateForCurrentFrame(_xrWindow);

        XrSwapchainImageAcquireInfo swapchainImageAcquireInfo;
        swapchainImageAcquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        swapchainImageAcquireInfo.next = nullptr;

        uint32_t imageIndex = 0;
        xrAcquireSwapchainImage(_internals->swapchain, &swapchainImageAcquireInfo, &imageIndex);
        //_semaphoreIndex = _frameIndex = imageIndex;

		XrSwapchainImageWaitInfo swapchainImageWaitInfo;
		swapchainImageWaitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
		swapchainImageWaitInfo.next = nullptr;
		swapchainImageWaitInfo.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(_internals->swapchain, &swapchainImageWaitInfo);
	}

    void OpenXRMetalSwapChain::Prepare()
	{

	}

    void OpenXRMetalSwapChain::Finalize()
	{

	}

    void OpenXRMetalSwapChain::PresentBackBuffer(id<MTLCommandBuffer> commandBuffer)
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

    id OpenXRMetalSwapChain::GetMetalColorTexture() const
	{
		return (id<MTLTexture>)_swapchainImages[0];
	}

	id OpenXRMetalSwapChain::GetMetalDepthTexture() const
	{
		return nullptr;
	}

	void OpenXRMetalSwapChain::SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic)
	{
		
	}

	void OpenXRMetalSwapChain::ResizeSwapChain(const Vector2& size)
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

	Framebuffer *OpenXRMetalSwapChain::GetSwapChainFramebuffer() const
	{
		return GetFramebuffer();
	}
}
