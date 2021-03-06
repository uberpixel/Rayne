//
//  RNOpenVRVulkanSwapChain.cpp
//  Rayne-OpenVR
//
//  Copyright 201 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenVRVulkanSwapChain.h"

#include "RNVulkanInternals.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanTexture.h"

namespace RN
{
	RNDefineMeta(OpenVRVulkanSwapChain, VulkanSwapChain)

	OpenVRVulkanSwapChain::OpenVRVulkanSwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system) : OpenVRSwapChain(system), _isFirstRender(true)
	{
		vr::VRCompositor()->SetExplicitTimingMode(vr::VRCompositorTimingMode_Explicit_ApplicationPerformsPostPresentHandoff);

		_renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		_descriptor = descriptor;
		_descriptor.colorFormat = Texture::Format::RGBA_8_SRGB;
		_descriptor.layerCount = 2;

		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_vrSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		_size = Vector2(recommendedWidth, recommendedHeight);

		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(_descriptor.colorFormat, _size.x, _size.y, false);
		textureDescriptor.type = Texture::Type::Type2DArray;
		textureDescriptor.depth = 2;
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);

		_descriptor.bufferCount = 1;
		_frameIndex = 0;
		_framebuffer = new VulkanFramebuffer(_size, _descriptor.layerCount, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat, Texture::Format::Invalid);

		//TODO: Update every frame, maybe move to window
		vr::HmdMatrix34_t leftEyeMatrix = _vrSystem->GetEyeToHeadTransform(vr::Eye_Left);
		vr::HmdMatrix34_t rightEyeMatrix = _vrSystem->GetEyeToHeadTransform(vr::Eye_Right);
		_hmdToEyeViewOffset[0].x = leftEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[0].y = leftEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[0].z = leftEyeMatrix.m[2][3];
		_hmdToEyeViewOffset[1].x = rightEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[1].y = rightEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[1].z = rightEyeMatrix.m[2][3];

		for (size_t i = _presentSemaphores.size(); i < _descriptor.bufferCount; i++)
		{
			_presentSemaphores.push_back(VK_NULL_HANDLE);
			_renderSemaphores.push_back(VK_NULL_HANDLE);
		}
	}

	OpenVRVulkanSwapChain::~OpenVRVulkanSwapChain()
	{
		SafeRelease(_targetTexture);
	}

	void OpenVRVulkanSwapChain::ResizeOpenVRSwapChain(const Vector2& size)
	{
		_size = size;
		//_framebuffer->WillUpdateSwapChain(); //As all it does is free the swap chain d3d buffer resources, it would free the targetTexture resource and should't be called in this case...
		SafeRelease(_targetTexture);
		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(_descriptor.colorFormat, _size.x, _size.y, false);
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);
		_framebuffer->DidUpdateSwapChain(_size, 2, _descriptor.colorFormat, _descriptor.depthStencilFormat, Texture::Format::Invalid);
		_isFirstRender = true;
	}


	void OpenVRVulkanSwapChain::AcquireBackBuffer()
	{
		
	}

	void OpenVRVulkanSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{
		if (_isFirstRender)
			return;

		VulkanTexture *texture = _targetTexture->Downcast<VulkanTexture>();
		VulkanTexture::SetImageLayout(commandBuffer, texture->GetVulkanImage(), 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, texture->GetCurrentLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VulkanTexture::BarrierIntent::RenderTarget);
		texture->SetCurrentLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		//vr::VRCompositor()->SubmitExplicitTimingData(); //Not sure where to do this, doing it here (or in finalize) causes massive stuttering issues. without, it falls back to doing this in waitgetposes which works good enough
	}

	void OpenVRVulkanSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{
		VulkanTexture *texture = _targetTexture->Downcast<VulkanTexture>();
		VulkanTexture::SetImageLayout(commandBuffer, texture->GetVulkanImage(), 0, 1, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, texture->GetCurrentLayout(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VulkanTexture::BarrierIntent::ExternalSource);
		texture->SetCurrentLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		_isFirstRender = false;
	}

	void OpenVRVulkanSwapChain::PresentBackBuffer(VkQueue queue)
	{
		VulkanTexture *texture = _targetTexture->Downcast<VulkanTexture>();

		//vr::VRVulkanTextureData_t vulkanEyeTexture;
		vr::VRVulkanTextureArrayData_t vulkanEyeTexture;
		vulkanEyeTexture.m_nImage = reinterpret_cast<uint64_t>(texture->GetVulkanImage());
		vulkanEyeTexture.m_pDevice = _renderer->GetVulkanDevice()->GetDevice();
		vulkanEyeTexture.m_pPhysicalDevice = _renderer->GetVulkanDevice()->GetPhysicalDevice();
		vulkanEyeTexture.m_pQueue = queue;
		vulkanEyeTexture.m_pInstance = _renderer->GetVulkanInstance()->GetInstance();
		vulkanEyeTexture.m_nQueueFamilyIndex = _renderer->GetVulkanDevice()->GetWorkQueue();
		vulkanEyeTexture.m_nFormat = texture->GetVulkanFormat();
		vulkanEyeTexture.m_nHeight = texture->GetDescriptor().height;
		vulkanEyeTexture.m_nWidth = texture->GetDescriptor().width;
		vulkanEyeTexture.m_nSampleCount = texture->GetDescriptor().sampleCount;
		vulkanEyeTexture.m_unArrayIndex = 0;
		vulkanEyeTexture.m_unArraySize = 2;

		vr::Texture_t eyeTexture = { (void *)&vulkanEyeTexture, vr::TextureType_Vulkan, vr::ColorSpace_Gamma };

		vr::VRCompositor()->Submit(vr::Eye_Left, &eyeTexture, nullptr, vr::Submit_VulkanTextureWithArrayData);

		vulkanEyeTexture.m_unArrayIndex = 1;
		vr::VRCompositor()->Submit(vr::Eye_Right, &eyeTexture, nullptr, vr::Submit_VulkanTextureWithArrayData);
	}

	VkImage OpenVRVulkanSwapChain::GetVulkanColorBuffer(int i) const
	{
		return _targetTexture->Downcast<VulkanTexture>()->GetVulkanImage();
	}

	Framebuffer *OpenVRVulkanSwapChain::GetOpenVRSwapChainFramebuffer() const
	{
		return GetFramebuffer()->Downcast<Framebuffer>();
	}
}
