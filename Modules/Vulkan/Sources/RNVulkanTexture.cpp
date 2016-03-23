//
//  RNVulkanTexture.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanTexture.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanTexture, Texture)

	static VkImageType VkImageTypeFromTextureType(Texture::Descriptor::Type type)
	{
		switch(type)
		{
			case Texture::Descriptor::Type::Type1D:
			case Texture::Descriptor::Type::Type1DArray:
				return VK_IMAGE_TYPE_1D;

			case Texture::Descriptor::Type::Type2D:
			case Texture::Descriptor::Type::Type2DArray:
				return VK_IMAGE_TYPE_2D;

			case Texture::Descriptor::Type::Type3D:
				return VK_IMAGE_TYPE_3D;

			default:
				throw InconsistencyException("Invalid texture type for Vulkan");
		}
	}

	static VkImageViewType VkImageViewTypeFromTextureType(Texture::Descriptor::Type type)
	{
		switch(type)
		{
			case Texture::Descriptor::Type::Type1D:
				return VK_IMAGE_VIEW_TYPE_1D;
			case Texture::Descriptor::Type::Type1DArray:
				return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

			case Texture::Descriptor::Type::Type2D:
				return VK_IMAGE_VIEW_TYPE_2D;
			case Texture::Descriptor::Type::Type2DArray:
				return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			case Texture::Descriptor::Type::Type3D:
				return VK_IMAGE_VIEW_TYPE_3D;

			default:
				throw InconsistencyException("Invalid texture type for Vulkan");
		}
	}

	static bool VkFormatIsDepthFormat(VkFormat format)
	{
		switch(format)
		{
			case VK_FORMAT_X8_D24_UNORM_PACK32:
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return true;

			default:
				return false;
		}
	}

	static bool VkFormatIsStencilFormat(VkFormat format)
	{
		switch(format)
		{
			case VK_FORMAT_S8_UINT:
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return true;

			default:
				return false;
		}
	}

	static VkImageUsageFlags VkImageUsageFromDescriptor(const Texture::Descriptor &descriptor, VkFormat format)
	{
		VkImageUsageFlags flags = 0;

		switch(descriptor.usageOptions)
		{
			case GPUResource::UsageOptions::ReadWrite:
				flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				break;
			case GPUResource::UsageOptions::WriteOnly:
				flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				break;
			case GPUResource::UsageOptions::Private:
				flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				break;
		}

		if(descriptor.usageHint & Texture::Descriptor::UsageHint::RenderTarget)
		{
			bool depth = VkFormatIsDepthFormat(format);
			bool stencil = VkFormatIsStencilFormat(format);

			if(!depth && !stencil)
				flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			else
				flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		else
		{
			flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		return flags;
	}

	static VkImageAspectFlags VkImageAspectFlagsFromFormat(VkFormat format)
	{
		bool depth = VkFormatIsDepthFormat(format);
		bool stencil = VkFormatIsStencilFormat(format);

		if(!depth && !stencil)
			return VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageAspectFlags flags = 0;

		if(depth)
			flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if(depth)
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;

		return flags;
	}

	static void setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);

	VulkanTexture::VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer) :
		Texture(descriptor),
		_renderer(renderer),
		_image(VK_NULL_HANDLE),
		_imageView(VK_NULL_HANDLE),
		_memory(VK_NULL_HANDLE),
		_format(renderer->GetVulkanFormatForName(descriptor.GetFormat()))
	{
		VulkanDevice *device = renderer->GetVulkanDevice();

		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.pNext = nullptr;
		image.imageType = VkImageTypeFromTextureType(descriptor.type);
		image.format = _format;
		image.extent = { descriptor.width, descriptor.height, descriptor.depth };
		image.mipLevels = descriptor.mipMaps;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VkImageUsageFromDescriptor(descriptor, image.format);
		image.flags = 0;

		RNVulkanValidate(vk::CreateImage(device->GetDevice(), &image, nullptr, &_image));


		VkMemoryRequirements requirements;
		vk::GetImageMemoryRequirements(device->GetDevice(), _image, &requirements);


		VkMemoryAllocateInfo alloctionInfo = {};
		alloctionInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloctionInfo.pNext = nullptr;
		alloctionInfo.allocationSize = requirements.size;

		device->GetMemoryWithType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alloctionInfo.memoryTypeIndex);


		RNVulkanValidate(vk::AllocateMemory(device->GetDevice(), &alloctionInfo, nullptr, &_memory));
		RNVulkanValidate(vk::BindImageMemory(device->GetDevice(), _image, _memory, 0));


		VkImageViewCreateInfo imageView = {};
		imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageView.pNext = nullptr;
		imageView.viewType = VkImageViewTypeFromTextureType(descriptor.type);
		imageView.format = image.format;
		imageView.flags = 0;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = VkImageAspectFlagsFromFormat(image.format);
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = _image;

		SetImageLayout(_image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		RNVulkanValidate(vk::CreateImageView(device->GetDevice(), &imageView, nullptr, &_imageView));
	}

	VulkanTexture::VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image, VkImageView imageView) :
		Texture(descriptor),
		_renderer(renderer),
		_image(image),
		_imageView(imageView),
		_memory(VK_NULL_HANDLE),
		_format(VK_FORMAT_B8G8R8A8_UNORM)
	{}

	VulkanTexture::~VulkanTexture()
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();

		if(_imageView != VK_NULL_HANDLE)
			vk::DestroyImageView(device->GetDevice(), _imageView, _renderer->GetAllocatorCallback());

		if(_image != VK_NULL_HANDLE)
			vk::DestroyImage(device->GetDevice(), _image, _renderer->GetAllocatorCallback());

		if(_memory != VK_NULL_HANDLE)
			vk::FreeMemory(device->GetDevice(), _memory, _renderer->GetAllocatorCallback());
	}

	void VulkanTexture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{}
	void VulkanTexture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{}
	void VulkanTexture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow)
	{}
	void VulkanTexture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const
	{}
	void VulkanTexture::SetParameter(const Parameter &parameter)
	{}
	void VulkanTexture::GenerateMipMaps()
	{}
	bool VulkanTexture::HasColorChannel(ColorChannel channel) const
	{
		return true;
	}



	void VulkanTexture::SetImageLayout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		// Source layouts (old)

		// Undefined layout
		// Only allowed as initial layout!
		// Make sure any writes to the image have been finished
		if(oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED) //TODO: Check if there is a case where this is needed...
			imageMemoryBarrier.srcAccessMask = 0;// VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

		// Old layout is color attachment
		// Make sure any writes to the color buffer have been finished
		if(oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// Old layout is transfer source
		// Make sure any reads from the image have been finished
		if(oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		// Old layout is shader read (sampler, input attachment)
		// Make sure any shader reads from the image have been finished
		if(oldImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

		// Target layouts (new)

		// New layout is transfer destination (copy, blit)
		// Make sure any copyies to the image have been finished
		if(newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		// New layout is transfer source (copy, blit)
		// Make sure any reads from and writes to the image have been finished
		if(newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		// New layout is color attachment
		// Make sure any writes to the color buffer hav been finished
		if(newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		// New layout is depth attachment
		// Make sure any writes to depth/stencil buffer have been finished
		if(newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			imageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// New layout is shader read (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if(newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}


		// Put barrier on top
		VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		// Put barrier inside setup command buffer
		VkCommandBuffer buffer = static_cast<VulkanRenderer *>(Renderer::GetActiveRenderer())->GetGlobalCommandBuffer();
		vk::CmdPipelineBarrier(buffer, srcStageFlags, destStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
}
