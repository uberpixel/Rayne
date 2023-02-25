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
		else if(targetView.texture->GetDescriptor().format == Texture::Format::Depth_16I || targetView.texture->GetDescriptor().format == Texture::Format::Depth_24I || targetView.texture->GetDescriptor().format == Texture::Format::Depth_32F)
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

	static VkImageViewCreateInfo VkImageViewCreateInfoWithMultiviewPatch(const VkImageViewCreateInfo &oldInfo, uint8 multiviewLayer, uint8 multiviewCount)
	{
		VkImageViewCreateInfo newInfo = oldInfo;

		//TODO: Support multisampled array render targets and plane slices
		if(multiviewCount > 0 || multiviewLayer > 0)
		{
			if(multiviewCount == 0)
			{
				multiviewCount = 1;
			}
			switch(newInfo.viewType)
			{
				case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
				{
					newInfo.subresourceRange.baseArrayLayer = multiviewLayer;
					newInfo.subresourceRange.layerCount = multiviewCount;
					break;
				}

				case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
				{
					newInfo.subresourceRange.baseArrayLayer = multiviewLayer;
					newInfo.subresourceRange.layerCount = multiviewCount;
					break;
				}

				case VK_IMAGE_VIEW_TYPE_3D:
				{
					newInfo.subresourceRange.baseArrayLayer = multiviewLayer;
					newInfo.subresourceRange.layerCount = multiviewCount;
					break;
				}

				default:
					RN_ASSERT(false, "Unsupported render target type ");
			}
		}

		return newInfo;
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, uint8 layerCount, VulkanSwapChain *swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat, Texture::Format fragmentDensityFormat) :
		Framebuffer(Vector2()),
		_sampleCount(1),
		_renderer(renderer),
		_swapChain(swapChain),
		_depthStencilTarget(nullptr),
        _currentVariantIndex(0)
	{
		DidUpdateSwapChain(size, layerCount, colorFormat, depthStencilFormat, fragmentDensityFormat);
	}

	VulkanFramebuffer::VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer) :
		Framebuffer(size),
		_sampleCount(1),
		_renderer(renderer),
		_swapChain(nullptr),
		_depthStencilTarget(nullptr),
		_currentVariantIndex(0)
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

		//Release cached vulkan framebuffers and their resources
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		for(const VulkanFramebufferVariant &variant : _framebufferVariants)
		{
			VkFramebuffer framebuffer = variant.framebuffer;
			std::vector<VkImageView> imageViews = variant.attachments;

			_renderer->AddFrameFinishedCallback([this, device, imageViews, framebuffer]() {
				vk::DestroyFramebuffer(device, framebuffer, _renderer->GetAllocatorCallback());

				for(VkImageView imageView : imageViews)
				{
					vk::DestroyImageView(device, imageView, _renderer->GetAllocatorCallback());
				}
			});
		}

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

	void VulkanFramebuffer::PrepareAsRendertargetForFrame(VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags, uint8 multiviewLayer, uint8 multiviewCount)
	{
		//Check if there is already a cached variant for this framebuffer
		uint8 swapchainImageIndex = 0;
		uint8 resolveSwapchainImageIndex = 0;
		uint8 fragmentDensitySwapchainImageIndex = 0;
		if(_colorTargets.size() > 0)
		{
			//Create the render target view
			if(_swapChain)
			{
				swapchainImageIndex = _swapChain->GetFrameIndex();
			}
			else if(resolveFramebuffer->_swapChain)
			{
				resolveSwapchainImageIndex = resolveFramebuffer->_swapChain->GetFrameIndex();
			}
		}
		VulkanFramebuffer *fragmentDensityFramebuffer = this;
		if(resolveFramebuffer)
		{
			fragmentDensityFramebuffer = resolveFramebuffer;
		}
		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 1 && fragmentDensityFramebuffer->_swapChain)
		{
			fragmentDensitySwapchainImageIndex = fragmentDensityFramebuffer->_swapChain->GetFrameIndex();
		}

		uint8 counter = 0;
		for(const VulkanFramebufferVariant &variant : _framebufferVariants)
		{
			if(variant.resolveFramebuffer == resolveFramebuffer && variant.renderPassFlags == flags && variant.multiviewLayer == multiviewLayer && variant.multiviewCount == multiviewCount && variant.swapchainImageIndex == swapchainImageIndex && variant.resolveSwapchainImageIndex == resolveSwapchainImageIndex && variant.fragmentDensitySwapchainImageIndex == fragmentDensitySwapchainImageIndex)
			{
				_currentVariantIndex = counter;
				return;
			}
			counter += 1;
		}
        _currentVariantIndex = _framebufferVariants.size();

		VulkanFramebufferVariant newVariant;
		newVariant.resolveFramebuffer = resolveFramebuffer;
		newVariant.renderPassFlags = flags;
		newVariant.multiviewLayer = multiviewLayer;
		newVariant.multiviewCount = multiviewCount;
		newVariant.swapchainImageIndex = 0;
		newVariant.resolveSwapchainImageIndex = 0;
		newVariant.fragmentDensitySwapchainImageIndex = 0;

		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		newVariant.renderPass = _renderer->GetVulkanRenderPass(this, resolveFramebuffer, flags, multiviewCount);

		if(_colorTargets.size() > 0)
		{
			//Create the render target view
			if(_swapChain)
			{
				newVariant.swapchainImageIndex = _swapChain->GetFrameIndex();
				const VkImageViewCreateInfo &imageViewCreateInfo = VkImageViewCreateInfoWithMultiviewPatch(_colorTargets[newVariant.swapchainImageIndex]->vulkanTargetViewDescriptor, multiviewLayer, multiviewCount);

				VkImageView imageView;
				RNVulkanValidate(vk::CreateImageView(device, &imageViewCreateInfo, _renderer->GetAllocatorCallback(), &imageView));
				newVariant.attachments.push_back(imageView);
			}
			else
			{
				int counter = 0;
				for(VulkanTargetView *targetView : _colorTargets)
				{
					const VkImageViewCreateInfo &imageViewCreateInfo = VkImageViewCreateInfoWithMultiviewPatch(targetView->vulkanTargetViewDescriptor, multiviewLayer, multiviewCount);

					VkImageView imageView;
					RNVulkanValidate(vk::CreateImageView(device, &imageViewCreateInfo, _renderer->GetAllocatorCallback(), &imageView));
					newVariant.attachments.push_back(imageView);

					//TODO: Add some error handling for wrong target counts for msaa
					if(resolveFramebuffer)
					{
						VkImageView imageView;
						if(resolveFramebuffer->_swapChain)
						{
							newVariant.resolveSwapchainImageIndex = resolveFramebuffer->_swapChain->GetFrameIndex();
							const VkImageViewCreateInfo &imageViewCreateInfo = VkImageViewCreateInfoWithMultiviewPatch(resolveFramebuffer->_colorTargets[newVariant.resolveSwapchainImageIndex]->vulkanTargetViewDescriptor, multiviewLayer, multiviewCount);
							RNVulkanValidate(vk::CreateImageView(device, &imageViewCreateInfo, _renderer->GetAllocatorCallback(), &imageView));
						}
						else
						{
							const VkImageViewCreateInfo &imageViewCreateInfo = VkImageViewCreateInfoWithMultiviewPatch(resolveFramebuffer->_colorTargets[counter]->vulkanTargetViewDescriptor, multiviewLayer, multiviewCount);
							RNVulkanValidate(vk::CreateImageView(device, &imageViewCreateInfo, _renderer->GetAllocatorCallback(), &imageView));
						}
						newVariant.attachments.push_back(imageView);
					}

					counter += 1;
				}
			}
		}

		if(_depthStencilTarget)
		{
			const VkImageViewCreateInfo &imageViewCreateInfo = VkImageViewCreateInfoWithMultiviewPatch(_depthStencilTarget->vulkanTargetViewDescriptor, multiviewLayer, multiviewCount);

			VkImageView imageView;
			RNVulkanValidate(vk::CreateImageView(device, &imageViewCreateInfo, _renderer->GetAllocatorCallback(), &imageView));
			newVariant.attachments.push_back(imageView);
		}

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 1 && fragmentDensityFramebuffer->_swapChain)
			{
				newVariant.fragmentDensitySwapchainImageIndex = fragmentDensityFramebuffer->_swapChain->GetFrameIndex();
			}
			const VkImageViewCreateInfo &imageViewCreateInfo = VkImageViewCreateInfoWithMultiviewPatch(fragmentDensityFramebuffer->_fragmentDensityTargets[newVariant.fragmentDensitySwapchainImageIndex]->vulkanTargetViewDescriptor, multiviewLayer, multiviewCount);

			VkImageView imageView;
			RNVulkanValidate(vk::CreateImageView(device, &imageViewCreateInfo, _renderer->GetAllocatorCallback(), &imageView));
			newVariant.attachments.push_back(imageView);
		}

		//TODO: Create framebuffer per framebuffer and not per camera, but also still handle msaa resolve somehow
		//TODO: Reuse framebuffer, cause creating imageviews and framebuffer is slow
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.pNext = nullptr;
		frameBufferCreateInfo.renderPass = newVariant.renderPass;
		frameBufferCreateInfo.attachmentCount = newVariant.attachments.size();
		frameBufferCreateInfo.pAttachments = newVariant.attachments.data();
		frameBufferCreateInfo.width = static_cast<uint32>(_size.x);
		frameBufferCreateInfo.height = static_cast<uint32>(_size.y);
		frameBufferCreateInfo.layers = 1; //Must be 1 for multiview, so this is ok for now, but will need to be the actual layer count when rendering to multiple layers with selection in the shader

		RNVulkanValidate(vk::CreateFramebuffer(device, &frameBufferCreateInfo, _renderer->GetAllocatorCallback(), &newVariant.framebuffer));
        _framebufferVariants.push_back(newVariant);
	}

	void VulkanFramebuffer::SetAsRendertarget(VkCommandBuffer commandBuffer, VulkanFramebuffer *resolveFramebuffer, const Color &clearColor, float depth, uint8 stencil) const
	{
		uint16 numberOfClearColors = _colorTargets.size();
		if(_swapChain || (resolveFramebuffer && resolveFramebuffer->_swapChain)) numberOfClearColors = 1;
		if(resolveFramebuffer) numberOfClearColors *= 2;
		numberOfClearColors += _depthStencilTarget? 1 : 0;

		std::vector<VkClearValue> clearColors;
		clearColors.reserve(numberOfClearColors);

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
		renderPassBeginInfo.renderPass = _framebufferVariants[_currentVariantIndex].renderPass;
		renderPassBeginInfo.framebuffer = _framebufferVariants[_currentVariantIndex].framebuffer;
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

		if(_fragmentDensityTargets.size() > 1)
		{
			for(VulkanTargetView *targetView : _fragmentDensityTargets)
			{
				SafeRelease(targetView->targetView.texture);
				delete targetView;
			}
		}
		_fragmentDensityTargets.clear();
	}

	void VulkanFramebuffer::DidUpdateSwapChain(Vector2 size, uint8 layerCount, Texture::Format colorFormat, Texture::Format depthStencilFormat, Texture::Format fragmentDensityFormat)
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
				colorTextureDescriptor.depth = layerCount;
				colorTextureDescriptor.type = layerCount > 1? Texture::Type::Type2DArray : Texture::Type::Type2D;
				colorTextureDescriptor.format = colorFormat;

				VulkanTexture *bufferTexture = new VulkanTexture(colorTextureDescriptor, _renderer, colorBuffer);
				VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), colorBuffer, 0, 1, 0, layerCount, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VulkanTexture::BarrierIntent::RenderTarget);
				bufferTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

				TargetView targetView;
				targetView.texture = bufferTexture->Autorelease();
				targetView.mipmap = 0;
				targetView.slice = 0;
				targetView.length = layerCount;

				VulkanTargetView *vulkanTargetView = VulkanTargetViewFromTargetView(targetView);
				_colorTargets.push_back(vulkanTargetView);
			}

			if(fragmentDensityFormat != Texture::Format::Invalid && _renderer->GetVulkanDevice()->GetSupportsFragmentDensityMaps())
			{
				for(uint8 i = 0; i < bufferCount; i++)
				{
					uint32 width = 0;
					uint32 height = 0;
					VkImage fragmentDensityBuffer = _swapChain->GetVulkanFragmentDensityBuffer(i, width, height);

					Texture::Descriptor fragmentDensityTextureDescriptor;
					fragmentDensityTextureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
					fragmentDensityTextureDescriptor.width = width;
					fragmentDensityTextureDescriptor.height = height;
					fragmentDensityTextureDescriptor.depth = layerCount;
					fragmentDensityTextureDescriptor.type = layerCount > 1 ? Texture::Type::Type2DArray : Texture::Type::Type2D;
					fragmentDensityTextureDescriptor.format = fragmentDensityFormat;

					VulkanTexture *bufferTexture = new VulkanTexture(fragmentDensityTextureDescriptor, _renderer, fragmentDensityBuffer);
                    VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), fragmentDensityBuffer, 0, 1, 0, layerCount, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT, VulkanTexture::BarrierIntent::ShaderSource);
                    bufferTexture->SetCurrentLayout(VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT);

					TargetView targetView;
					targetView.texture = bufferTexture->Autorelease();
					targetView.mipmap = 0;
					targetView.slice = 0;
					targetView.length = layerCount;

					VulkanTargetView *vulkanTargetView = VulkanTargetViewFromTargetView(targetView);
					_fragmentDensityTargets.push_back(vulkanTargetView);
				}
			}

            commandBuffer->End();
            _renderer->SubmitCommandBuffer(commandBuffer);
		}

		if(!_depthStencilTarget && depthStencilFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DRenderTargetFormat(depthStencilFormat, size.x, size.y);
			depthDescriptor.depth = layerCount;
			depthDescriptor.type = layerCount > 1? Texture::Type::Type2DArray : Texture::Type::Type2D;

			TargetView target;
			target.texture = Texture::WithDescriptor(depthDescriptor);
			target.mipmap = 0;
			target.slice = 0;
			target.length = layerCount;
			SetDepthStencilTarget(target);
		}
	}
}
