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
		VulkanFramebuffer::VulkanTargetView *colorTargetView = new VulkanFramebuffer::VulkanTargetView();
		colorTargetView->targetView = targetView;
		colorTargetView->targetView.texture->Retain();

		colorTargetView->vulkanTargetViewDescriptor = {};
		colorTargetView->vulkanTargetViewDescriptor.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorTargetView->vulkanTargetViewDescriptor.pNext = NULL;
		colorTargetView->vulkanTargetViewDescriptor.format = VulkanTexture::VulkanImageFormatFromTextureFormat(targetView.texture->GetDescriptor().format);
		colorTargetView->vulkanTargetViewDescriptor.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorTargetView->vulkanTargetViewDescriptor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorTargetView->vulkanTargetViewDescriptor.flags = 0;
		colorTargetView->vulkanTargetViewDescriptor.image = targetView.texture->Downcast<VulkanTexture>()->GetVulkanImage();
		colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseMipLevel = targetView.mipmap;

		//TODO: Support multisampled array render targets and plane slices
		switch(targetView.texture->GetDescriptor().type)
		{
			case Texture::Type::Type1D:
			{
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = 0;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			}

			case Texture::Type::Type1DArray:
			{
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = targetView.length;
				colorTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				break;
			}

			case Texture::Type::Type2D:
			{
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			}

			case Texture::Type::Type2DMS:
			{
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			}

			case Texture::Type::Type2DArray:
			{
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = targetView.length;
				colorTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				break;
			}

			case Texture::Type::Type3D:
			{
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.levelCount = 1;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.baseArrayLayer = targetView.slice;
				colorTargetView->vulkanTargetViewDescriptor.subresourceRange.layerCount = targetView.length;
				colorTargetView->vulkanTargetViewDescriptor.viewType = VK_IMAGE_VIEW_TYPE_3D;
				break;
			}

			default:
				RN_ASSERT(false, "Unsupported render target type ");
		}

		return colorTargetView;
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VulkanSwapChain *swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat) :
		Framebuffer(Vector2()),
		_sampleCount(1),
		_renderer(renderer),
		_swapChain(swapChain),
		_depthStencilTarget(nullptr)
//		_rtvHandle(nullptr),
//		_dsvHandle(nullptr)
	{
		DidUpdateSwapChain(size, colorFormat, depthStencilFormat);
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer) :
		Framebuffer(size),
		_sampleCount(1),
		_renderer(renderer),
		_swapChain(nullptr),
		_depthStencilTarget(nullptr)
//		_rtvHandle(nullptr),
//		_dsvHandle(nullptr)
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

/*		if (_rtvHandle)
			delete _rtvHandle;
		if (_dsvHandle)
			delete _dsvHandle;*/
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

	void VulkanFramebuffer::PrepareAsRendertargetForFrame(uint32 frame)
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		_frameLastUsed = frame;

		uint32 attachmentCount = _colorTargets.size();
		std::vector<VkImageView> attachments;
		if(_swapChain) attachmentCount = 1;
		if(_depthStencilTarget) attachmentCount += 1;

		if(_colorTargets.size() > 0)
		{
			//Create the render target view
			uint32 counter = 0;
			if(_swapChain)
			{
				VkImageView imageView;
				RNVulkanValidate(vk::CreateImageView(device, &(_colorTargets[_swapChain->GetFrameIndex()]->vulkanTargetViewDescriptor), _renderer->GetAllocatorCallback(), &imageView));
				attachments.push_back(imageView);
			}
			else
			{
				for(VulkanTargetView *targetView : _colorTargets)
				{
					VkImageView imageView;
					RNVulkanValidate(vk::CreateImageView(device, &targetView->vulkanTargetViewDescriptor, _renderer->GetAllocatorCallback(), &imageView));
					attachments.push_back(imageView);
				}
			}
		}

		if(_depthStencilTarget)
		{
			VkImageView imageView;
			RNVulkanValidate(vk::CreateImageView(device, &_depthStencilTarget->vulkanTargetViewDescriptor, _renderer->GetAllocatorCallback(), &imageView));
			attachments.push_back(imageView);
		}

		//TODO: Create framebuffer per framebuffer and not per camera
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.pNext = nullptr;
		frameBufferCreateInfo.renderPass = _renderPass;
		frameBufferCreateInfo.attachmentCount = attachmentCount;
		frameBufferCreateInfo.pAttachments = &attachments[0];
		frameBufferCreateInfo.width = static_cast<uint32>(_size.x);
		frameBufferCreateInfo.height = static_cast<uint32>(_size.y);
		frameBufferCreateInfo.layers = 1;

		RNVulkanValidate(vk::CreateFramebuffer(device, &frameBufferCreateInfo, _renderer->GetAllocatorCallback(), &_frameBuffer));
	}

	void VulkanFramebuffer::SetAsRendertarget(VulkanCommandList *commandList) const
	{
		//Set the rendertargets
//		commandList->GetCommandList()->OMSetRenderTargets(_colorTargets.size() ? 1 : 0, _rtvHandle, false, _dsvHandle);
	}

	void VulkanFramebuffer::ClearColorTargets(VulkanCommandList *commandList, const Color &color)
	{
		if(_colorTargets.size() == 0)
			return;

//		D3D12_RECT clearRect{ 0, 0, static_cast<LONG>(GetSize().x), static_cast<LONG>(GetSize().y) };
//		commandList->GetCommandList()->ClearRenderTargetView(*_rtvHandle, &color.r, 1, &clearRect);
	}

	void VulkanFramebuffer::ClearDepthStencilTarget(VulkanCommandList *commandList, float depth, uint8 stencil)
	{
		if(!_depthStencilTarget)
			return;

//		D3D12_RECT clearRect{ 0, 0, static_cast<LONG>(GetSize().x), static_cast<LONG>(GetSize().y) };
		//TODO: Needs D3D12_CLEAR_FLAG_STENCIL to also clear stencil buffer
//		commandList->GetCommandList()->ClearDepthStencilView(*_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 1, &clearRect);
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
		VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
		commandBuffer->Begin();

		uint8 bufferCount = _swapChain->GetBufferCount();
		if(bufferCount > 0)
		{
			for(uint8 i = 0; i < bufferCount; i++)
			{
				VkImage colorBuffer = _swapChain->GetVulkanColorBuffer(i);

				Texture::Descriptor colorTextureDescriptor;
				colorTextureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
				colorTextureDescriptor.width = static_cast<uint32>(size.x);
				colorTextureDescriptor.height = static_cast<uint32>(size.y);
				colorTextureDescriptor.format = colorFormat;

				VulkanTexture *bufferTexture = new VulkanTexture(colorTextureDescriptor, _renderer, colorBuffer);
				VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), colorBuffer, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

/*	void VulkanFramebuffer::InitializeRenderPass()
	{
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

		if(_colorTargets.size() > 0)
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
