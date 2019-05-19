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

		_size = size;

		ovrTextureFormat textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
/*		switch(descriptor.colorFormat)
		{
			case Texture::Format::BGRA8888SRGB:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
				break;
			}
			case Texture::Format::BGRA8888:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888;
				break;
			}
			case Texture::Format::RGBA8888SRGB:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
				break;
			}
			case Texture::Format::RGBA8888:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888;
				break;
			}
			default:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
				break;
			}
		}*/

		_descriptor.bufferCount = 3;
		_colorSwapChain = vrapi_CreateTextureSwapChain2(VRAPI_TEXTURE_TYPE_2D, textureFormat, _size.x, _size.y, 1, _descriptor.bufferCount);
		_descriptor.bufferCount = vrapi_GetTextureSwapChainLength(_colorSwapChain);

		for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore = VK_NULL_HANDLE;
			VkSemaphore renderSemaphore = VK_NULL_HANDLE;
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		_framebuffer = new VulkanFramebuffer(_size, this, renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);
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
    	float eyeOffset = 0.0f;
    	float eyeFactor = 1.0f;

    	const ovrMatrix4f tanAngleMatrix =
    	{ {
    		{ eyeFactor * 0.5f * projection->M[0][0], 0.0f, 0.5f * projection->M[0][2] - 0.5f, 0.0f },
    		{ 0.0f, -0.5f * projection->M[1][1], 0.5f * projection->M[1][2] - 0.5f, 0.0f },
    		{ 0.0f, 0.0f, -1.0f, 0.0f },
    		// Store the values to convert a clip-Z to a linear depth in the unused matrix elements.
    		{ projection->M[2][2], projection->M[2][3], projection->M[3][2], 1.0f }
    	} };
    	return tanAngleMatrix;
    }
}
