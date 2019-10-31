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

	static VulkanFramebuffer::VulkanTargetView *VulkanTargetViewFromTargetView(const Framebuffer::TargetView &targetView)
	{
		VulkanFramebuffer::VulkanTargetView *vulkanTargetView = new VulkanFramebuffer::VulkanTargetView();
		vulkanTargetView->targetView = targetView;
		vulkanTargetView->targetView.texture->Retain();

		vulkanTargetView->vulkanTargetViewDescriptor = {};
		vulkanTargetView->vulkanTargetViewDescriptor.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vulkanTargetView->vulkanTargetViewDescriptor.pNext = NULL;
		vulkanTargetView->vulkanTargetViewDescriptor.format = VulkanTexture::VulkanImageFormatFromTextureFormat(targetView.texture->GetDescriptor().format);
		vulkanTargetView->vulkanTargetViewDescriptor.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};

		if(targetView.texture->GetDescriptor().format == Texture::Format::Depth_24_Stencil_8 || targetView.texture->GetDescriptor().format == Texture::Format::Depth_32F_Stencil_8)
		{
			vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else if(targetView.texture->GetDescriptor().format == Texture::Format::Depth_24I || targetView.texture->GetDescriptor().format == Texture::Format::Depth_32F)
		{
			vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (targetView.texture->GetDescriptor().format == Texture::Format::Stencil_8)
		{
			vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		vulkanTargetView->vulkanTargetViewDescriptor.flags = 0;
		vulkanTargetView->vulkanTargetViewDescriptor.image = targetView.texture->Downcast<VulkanTexture>()->GetVulkanImage();
		vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseMipLevel = targetView.mipmap;

		//TODO: Support multisampled array render targets and plane slices
		switch(targetView.texture->GetDescriptor().type)
		{
			case Texture::Type::Type1D:
			{
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = 0;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			}

			case Texture::Type::Type1DArray:
			{
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = targetView.length;
				vulkanTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				break;
			}

			case Texture::Type::Type2D:
			{
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			}

			case Texture::Type::Type2DMS:
			{
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			}

			case Texture::Type::Type2DArray:
			{
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = targetView.length;
				vulkanTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				break;
			}

			case Texture::Type::Type3D:
			{
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				vulkanTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = targetView.length;
				vulkanTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_3D;
				break;
			}

			default:
				RN_ASSERT(false, "Unsupported render target type ");
		}

		return vulkanTargetView;
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VulkanSwapChain *swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat) :
		Framebuffer(Vector2()),
		_sampleCount(1),
		_renderer(renderer),
		_swapChain(swapChain),
		_depthStencilTarget(nullptr),
		_renderPass(nullptr),
		_frameBuffer(nullptr)
	{
		DidUpdateSwapChain(size, colorFormat, depthStencilFormat);
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer) :
		Framebuffer(size),
		_sampleCount(1),
		_renderer(renderer),
		_swapChain(nullptr),
		_depthStencilTarget(nullptr),
		_renderPass(nullptr),
		_frameBuffer(nullptr)
	{
/*		_colorFormat = D3D12ImageFormatFromTextureFormat(descriptor.colorFormat);
		_depthFormat = D3D12ImageFormatFromTextureFormat(descriptor.depthFormat);

		if (descriptor.colorFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor colorDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.colorFormat, size.x, size.y, false);
			colorDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;
			Texture *colorTexture = Texture::WithDescriptor(colorDescriptor);
			SetColorTexture(colorTexture);
		}

		if (descriptor.depthFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.depthFormat, size.x, size.y, false);
			depthDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;
			Texture *depthTexture = Texture::WithDescriptor(depthDescriptor);
			SetDepthTexture(depthTexture);
		}*/
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		//TODO: Maybe release swap chain resources!?

		for(VulkanTargetView *targetView : _colorTargets)
		{
			targetView->targetView.texture->Release();
			delete targetView;
		}

		if(_depthStencilTarget)
		{
			_depthStencilTarget->targetView.texture->Release();
			delete _depthStencilTarget;
		}
	}

	void VulkanFramebuffer::SetColorTarget(const TargetView &target, uint32 index)
	{
		RN_ASSERT(!_swapChain, "A swap chain framebuffer can not have additional color targets!");
		RN_ASSERT(target.texture, "The color target needs a texture!");

		_sampleCount = target.texture->GetDescriptor().sampleCount;

		VulkanTargetView *targetView = VulkanTargetViewFromTargetView(target);
		if(index < _colorTargets.size())
		{
			_colorTargets[index]->targetView.texture->Release();
			delete _colorTargets[index];
			_colorTargets[index] = targetView;
		}
		else
		{
			_colorTargets.push_back(targetView);
		}

		//TODO: Update renderpass once all changes have been made
	}

	void VulkanFramebuffer::SetDepthStencilTarget(const TargetView &target)
	{
		RN_ASSERT(!_swapChain || !_swapChain->HasDepthBuffer(), "A swap chain framebuffer can not have additional depth targets, if the swap chain has them!");
		RN_ASSERT(target.texture, "The depth stencil target needs a texture!");

		VulkanTargetView *targetView = VulkanTargetViewFromTargetView(target);
		if(_depthStencilTarget)
		{
			SafeRelease(_depthStencilTarget->targetView.texture);
			delete _depthStencilTarget;
		}

		_depthStencilTarget = targetView;

		//TODO: Update renderpass once all changes have been made
	}

	Texture *VulkanFramebuffer::GetColorTexture(uint32 index) const
	{
		if(index >= _colorTargets.size())
			return nullptr;

		return _colorTargets[index]->targetView.texture;
	}

	Texture *VulkanFramebuffer::GetDepthStencilTexture() const
	{
		if (!_depthStencilTarget)
			return nullptr;

		return _depthStencilTarget->targetView.texture;
	}

	uint8 VulkanFramebuffer::GetSampleCount() const
	{
		return _sampleCount;
	}

	void VulkanFramebuffer::PrepareAsRendertargetForFrame(VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags)
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

		_renderPass = _renderer->GetVulkanRenderPass(this, resolveFramebuffer, flags);

		std::vector<VkImageView> attachments;
		if(_colorTargets.size() > 0)
		{
			//Create the render target view
			if(_swapChain)
			{
				VkImageView imageView;
				RNVulkanValidate(vk::CreateImageView(device, &(_colorTargets[_swapChain->GetFrameIndex()]->vulkanTargetViewDescriptor), _renderer->GetAllocatorCallback(), &imageView));
				attachments.push_back(imageView);
                _colorTargets[_swapChain->GetFrameIndex()]->tempVulkanImageView = imageView;
			}
			else
			{
				int counter = 0;
				for(VulkanTargetView *targetView : _colorTargets)
				{
					VkImageView imageView;
					RNVulkanValidate(vk::CreateImageView(device, &targetView->vulkanTargetViewDescriptor, _renderer->GetAllocatorCallback(), &imageView));
                    targetView->tempVulkanImageView = imageView;
					attachments.push_back(imageView);

					//TODO: Add some error handling for wrong target counts for msaa
					if(resolveFramebuffer)
					{
						VkImageView imageView;
						if(resolveFramebuffer->_swapChain)
						{
							RNVulkanValidate(vk::CreateImageView(device, &(resolveFramebuffer->_colorTargets[resolveFramebuffer->_swapChain->GetFrameIndex()]->vulkanTargetViewDescriptor), _renderer->GetAllocatorCallback(), &imageView));
                            resolveFramebuffer->_colorTargets[resolveFramebuffer->_swapChain->GetFrameIndex()]->tempVulkanImageView = imageView;
						}
						else
						{
							RNVulkanValidate(vk::CreateImageView(device, &(resolveFramebuffer->_colorTargets[counter]->vulkanTargetViewDescriptor), _renderer->GetAllocatorCallback(), &imageView));
                            resolveFramebuffer->_colorTargets[counter]->tempVulkanImageView = imageView;
						}
						attachments.push_back(imageView);
					}

					counter += 1;
				}
			}
		}

		if(_depthStencilTarget)
		{
			VkImageView imageView;
			RNVulkanValidate(vk::CreateImageView(device, &_depthStencilTarget->vulkanTargetViewDescriptor, _renderer->GetAllocatorCallback(), &imageView));
            _depthStencilTarget->tempVulkanImageView = imageView;
			attachments.push_back(imageView);
		}

		//TODO: Create framebuffer per framebuffer and not per camera, but also still handle msaa resolve somehow
		//TODO: Reuse framebuffer, cause creating imageviews and framebuffer is slow
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.pNext = nullptr;
		frameBufferCreateInfo.renderPass = _renderPass;
		frameBufferCreateInfo.attachmentCount = attachments.size();
		frameBufferCreateInfo.pAttachments = attachments.data();
		frameBufferCreateInfo.width = static_cast<uint32>(_size.x);
		frameBufferCreateInfo.height = static_cast<uint32>(_size.y);
		frameBufferCreateInfo.layers = 1;

		RNVulkanValidate(vk::CreateFramebuffer(device, &frameBufferCreateInfo, _renderer->GetAllocatorCallback(), &_frameBuffer));

		VkFramebuffer framebuffer = _frameBuffer;
		_renderer->AddFrameFinishedCallback([this, device, attachments, framebuffer]() {
			vk::DestroyFramebuffer(device, framebuffer, _renderer->GetAllocatorCallback());

			for(VkImageView imageView : attachments)
			{
				vk::DestroyImageView(device, imageView, _renderer->GetAllocatorCallback());
			}
		});
	}

	void VulkanFramebuffer::SetAsRendertarget(VkCommandBuffer commandBuffer, VulkanFramebuffer *resolveFramebuffer, const Color &clearColor, float depth, uint8 stencil) const
	{
		std::vector<VkClearValue> clearColors;

		int counter = 0;
		for(VulkanTargetView *targetView : _colorTargets)
		{
			VkClearValue clearValue;
			clearValue.color = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
			clearColors.push_back(clearValue);

			if(resolveFramebuffer)
			{
				clearColors.push_back(clearValue);
			}

			counter += 1;

			if(_swapChain || (resolveFramebuffer && resolveFramebuffer->_swapChain)) break;
		}

		if(_depthStencilTarget)
		{
			VkClearValue clearValue;
			clearValue.depthStencil = {depth, stencil};
			clearColors.push_back(clearValue);
		}

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = _renderPass;
		renderPassBeginInfo.framebuffer = _frameBuffer;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(_size.x);
		renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(_size.y);
		renderPassBeginInfo.clearValueCount = clearColors.size();
		renderPassBeginInfo.pClearValues = clearColors.data();

		vk::CmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanFramebuffer::WillUpdateSwapChain()
	{
		for(VulkanTargetView *targetView : _colorTargets)
		{
			SafeRelease(targetView->targetView.texture);
			delete targetView;
		}
		_colorTargets.clear();

		if(_depthStencilTarget)
		{
			SafeRelease(_depthStencilTarget->targetView.texture);
			delete _depthStencilTarget;
		}
		_depthStencilTarget = nullptr;
	}

	void VulkanFramebuffer::DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat)
	{
		_size = size;

		VulkanDevice *device = _renderer->GetVulkanDevice();

		uint8 bufferCount = _swapChain->GetBufferCount();
		if(bufferCount > 0)
		{
			VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
			commandBuffer->Begin();

			for(uint8 i = 0; i < bufferCount; i++)
			{
				VkImage colorBuffer = _swapChain->GetVulkanColorBuffer(i);

				Texture::Descriptor colorTextureDescriptor;
				colorTextureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
				colorTextureDescriptor.width = static_cast<uint32>(size.x);
				colorTextureDescriptor.height = static_cast<uint32>(size.y);
				colorTextureDescriptor.format = colorFormat;

				VulkanTexture *bufferTexture = new VulkanTexture(colorTextureDescriptor, _renderer, colorBuffer);
//				VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), colorBuffer, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

				TargetView targetView;
				targetView.texture = bufferTexture->Autorelease();
				targetView.mipmap = 0;
				targetView.slice = 0;
				targetView.length = 1;

				VulkanTargetView *vulkanTargetView = VulkanTargetViewFromTargetView(targetView);
				_colorTargets.push_back(vulkanTargetView);
			}

			if(_swapChain->HasDepthBuffer())
			{
/*				_swapChainDepthBuffers = new ID3D12Resource*[bufferCount];
				for (uint8 i = 0; i < bufferCount; i++)
				{
					_swapChainDepthBuffers[i] = _swapChain->GetD3D12DepthBuffer(i);
				}*/
			}

			commandBuffer->End();
			_renderer->SubmitCommandBuffer(commandBuffer);
		}

		if(!_depthStencilTarget && depthStencilFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DRenderTargetFormat(depthStencilFormat, size.x, size.y);

			TargetView target;
			target.texture = Texture::WithDescriptor(depthDescriptor);
			target.mipmap = 0;
			target.slice = 0;
			target.length = 1;
			SetDepthStencilTarget(target);
		}

/*		if(_swapChainDepthBuffers)
		{
			VulkanTargetView *targetView = new VulkanTargetView();
			targetView->targetView.texture = nullptr;
			targetView->targetView.mipmap = 0;
			targetView->targetView.slice = 0;
			targetView->targetView.length = 1;
			targetView->d3dTargetViewDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(depthStencilFormat);
			targetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			targetView->d3dTargetViewDesc.Texture2D.MipSlice = 0;

			_depthStencilTarget = targetView;
		}*/
	}



/*	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VkSwapchainKHR swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat) :
		Framebuffer(size),
		_renderer(renderer)
	{
		VulkanCommandBuffer *commandBuffer = renderer->GetCommandBuffer();
		commandBuffer->Begin();

		for(size_t i = 0; i < count; i ++)
		{
			FramebufferData *data = new FramebufferData();
			_framebuffers[i] = data;

			VkImageView attachments[3];
			size_t attachmentCount = 0;

			Texture::Descriptor attachment;
			attachment.usageHint = Texture::UsageHint::RenderTarget;
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
	}*/

    VkImageView VulkanFramebuffer::GetCurrentFrameVulkanColorImageView() const
    {
        if(_colorTargets.size() > 0)
        {
            //Create the render target view
            if(_swapChain)
            {
                return _colorTargets[_swapChain->GetFrameIndex()]->tempVulkanImageView;
            }
            else
            {
                return _colorTargets[0]->tempVulkanImageView;
            }
        }

        return NULL;
    }
}
