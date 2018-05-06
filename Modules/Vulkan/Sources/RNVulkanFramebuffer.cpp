//
//  RNVulkanFramebuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanFramebuffer.h"
#include "RNVulkanDevice.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanInternals.h"

namespace RN
{
	RNDefineMeta(VulkanFramebuffer, Framebuffer)

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VkSwapchainKHR swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat) :
		Framebuffer(size),
		_renderer(renderer)
	{
		VulkanDevice *device = renderer->GetVulkanDevice();
		uint32_t count = kRNVulkanRenderStages;

		VkImage colorImages[32];

		if(swapChain != VK_NULL_HANDLE)
		{
			vk::GetSwapchainImagesKHR(device->GetDevice(), swapChain, &count, nullptr);
			vk::GetSwapchainImagesKHR(device->GetDevice(), swapChain, &count, colorImages);
		}

		VulkanCommandBuffer *commandBuffer = renderer->GetCommandBuffer();
		commandBuffer->Begin();

		for(size_t i = 0; i < count; i ++)
		{
			FramebufferData *data = new FramebufferData();
			_framebuffers[i] = data;

			VkImageView attachments[3];
			size_t attachmentCount = 0;

			Texture::Descriptor attachment;
			attachment.usageHint = Texture::Descriptor::UsageHint::RenderTarget;
			attachment.width = static_cast<uint32>(size.x);
			attachment.height = static_cast<uint32>(size.y);

			if(descriptor.colorFormat != Texture::Format::Invalid)
			{
				attachment.format = descriptor.colorFormat;

				if(swapChain != VK_NULL_HANDLE)
				{
					VkImageViewCreateInfo colorAttachmentView = {};
					colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					colorAttachmentView.pNext = NULL;
					colorAttachmentView.format = VK_FORMAT_B8G8R8A8_UNORM;
					colorAttachmentView.components = {
						VK_COMPONENT_SWIZZLE_R,
						VK_COMPONENT_SWIZZLE_G,
						VK_COMPONENT_SWIZZLE_B,
						VK_COMPONENT_SWIZZLE_A
					};
					colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					colorAttachmentView.subresourceRange.baseMipLevel = 0;
					colorAttachmentView.subresourceRange.levelCount = 1;
					colorAttachmentView.subresourceRange.baseArrayLayer = 0;
					colorAttachmentView.subresourceRange.layerCount = 1;
					colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
					colorAttachmentView.flags = 0;
					colorAttachmentView.image = colorImages[i];

					VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), colorImages[i], 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

					VkImageView imageView;
					RNVulkanValidate(vk::CreateImageView(device->GetDevice(), &colorAttachmentView, _renderer->GetAllocatorCallback(), &imageView));

					data->colorTexture = new VulkanTexture(attachment, renderer, colorImages[i], imageView);
				}
				else
				{
					data->colorTexture = new VulkanTexture(attachment, renderer);
				}

				attachments[attachmentCount++] = data->colorTexture->GetImageView();
			}
			if(descriptor.depthFormat != Texture::Format::Invalid)
			{
				attachment.format = descriptor.depthFormat;
				data->depthTexture = new VulkanTexture(attachment, renderer);

				attachments[attachmentCount++] = data->depthTexture->GetImageView();

			}
			if(descriptor.stencilFormat != Texture::Format::Invalid)
			{
				if(descriptor.stencilFormat != descriptor.depthFormat)
				{
					attachment.format = descriptor.stencilFormat;
					data->stencilTexture = new VulkanTexture(attachment, renderer);

					attachments[attachmentCount++] = data->stencilTexture->GetImageView();
				}
				else
				{
					data->stencilTexture = data->depthTexture->Retain();
				}
			}

			if(_renderPass == VK_NULL_HANDLE)
				InitializeRenderPass(i);

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferCreateInfo.pNext = nullptr;
			frameBufferCreateInfo.renderPass = _renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32>(attachmentCount);
			frameBufferCreateInfo.pAttachments = attachments;
			frameBufferCreateInfo.width = static_cast<uint32>(size.x);
			frameBufferCreateInfo.height = static_cast<uint32>(size.y);
			frameBufferCreateInfo.layers = 1;

			RNVulkanValidate(vk::CreateFramebuffer(device->GetDevice(), &frameBufferCreateInfo, _renderer->GetAllocatorCallback(), &data->framebuffer));
		}

		commandBuffer->End();
		renderer->SubmitCommandBuffer(commandBuffer);
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer) :
		Framebuffer(size),
		_sampleCount(1),
		_renderer(renderer),
		_depthStencilTarget(nullptr)
	{

	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{

	}

	Texture *VulkanFramebuffer::GetColorTexture(uint32 index) const
	{
		return _colorTargets[index]->targetView.texture;
	}

	Texture *VulkanFramebuffer::GetDepthStencilTexture() const
	{
		return _depthStencilTarget->targetView.texture;
	}

/*	void VulkanFramebuffer::ClearDepthStencilTarget(D3D12CommandList *commandList, float depth, uint8 stencil)
	{
		if(!_depthStencilTarget)
			return;

		D3D12_RECT clearRect{ 0, 0, static_cast<LONG>(GetSize().x), static_cast<LONG>(GetSize().y) };
		//TODO: Needs D3D12_CLEAR_FLAG_STENCIL to also clear stencil buffer
		commandList->GetCommandList()->ClearDepthStencilView(*_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 1, &clearRect);
	}*/

	void VulkanFramebuffer::WillUpdateSwapChain()
	{
		if(_swapChainColorBuffers)
		{
			uint8 bufferCount = _swapChain->GetBufferCount();
			for(uint8 i = 0; i < bufferCount; i++)
			{
				_swapChainColorBuffers[i]->Release();
			}

			delete[] _swapChainColorBuffers;
			_swapChainColorBuffers = nullptr;
		}

		if(_swapChainDepthBuffers)
		{
			uint8 bufferCount = _swapChain->GetBufferCount();
			for (uint8 i = 0; i < bufferCount; i++)
			{
				_swapChainDepthBuffers[i]->Release();
			}

			delete[] _swapChainDepthBuffers;
			_swapChainDepthBuffers = nullptr;
		}
	}

	void VulkanFramebuffer::DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat)
	{
		_size = size;

		for(D3D12ColorTargetView *targetView : _colorTargets)
		{
			delete targetView;
		}
		_colorTargets.clear();

		uint8 bufferCount = _swapChain->GetBufferCount();
		if(bufferCount > 0)
		{
			_swapChainColorBuffers = new ID3D12Resource*[bufferCount];
			for(uint8 i = 0; i < bufferCount; i++)
			{
				_swapChainColorBuffers[i] = _swapChain->GetD3D12ColorBuffer(i);
			}

			if(_swapChain->HasDepthBuffer())
			{
				_swapChainDepthBuffers = new ID3D12Resource*[bufferCount];
				for (uint8 i = 0; i < bufferCount; i++)
				{
					_swapChainDepthBuffers[i] = _swapChain->GetD3D12DepthBuffer(i);
				}
			}
		}

		D3D12ColorTargetView *targetView = new D3D12ColorTargetView();
		targetView->targetView.texture = nullptr;
		targetView->targetView.mipmap = 0;
		targetView->targetView.slice = 0;
		targetView->targetView.length = 1;
		targetView->d3dTargetViewDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(colorFormat);
		targetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		targetView->d3dTargetViewDesc.Texture2D.MipSlice = 0;
		targetView->d3dTargetViewDesc.Texture2D.PlaneSlice = 0;
		_colorTargets.push_back(targetView);

		if(!_swapChainDepthBuffers && depthStencilFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DRenderTargetFormat(depthStencilFormat, size.x, size.y);

			TargetView target;
			target.texture = Texture::WithDescriptor(depthDescriptor);
			target.mipmap = 0;
			target.slice = 0;
			target.length = 1;
			SetDepthStencilTarget(target);
		}

		if(_swapChainDepthBuffers)
		{
			D3D12DepthStencilTargetView *targetView = new D3D12DepthStencilTargetView();
			targetView->targetView.texture = nullptr;
			targetView->targetView.mipmap = 0;
			targetView->targetView.slice = 0;
			targetView->targetView.length = 1;
			targetView->d3dTargetViewDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(depthStencilFormat);
			targetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			targetView->d3dTargetViewDesc.Texture2D.MipSlice = 0;

			_depthStencilTarget = targetView;
		}
	}

/*	void VulkanFramebuffer::InitializeRenderPass(size_t index)
	{
		FramebufferData *data = _framebuffers[index];

		VkAttachmentDescription attachments[3];
		uint32_t count = 0;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		if(data->colorTexture)
		{
			attachments[count].format = data->colorTexture->GetFormat();
			attachments[count].flags = 0;
			attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[count].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[count].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			subpass.pColorAttachments = &colorReference;

			count ++;
		}

		if(data->depthTexture)
		{
			attachments[count].format = data->depthTexture->GetFormat();
			attachments[count].flags = 0;
			attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[count].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depthReference;

			count ++;
		}

		if(data->stencilTexture && data->stencilTexture != data->depthTexture)
		{
			attachments[count].format = data->stencilTexture->GetFormat();
			attachments[count].flags = 0;
			attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[count].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depthReference;

			count ++;
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.attachmentCount = count;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = nullptr;

		VulkanDevice *device = _renderer->GetVulkanDevice();
		RNVulkanValidate(vk::CreateRenderPass(device->GetDevice(), &renderPassInfo, _renderer->GetAllocatorCallback(), &_renderPass));
	}*/
}
