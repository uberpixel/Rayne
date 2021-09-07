//
//  RNOpenXRD3D12SwapChain.cpp
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNOpenXRD3D12SwapChain.h"

#include "RNOpenXRWindow.h"
#include "RNOpenXRInternals.h"
#include "RND3D12Internals.h"
#include "RND3D12Renderer.h"
#include "RND3D12Framebuffer.h"

namespace RN
{
	RNDefineMeta(OpenXRD3D12SwapChain, D3D12SwapChain)

	OpenXRD3D12SwapChain::OpenXRD3D12SwapChain(const OpenXRWindow *window, const Window::SwapChainDescriptor &descriptor, const Vector2 &size) : OpenXRSwapChain(window)
	{
		_renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		_size = size;
		
		_descriptor = descriptor;
		_descriptor.colorFormat = Texture::Format::RGBA_8_SRGB; //TODO: Don´t hardcode the format here?
		_descriptor.layerCount = 2;

		uint32 numberOfSupportedSwapChainFormats = 0;
		xrEnumerateSwapchainFormats(window->_internals->session, 0, &numberOfSupportedSwapChainFormats, nullptr);

		int64_t *supportedSwapChainFormats = new int64_t[numberOfSupportedSwapChainFormats];
		xrEnumerateSwapchainFormats(window->_internals->session, numberOfSupportedSwapChainFormats, &numberOfSupportedSwapChainFormats, supportedSwapChainFormats);

		for(int i = 0; i < numberOfSupportedSwapChainFormats; i++)
		{
			//TODO: Check if the requested swapchain format is actually supported
			RNDebug("Supported swap chain format: " << supportedSwapChainFormats[i]);
		}

		delete[] supportedSwapChainFormats;

		XrSwapchainCreateInfo swapchainCreateInfo;
		swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		swapchainCreateInfo.next = nullptr;
		swapchainCreateInfo.createFlags = 0;
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //TODO: Use the format from the descriptor here!
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

		XrSwapchainImageD3D12KHR *swapchainImages = new XrSwapchainImageD3D12KHR[numberOfSwapChainImages];
		_swapchainImages = new ID3D12Resource*[numberOfSwapChainImages];

		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			swapchainImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
			swapchainImages[i].next = nullptr;
		}
		xrEnumerateSwapchainImages(_internals->swapchain, numberOfSwapChainImages, &numberOfSwapChainImages, (XrSwapchainImageBaseHeader*)swapchainImages);
		_descriptor.bufferCount = numberOfSwapChainImages;

		for(int i = 0; i < numberOfSwapChainImages; i++)
		{
			_swapchainImages[i] = swapchainImages[i].texture;
		}
		delete[] swapchainImages;

		_framebuffer = new D3D12Framebuffer(_size, _descriptor.layerCount, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);

	}

	OpenXRD3D12SwapChain::~OpenXRD3D12SwapChain()
	{
		_framebuffer->Release();
		xrDestroySwapchain(_internals->swapchain);
		delete[] _swapchainImages;
	}

	void OpenXRD3D12SwapChain::ResizeSwapChain(const Vector2& size)
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


	void OpenXRD3D12SwapChain::AcquireBackBuffer()
	{
		XrSwapchainImageAcquireInfo swapchainImageAcquireInfo;
		swapchainImageAcquireInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
		swapchainImageAcquireInfo.next = nullptr;

		uint32_t imageIndex = 0;
		xrAcquireSwapchainImage(_internals->swapchain, &swapchainImageAcquireInfo, &imageIndex);
		_frameIndex = imageIndex;

		XrSwapchainImageWaitInfo swapchainImageWaitInfo;
		swapchainImageWaitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
		swapchainImageWaitInfo.next = nullptr;
		swapchainImageWaitInfo.timeout = 100000000000; //100s //TODO: Handle timeouts (release later will cause and error and all future frames start being behind somehow)
		xrWaitSwapchainImage(_internals->swapchain, &swapchainImageWaitInfo);
	}

	void OpenXRD3D12SwapChain::Prepare(D3D12CommandList *commandList)
	{
		
	}

	void OpenXRD3D12SwapChain::Finalize(D3D12CommandList *commandList)
	{

	}

	void OpenXRD3D12SwapChain::PresentBackBuffer()
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

	ID3D12Resource *OpenXRD3D12SwapChain::GetD3D12ColorBuffer(int i) const
	{
		return _swapchainImages[i];
	}

	Framebuffer *OpenXRD3D12SwapChain::GetSwapChainFramebuffer() const
	{
		return GetFramebuffer();
	}
}
