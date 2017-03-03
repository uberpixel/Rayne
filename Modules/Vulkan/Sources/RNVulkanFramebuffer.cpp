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

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, const Descriptor &descriptor, VkSwapchainKHR swapChain,VulkanRenderer *renderer) :
		Framebuffer(size, descriptor),
		_renderer(renderer),
		_renderPass(VK_NULL_HANDLE)
	{
		VulkanDevice *device = renderer->GetVulkanDevice();
		uint32_t count = kRNVulkanRenderStages;

		VkImage colorImages[32];

		if(swapChain != VK_NULL_HANDLE)
		{
			vk::GetSwapchainImagesKHR(device->GetDevice(), swapChain, &count, nullptr);
			vk::GetSwapchainImagesKHR(device->GetDevice(), swapChain, &count, colorImages);
		}

		_framebuffers.resize(count);

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
				attachment.SetFormat(descriptor.colorFormat);

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
				attachment.SetFormat(descriptor.depthFormat);
				data->depthTexture = new VulkanTexture(attachment, renderer);

				attachments[attachmentCount++] = data->depthTexture->GetImageView();

			}
			if(descriptor.stencilFormat != Texture::Format::Invalid)
			{
				if(descriptor.stencilFormat != descriptor.depthFormat)
				{
					attachment.SetFormat(descriptor.stencilFormat);
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
	VulkanFramebuffer::~VulkanFramebuffer()
	{}

	Texture *VulkanFramebuffer::GetColorTexture() const
	{
		return _framebuffers[0]->colorTexture;
	}
	Texture *VulkanFramebuffer::GetDepthTexture() const
	{
		return _framebuffers[0]->depthTexture;
	}
	Texture *VulkanFramebuffer::GetStencilTexture() const
	{
		return _framebuffers[0]->stencilTexture;
	}

	Texture *VulkanFramebuffer::GetColorTexture(size_t index) const
	{
		return _framebuffers[index]->colorTexture;
	}
	Texture *VulkanFramebuffer::GetDepthTexture(size_t index) const
	{
		return _framebuffers[index]->depthTexture;
	}
	Texture *VulkanFramebuffer::GetStencilTexture(size_t index) const
	{
		return _framebuffers[index]->stencilTexture;
	}

	VkFramebuffer VulkanFramebuffer::GetFramebuffer(size_t index) const
	{
		return _framebuffers[index]->framebuffer;
	}

	void VulkanFramebuffer::InitializeRenderPass(size_t index)
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
	}
}
