//
//  RNVulkanTexture.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanTexture.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanInternals.h"

namespace RN
{
	RNDefineMeta(VulkanTexture, Texture)

	static VkImageType VkImageTypeFromTextureType(Texture::Type type)
	{
		switch(type)
		{
			case Texture::Type::Type1D:
			case Texture::Type::Type1DArray:
				return VK_IMAGE_TYPE_1D;

			case Texture::Type::Type2D:
			case Texture::Type::Type2DArray:
				return VK_IMAGE_TYPE_2D;

			case Texture::Type::Type3D:
				return VK_IMAGE_TYPE_3D;

			default:
				throw InconsistencyException("Invalid texture type for Vulkan");
		}
	}

	VkFormat VulkanTexture::VulkanImageFormatFromTextureFormat(Texture::Format format)
	{
		switch(format)
		{
			case Texture::Format::RGBA8888:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::RGBA8888SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Texture::Format::BGRA8888:
				return VK_FORMAT_B8G8R8A8_UNORM;
			case Texture::Format::BGRA8888SRGB:
				return VK_FORMAT_B8G8R8A8_SRGB;
			case Texture::Format::RGB10A2:
				return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
			case Texture::Format::R8:
				return VK_FORMAT_R8_UNORM;
			case Texture::Format::RG88:
				return VK_FORMAT_R8G8_UNORM;
			case Texture::Format::RGB888:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::RGB888SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Texture::Format::R16F:
				return VK_FORMAT_R16_SFLOAT;
			case Texture::Format::RG16F:
				return VK_FORMAT_R16G16_SFLOAT;
			case Texture::Format::RGB16F:
				return VK_FORMAT_R16G16B16_SFLOAT;
			case Texture::Format::RGBA16F:
				return VK_FORMAT_R16G16B16A16_SFLOAT;
			case Texture::Format::R32F:
				return VK_FORMAT_R32_SFLOAT;
			case Texture::Format::RG32F:
				return VK_FORMAT_R32G32_SFLOAT;
			case Texture::Format::RGB32F:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case Texture::Format::RGBA32F:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case Texture::Format::Depth24I:
				return VK_FORMAT_X8_D24_UNORM_PACK32;
			case Texture::Format::Depth32F:
				return VK_FORMAT_D32_SFLOAT;
			case Texture::Format::Stencil8:
				return VK_FORMAT_S8_UINT;
			case Texture::Format::Depth24Stencil8:
				return VK_FORMAT_D24_UNORM_S8_UINT;
			case Texture::Format::Depth32FStencil8:
				return VK_FORMAT_D32_SFLOAT_S8_UINT;

			default:
				return VK_FORMAT_UNDEFINED;
		}
	}

	static VkImageViewType VkImageViewTypeFromTextureType(Texture::Type type)
	{
		switch(type)
		{
			case Texture::Type::Type1D:
				return VK_IMAGE_VIEW_TYPE_1D;
			case Texture::Type::Type1DArray:
				return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

			case Texture::Type::Type2D:
				return VK_IMAGE_VIEW_TYPE_2D;
			case Texture::Type::Type2DArray:
				return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			case Texture::Type::Type3D:
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

		switch(descriptor.accessOptions)
		{
			case GPUResource::AccessOptions::ReadWrite:
				flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				break;
			case GPUResource::AccessOptions::WriteOnly:
				flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				break;
			case GPUResource::AccessOptions::Private:
				flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				break;
		}

		if(descriptor.usageHint & Texture::UsageHint::RenderTarget)
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
			flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
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

	VulkanTexture::VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer) :
		Texture(descriptor),
		_renderer(renderer),
		_image(VK_NULL_HANDLE),
		_memory(VK_NULL_HANDLE),
		_format(VulkanImageFormatFromTextureFormat(descriptor.format))
	{
		VulkanDevice *device = renderer->GetVulkanDevice();

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VkImageTypeFromTextureType(descriptor.type);
		imageInfo.format = _format;
		imageInfo.extent = { descriptor.width, descriptor.height, descriptor.depth };
		imageInfo.arrayLayers = 1;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.usage = VkImageUsageFromDescriptor(descriptor, imageInfo.format);
		imageInfo.flags = 0;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.mipLevels = descriptor.mipMaps;

		if(descriptor.usageHint & UsageHint::RenderTarget)
		{
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else
		{
			imageInfo.tiling = (descriptor.mipMaps > 1)? VK_IMAGE_TILING_OPTIMAL:VK_IMAGE_TILING_LINEAR;
		}

		_currentLayout = imageInfo.initialLayout;

		RNVulkanValidate(vk::CreateImage(device->GetDevice(), &imageInfo, _renderer->GetAllocatorCallback(), &_image));


		vk::GetImageMemoryRequirements(device->GetDevice(), _image, &_requirements);

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = _requirements.size;

		if(descriptor.usageHint & UsageHint::RenderTarget)
		{
			device->GetMemoryWithType(_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocateInfo.memoryTypeIndex);
		}
		else
		{
			if(descriptor.mipMaps > 1)
				device->GetMemoryWithType(_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocateInfo.memoryTypeIndex);
			else
				device->GetMemoryWithType(_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, allocateInfo.memoryTypeIndex);
		}

		RNVulkanValidate(vk::AllocateMemory(device->GetDevice(), &allocateInfo, _renderer->GetAllocatorCallback(), &_memory));
		RNVulkanValidate(vk::BindImageMemory(device->GetDevice(), _image, _memory, 0));


/*		VkImageViewCreateInfo imageViewInfo = {};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.pNext = nullptr;
		imageViewInfo.viewType = VkImageViewTypeFromTextureType(descriptor.type);
		imageViewInfo.format = imageInfo.format;
		imageViewInfo.flags = 0;
		imageViewInfo.subresourceRange = {};
		imageViewInfo.subresourceRange.aspectMask = VkImageAspectFlagsFromFormat(imageInfo.format);
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = descriptor.mipMaps;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.image = _image;

		RNVulkanValidate(vk::CreateImageView(device->GetDevice(), &imageViewInfo, _renderer->GetAllocatorCallback(), &_imageView));

		if(!(descriptor.usageHint & UsageHint::RenderTarget))
		{
			VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
			commandBuffer->Begin();
			SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			commandBuffer->End();
			_renderer->SubmitCommandBuffer(commandBuffer);
		}
		else if(descriptor.format == Format::Depth24Stencil8)
		{
			VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
			commandBuffer->Begin();
			SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			commandBuffer->End();
			_renderer->SubmitCommandBuffer(commandBuffer);
		}*/
	}

	VulkanTexture::VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image) :
		Texture(descriptor),
		_renderer(renderer),
		_image(image),
		_memory(VK_NULL_HANDLE),
		_format(VulkanImageFormatFromTextureFormat(descriptor.format))
	{

	}

	VulkanTexture::~VulkanTexture()
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();

//		if(_imageView != VK_NULL_HANDLE)
//			vk::DestroyImageView(device->GetDevice(), _imageView, _renderer->GetAllocatorCallback());

		if(_image != VK_NULL_HANDLE)
			vk::DestroyImage(device->GetDevice(), _image, _renderer->GetAllocatorCallback());

		if(_memory != VK_NULL_HANDLE)
			vk::FreeMemory(device->GetDevice(), _memory, _renderer->GetAllocatorCallback());

//		if(_sampler != VK_NULL_HANDLE)
//			vk::DestroySampler(device->GetDevice(), _sampler, _renderer->GetAllocatorCallback());
	}

	void VulkanTexture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		SetData(Region(0, 0, 0, _descriptor.GetWidthForMipMapLevel(mipmapLevel), _descriptor.GetHeightForMipMapLevel(mipmapLevel), _descriptor.depth), mipmapLevel, bytes, bytesPerRow);
	}

	void VulkanTexture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		SetData(region, mipmapLevel, 0, bytes, bytesPerRow);
	}

	void VulkanTexture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow)
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

		VkImage uploadImage;
		VkDeviceMemory uploadMemory;
		VkMemoryRequirements uploadRequirements;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VkImageTypeFromTextureType(_descriptor.type);
		imageInfo.format = _format;
		imageInfo.extent = { region.width, region.height, region.depth };
		imageInfo.arrayLayers = 1;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.usage = VkImageUsageFromDescriptor(_descriptor, imageInfo.format);
		imageInfo.flags = 0;
		imageInfo.mipLevels = 1;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		RNVulkanValidate(vk::CreateImage(device, &imageInfo, _renderer->GetAllocatorCallback(), &uploadImage));

		vk::GetImageMemoryRequirements(device, uploadImage, &uploadRequirements);
		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = uploadRequirements.size;

		_renderer->GetVulkanDevice()->GetMemoryWithType(uploadRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, allocateInfo.memoryTypeIndex);

		RNVulkanValidate(vk::AllocateMemory(device, &allocateInfo, _renderer->GetAllocatorCallback(), &uploadMemory));
		RNVulkanValidate(vk::BindImageMemory(device, uploadImage, uploadMemory, 0));

		VkSubresourceLayout subResLayout;
		void *data;

		VkImageSubresource subRes = {};
		subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subRes.mipLevel = 0;

		// Get sub resources layout
		// Includes row pitch, size offsets, etc.
		vk::GetImageSubresourceLayout(device, uploadImage, &subRes, &subResLayout);

		// Map image memory
		RNVulkanValidate(vk::MapMemory(device, uploadMemory, 0, uploadRequirements.size, 0, &data));

		// Copy image data into memory
		memcpy(data, bytes, bytesPerRow*_descriptor.height);

		vk::UnmapMemory(device, uploadMemory);

		VulkanCommandBufferWithCallback *commandBuffer = _renderer->GetCommandBufferWithCallback();
		commandBuffer->Begin();
		SetImageLayout(commandBuffer->GetCommandBuffer(), uploadImage, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, _currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		_currentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = slice;
		copyRegion.dstSubresource.mipLevel = mipmapLevel;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset.x = region.x;
		copyRegion.dstOffset.y = region.y;
		copyRegion.dstOffset.z = region.z;

		copyRegion.extent.width = region.width;
		copyRegion.extent.height = region.height;
		copyRegion.extent.depth = region.depth;

		vk::CmdCopyImage(commandBuffer->GetCommandBuffer(), uploadImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, _currentLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		_currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		commandBuffer->End();

		commandBuffer->SetFinishedCallback([this, device, uploadImage, uploadMemory]() {
			vk::DestroyImage(device, uploadImage, _renderer->GetAllocatorCallback());
			vk::FreeMemory(device, uploadMemory, _renderer->GetAllocatorCallback());
		});

		_renderer->SubmitCommandBuffer(commandBuffer);
	}

	void VulkanTexture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const
	{
		//TODO: Force main thread, or make it more flexible

		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

		VkImage downloadImage;
		VkDeviceMemory downloadMemory;
		VkMemoryRequirements downloadRequirements;

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VkImageTypeFromTextureType(_descriptor.type);
		imageInfo.format = _format;
		imageInfo.extent = { _descriptor.width, _descriptor.height, _descriptor.depth };
		imageInfo.arrayLayers = 1;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.usage = VkImageUsageFromDescriptor(_descriptor, imageInfo.format);
		imageInfo.flags = 0;
		imageInfo.mipLevels = 1;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		RNVulkanValidate(vk::CreateImage(device, &imageInfo, _renderer->GetAllocatorCallback(), &downloadImage));

		vk::GetImageMemoryRequirements(device, downloadImage, &downloadRequirements);
		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = downloadRequirements.size;

		_renderer->GetVulkanDevice()->GetMemoryWithType(downloadRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, allocateInfo.memoryTypeIndex);

		RNVulkanValidate(vk::AllocateMemory(device, &allocateInfo, _renderer->GetAllocatorCallback(), &downloadMemory));
		RNVulkanValidate(vk::BindImageMemory(device, downloadImage, downloadMemory, 0));

		VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
		commandBuffer->Begin();
		SetImageLayout(commandBuffer->GetCommandBuffer(), downloadImage, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		SetImageLayout(commandBuffer->GetCommandBuffer(), _image, mipmapLevel, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = mipmapLevel;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = { 0, 0, 0 };

		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset = { 0, 0, 0 };

		copyRegion.extent.width = _descriptor.width;
		copyRegion.extent.height = _descriptor.height;
		copyRegion.extent.depth = _descriptor.depth;

		vk::CmdCopyImage(commandBuffer->GetCommandBuffer(), _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, downloadImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		SetImageLayout(commandBuffer->GetCommandBuffer(), _image, mipmapLevel, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		SetImageLayout(commandBuffer->GetCommandBuffer(), downloadImage, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		commandBuffer->End();
		_renderer->SubmitCommandBuffer(commandBuffer);

		//TODO: block and wait for GPU to copy data before reading it.

		VkSubresourceLayout subResLayout;
		void *data;

		VkImageSubresource subRes = {};
		subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subRes.mipLevel = 0;

		// Get sub resources layout
		// Includes row pitch, size offsets, etc.
		vk::GetImageSubresourceLayout(device, downloadImage, &subRes, &subResLayout);

		// Map image memory
		RNVulkanValidate(vk::MapMemory(device, downloadMemory, 0, downloadRequirements.size, 0, &data));

		// Copy image data into memory
		memcpy(bytes, data, bytesPerRow*_descriptor.height);

		vk::UnmapMemory(device, downloadMemory);

		vk::DestroyImage(device, downloadImage, _renderer->GetAllocatorCallback());
		vk::FreeMemory(device, downloadMemory, _renderer->GetAllocatorCallback());
	}

/*	void VulkanTexture::SetParameter(const Parameter &parameter)
	{
		//TODO: Have the state coordinator pool these.
		Texture::SetParameter(parameter);

		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		if(_sampler)
			vk::DestroySampler(device, _sampler, _renderer->GetAllocatorCallback());

		VkSamplerAddressMode addressMode;
		switch(parameter.wrapMode)
		{
			case Texture::WrapMode::Clamp:
				addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			case Texture::WrapMode::Repeat:
				addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				break;
		}

		VkFilter filter;
		switch(parameter.filter)
		{
			case Texture::Filter::Nearest:
				filter = VK_FILTER_NEAREST;
				break;
			case Texture::Filter::Linear:
				filter = VK_FILTER_LINEAR;
				break;
		}

		// Create sampler
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = _descriptor.mipMaps;
		samplerInfo.maxAnisotropy = parameter.anisotropy;
		samplerInfo.anisotropyEnable = (parameter.anisotropy > 0)? VK_TRUE:VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		RNVulkanValidate(vk::CreateSampler(device, &samplerInfo, _renderer->GetAllocatorCallback(), &_sampler));
	}*/

	void VulkanTexture::GenerateMipMaps()
	{
		_renderer->CreateMipMapForTexture(this);
	}

/*	void VulkanTexture::TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout)
	{
		TransitionToLayout(buffer, targetLayout, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT);
		_currentLayout = targetLayout;
	}

	void VulkanTexture::TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask)
	{
		SetImageLayout(buffer, _image, baseMipmap, mipmapCount, aspectMask, _currentLayout, targetLayout);
		_currentLayout = targetLayout;
	}*/

	void VulkanTexture::SetImageLayout(VkCommandBuffer buffer, VkImage image, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask, VkImageLayout fromLayout, VkImageLayout toLayout)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = fromLayout;
		imageMemoryBarrier.newLayout = toLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
		imageMemoryBarrier.subresourceRange.baseMipLevel = baseMipmap;
		imageMemoryBarrier.subresourceRange.levelCount = mipmapCount;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = 0;

		// Source layouts (old)

		// Old layout is color attachment
		// Make sure any writes to the color buffer have been finished
		if(fromLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// Old layout is shader read (sampler, input attachment)
		// Make sure any shader reads from the image have been finished
		if(fromLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

		if(fromLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		// Target layouts (new)

		// New layout is transfer destination (copy, blit)
		// Make sure any copyies to the image have been finished
		if(toLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		// New layout is transfer source (copy, blit)
		// Make sure any reads from and writes to the image have been finished
		if(toLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			//imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		// New layout is color attachment
		// Make sure any writes to the color buffer hav been finished
		if(toLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		if(toLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		}

		// New layout is depth attachment
		// Make sure any writes to depth/stencil buffer have been finished
		if(toLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			imageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// New layout is shader read (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if(toLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		if(fromLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		// Undefined layout
		// Only allowed as initial layout!
		// Make sure any writes to the image have been finished
		if(fromLayout == VK_IMAGE_LAYOUT_UNDEFINED) //TODO: Check if there is a case where this is needed...
			imageMemoryBarrier.srcAccessMask = 0;// VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

		// Put barrier on top
		VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		// Put barrier inside setup command buffer
		vk::CmdPipelineBarrier(buffer, srcStageFlags, destStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
}
