//
//  RNOculusMobileVulkanSwapChain.cpp
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusMobileVulkanSwapChain.h"
#include "RNVulkanInternals.h"

#include "VrApi_Vulkan.h"
#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"

namespace RN
{
	RNDefineMeta(OculusMobileVulkanSwapChain, VulkanSwapChain)

	OculusMobileVulkanSwapChain::OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor, const Vector2 &size)
	{
		_descriptor = descriptor;
		_descriptor.depthStencilFormat = Texture::Format::Invalid;
		_descriptor.colorFormat = Texture::Format::RGBA_8_SRGB;
		_descriptor.layerCount = 2;

		_size = size;

		ovrSwapChainCreateInfo swapChainCreateInfo;
		swapChainCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;

		swapChainCreateInfo.Width = _size.x;
		swapChainCreateInfo.Height = _size.y;
		swapChainCreateInfo.Levels = 1;
		swapChainCreateInfo.FaceCount = 1;
		swapChainCreateInfo.ArraySize = _descriptor.layerCount;
		swapChainCreateInfo.BufferCount = 3;
		swapChainCreateInfo.CreateFlags = 0;
		swapChainCreateInfo.UsageFlags = VRAPI_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

		_colorSwapChain = vrapi_CreateTextureSwapChain4(&swapChainCreateInfo);
		_descriptor.bufferCount = vrapi_GetTextureSwapChainLength(_colorSwapChain);

		for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore = VK_NULL_HANDLE;
			VkSemaphore renderSemaphore = VK_NULL_HANDLE;
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		_framebuffer = new VulkanFramebuffer(_size, _descriptor.layerCount, this, renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);
	}

	OculusMobileVulkanSwapChain::~OculusMobileVulkanSwapChain()
	{
		if(_colorSwapChain)
		{
			vrapi_DestroyTextureSwapChain(_colorSwapChain);
		}
	}

	void OculusMobileVulkanSwapChain::AcquireBackBuffer()
	{
		_semaphoreIndex += 1;
		_frameIndex = _semaphoreIndex %= _descriptor.bufferCount;
	}

    void OculusMobileVulkanSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{

	}

    void OculusMobileVulkanSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{

	}

    void OculusMobileVulkanSwapChain::PresentBackBuffer(VkQueue queue)
	{
		if(_presentEvent)
		{
			_presentEvent();
		}
	}

    VkImage OculusMobileVulkanSwapChain::GetVulkanColorBuffer(int i) const
	{
		return vrapi_GetTextureSwapChainBufferVulkan(_colorSwapChain, i);
	}

    VkImage OculusMobileVulkanSwapChain::GetVulkanDepthBuffer(int i) const
	{
		return nullptr;
	}

	ovrMatrix4f OculusMobileVulkanSwapChain::GetTanAngleMatrixForEye(uint8 eye)
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
    }
}
