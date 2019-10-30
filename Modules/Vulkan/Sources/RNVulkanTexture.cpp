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
			case Texture::Type::Type2DMS:
			case Texture::Type::Type2DArray:
			case Texture::Type::TypeCube:
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
			case Texture::Format::RGBA_8:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::RGBA_8_SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Texture::Format::BGRA_8:
				return VK_FORMAT_B8G8R8A8_UNORM;
			case Texture::Format::BGRA_8_SRGB:
				return VK_FORMAT_B8G8R8A8_SRGB;
			case Texture::Format::RGB_10_A_2:
				return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
			case Texture::Format::R_8:
				return VK_FORMAT_R8_UNORM;
			case Texture::Format::RG_8:
				return VK_FORMAT_R8G8_UNORM;
			case Texture::Format::RGB_8:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::RGB_8_SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Texture::Format::R_16F:
				return VK_FORMAT_R16_SFLOAT;
			case Texture::Format::RG_16F:
				return VK_FORMAT_R16G16_SFLOAT;
			case Texture::Format::RGB_16F:
				return VK_FORMAT_R16G16B16_SFLOAT;
			case Texture::Format::RGBA_16F:
				return VK_FORMAT_R16G16B16A16_SFLOAT;
			case Texture::Format::R_32F:
				return VK_FORMAT_R32_SFLOAT;
			case Texture::Format::RG_32F:
				return VK_FORMAT_R32G32_SFLOAT;
			case Texture::Format::RGB_32F:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case Texture::Format::RGBA_32F:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case Texture::Format::Depth_24I:
				return VK_FORMAT_X8_D24_UNORM_PACK32;
			case Texture::Format::Depth_32F:
				return VK_FORMAT_D32_SFLOAT;
			case Texture::Format::Stencil_8:
				return VK_FORMAT_S8_UINT;
			case Texture::Format::Depth_24_Stencil_8:
				return VK_FORMAT_D24_UNORM_S8_UINT;
			case Texture::Format::Depth_32F_Stencil_8:
				return VK_FORMAT_D32_SFLOAT_S8_UINT;

			case Texture::Format::RGBA_BC1_SRGB:
				return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
			case Texture::Format::RGBA_BC2_SRGB:
				return VK_FORMAT_BC2_SRGB_BLOCK;
			case Texture::Format::RGBA_BC3_SRGB:
				return VK_FORMAT_BC3_SRGB_BLOCK;
			case Texture::Format::RGBA_BC7_SRGB:
				return VK_FORMAT_BC7_SRGB_BLOCK;

			case Texture::Format::RGBA_BC1:
				return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			case Texture::Format::RGBA_BC2:
				return VK_FORMAT_BC2_UNORM_BLOCK;
			case Texture::Format::RGBA_BC3:
				return VK_FORMAT_BC3_UNORM_BLOCK;
            case Texture::Format::RGBA_BC4:
                return VK_FORMAT_BC4_UNORM_BLOCK;
			case Texture::Format::RGBA_BC7:
				return VK_FORMAT_BC7_UNORM_BLOCK;

			case Texture::Format::RGBA_ASTC_4X4_SRGB:
				return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_5X4_SRGB:
				return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_5X5_SRGB:
				return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_6X5_SRGB:
				return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_6X6_SRGB:
				return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_8X5_SRGB:
				return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_8X6_SRGB:
				return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_8X8_SRGB:
				return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_10X5_SRGB:
				return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_10X6_SRGB:
				return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_10X8_SRGB:
				return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_10X10_SRGB:
				return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_12X10_SRGB:
				return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
			case Texture::Format::RGBA_ASTC_12X12_SRGB:
				return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;

			case Texture::Format::RGBA_ASTC_4X4:
				return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_5X4:
				return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_5X5:
				return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_6X5:
				return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_6X6:
				return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_8X5:
				return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_8X6:
				return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_8X8:
				return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_10X5:
				return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_10X6:
				return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_10X8:
				return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_10X10:
				return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_12X10:
				return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
			case Texture::Format::RGBA_ASTC_12X12:
				return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;

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

			case Texture::Type::Type2DMS:
			case Texture::Type::Type2D:
				return VK_IMAGE_VIEW_TYPE_2D;
			case Texture::Type::Type2DArray:
				return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			case Texture::Type::Type3D:
				return VK_IMAGE_VIEW_TYPE_3D;

			case Texture::Type::TypeCube:
				return VK_IMAGE_VIEW_TYPE_CUBE;

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
				flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
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
		if(stencil)
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;

		return flags;
	}

	VulkanTexture::VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer) :
		Texture(descriptor),
		_renderer(renderer),
		_uploadImage(VK_NULL_HANDLE),
		_uploadMemory(VK_NULL_HANDLE),
		_uploadData(nullptr),
		_image(VK_NULL_HANDLE),
		_imageView(VK_NULL_HANDLE),
		_memory(VK_NULL_HANDLE),
		_format(VulkanImageFormatFromTextureFormat(descriptor.format))
	{
		VulkanDevice *device = renderer->GetVulkanDevice();

		VkFormatProperties properties;
		vk::GetPhysicalDeviceFormatProperties(device->GetPhysicalDevice(), _format, &properties);
		RN_ASSERT(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, "Requested texture format is not supported by this device");

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VkImageTypeFromTextureType(descriptor.type);
		imageInfo.format = _format;
		imageInfo.extent = { descriptor.width, descriptor.height, imageInfo.imageType != VK_IMAGE_TYPE_2D? descriptor.depth : 1 };
		imageInfo.arrayLayers = descriptor.depth;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = static_cast<VkSampleCountFlagBits>(descriptor.sampleCount);
		imageInfo.usage = VkImageUsageFromDescriptor(descriptor, imageInfo.format);
		imageInfo.flags = 0;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.mipLevels = descriptor.mipMaps;

	/*	if(descriptor.usageHint & UsageHint::RenderTarget)
		{
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else
		{
			imageInfo.tiling = (descriptor.mipMaps > 1)? VK_IMAGE_TILING_OPTIMAL:VK_IMAGE_TILING_LINEAR;
		}*/

		if(descriptor.type == Texture::Type::TypeCube)
		{
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		_currentLayout = imageInfo.initialLayout;

		VkImageFormatProperties formatProperties;
		vk::GetPhysicalDeviceImageFormatProperties(device->GetPhysicalDevice(), imageInfo.format, imageInfo.imageType, imageInfo.tiling, imageInfo.usage, imageInfo.flags, &formatProperties);

		RN_ASSERT(formatProperties.sampleCounts & descriptor.sampleCount, "Requested sample count for texture format is not supported by this device");

/*		uint32 requestedSampleCount = descriptor.sampleCount;
		uint32 availableSampleCount = descriptor.sampleCount;
		while(!(formatProperties.sampleCounts & availableSampleCount))
		{
			availableSampleCount = availableSampleCount >> 1;
		}

		imageInfo.samples = static_cast<VkSampleCountFlagBits>(availableSampleCount);
		if(availableSampleCount != requestedSampleCount)
		{
			RNDebug(RNSTR("Requested sample count: " << requestedSampleCount << ", But available sample count: " << availableSampleCount));
		}*/

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
			//if(descriptor.mipMaps > 1)
				device->GetMemoryWithType(_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocateInfo.memoryTypeIndex);
			//else
			//	device->GetMemoryWithType(_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, allocateInfo.memoryTypeIndex);
		}

		RNVulkanValidate(vk::AllocateMemory(device->GetDevice(), &allocateInfo, _renderer->GetAllocatorCallback(), &_memory));
		RNVulkanValidate(vk::BindImageMemory(device->GetDevice(), _image, _memory, 0));

		if((descriptor.usageHint & UsageHint::RenderTarget))
		{
			if(VkFormatIsDepthFormat(_format) || VkFormatIsStencilFormat(_format))
			{
				VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
				commandBuffer->Begin();
				uint32 aspectFlagBits = 0;
				if(VkFormatIsDepthFormat(_format))
				{
					aspectFlagBits |= VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if(VkFormatIsStencilFormat(_format))
				{
					aspectFlagBits |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
				SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, aspectFlagBits, _currentLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, BarrierIntent::RenderTarget);
				_currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				commandBuffer->End();
				_renderer->SubmitCommandBuffer(commandBuffer);
			}
			else //Any color rendertarget
			{
				VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
				commandBuffer->Begin();
				SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, _currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, BarrierIntent::RenderTarget);
				_currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				commandBuffer->End();
				_renderer->SubmitCommandBuffer(commandBuffer);
			}
		}

		VkImageViewCreateInfo imageViewInfo = {};
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
		imageViewInfo.subresourceRange.layerCount = descriptor.depth;
		imageViewInfo.image = _image;

		RNVulkanValidate(vk::CreateImageView(device->GetDevice(), &imageViewInfo, _renderer->GetAllocatorCallback(), &_imageView));
	}

	VulkanTexture::VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image) :
		Texture(descriptor),
		_renderer(renderer),
		_uploadImage(VK_NULL_HANDLE),
		_uploadMemory(VK_NULL_HANDLE),
		_uploadData(nullptr),
		_image(image),
		_imageView(VK_NULL_HANDLE),
		_memory(VK_NULL_HANDLE),
		_format(VulkanImageFormatFromTextureFormat(descriptor.format))
	{

	}

	VulkanTexture::~VulkanTexture()
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();

		if(_uploadImage != VK_NULL_HANDLE)
			StopStreamingData();

		if(_imageView != VK_NULL_HANDLE)
			vk::DestroyImageView(device->GetDevice(), _imageView, _renderer->GetAllocatorCallback());

		if(_image != VK_NULL_HANDLE)
			vk::DestroyImage(device->GetDevice(), _image, _renderer->GetAllocatorCallback());

		if(_memory != VK_NULL_HANDLE)
			vk::FreeMemory(device->GetDevice(), _memory, _renderer->GetAllocatorCallback());
	}

	void VulkanTexture::StartStreamingData(const Region &region)
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

		VkMemoryRequirements uploadRequirements;

		_isFirstUpload = true;

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
		RNVulkanValidate(vk::CreateImage(device, &imageInfo, _renderer->GetAllocatorCallback(), &_uploadImage));

		vk::GetImageMemoryRequirements(device, _uploadImage, &uploadRequirements);
		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = uploadRequirements.size;

		_renderer->GetVulkanDevice()->GetMemoryWithType(uploadRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, allocateInfo.memoryTypeIndex);

		RNVulkanValidate(vk::AllocateMemory(device, &allocateInfo, _renderer->GetAllocatorCallback(), &_uploadMemory));
		RNVulkanValidate(vk::BindImageMemory(device, _uploadImage, _uploadMemory, 0));

		VkImageSubresource subRes = {};
		subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subRes.mipLevel = 0;

		// Get sub resources layout
		// Includes row pitch, size offsets, etc.
		vk::GetImageSubresourceLayout(device, _uploadImage, &subRes, &_uploadSubresourceLayout);

		// Map image memory
		RNVulkanValidate(vk::MapMemory(device, _uploadMemory, 0, uploadRequirements.size, 0, &_uploadData));
	}

	void VulkanTexture::StopStreamingData()
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		vk::UnmapMemory(device, _uploadMemory);

		VkImage uploadImage = _uploadImage;
		VkDeviceMemory uploadMemory = _uploadMemory;
		VulkanRenderer *renderer = _renderer;
		renderer->AddFrameFinishedCallback([renderer, device, uploadImage, uploadMemory]() {
			vk::DestroyImage(device, uploadImage, renderer->GetAllocatorCallback());
			vk::FreeMemory(device, uploadMemory, renderer->GetAllocatorCallback());
		});

		_uploadImage = VK_NULL_HANDLE;
		_uploadMemory = VK_NULL_HANDLE;
		_uploadData = nullptr;
	}

	void VulkanTexture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow, size_t numberOfRows)
	{
		SetData(Region(0, 0, 0, _descriptor.GetWidthForMipMapLevel(mipmapLevel), _descriptor.GetHeightForMipMapLevel(mipmapLevel), _descriptor.depth), mipmapLevel, bytes, bytesPerRow, numberOfRows);
	}

	void VulkanTexture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow, size_t numberOfRows)
	{
		SetData(region, mipmapLevel, 0, bytes, bytesPerRow, numberOfRows);
	}

	void VulkanTexture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow, size_t numberOfRows) {
		bool isOneTimeUpload = true;
		if(_uploadImage)
		{
			isOneTimeUpload = false;
		}

		if(isOneTimeUpload)
		{
			StartStreamingData(region);
		}

		// Copy image data into memory
		size_t rowIndex = 0;
		while(rowIndex < numberOfRows)
		{
			memcpy(static_cast<uint8*>(_uploadData) + _uploadSubresourceLayout.offset + rowIndex * _uploadSubresourceLayout.rowPitch, static_cast<const uint8*>(bytes) + rowIndex * bytesPerRow, bytesPerRow);
			rowIndex += 1;
		}

		VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
		commandBuffer->Begin();
		if(_isFirstUpload)
		{
			SetImageLayout(commandBuffer->GetCommandBuffer(), _uploadImage, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, BarrierIntent::UploadSource);
			_isFirstUpload = false;
		}

		SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, _currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, BarrierIntent::UploadDestination);
		VkImageLayout oldLayout = _currentLayout;
		if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
		{
			oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		_currentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		VkImageCopy copyRegion = {};

		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset = {0, 0, 0};

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

		vk::CmdCopyImage(commandBuffer->GetCommandBuffer(), _uploadImage,
						 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _image,
						 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps,
					   VK_IMAGE_ASPECT_COLOR_BIT, _currentLayout, oldLayout,
					   BarrierIntent::ShaderSource);
		_currentLayout = oldLayout;
		commandBuffer->End();

		_renderer->SubmitCommandBuffer(commandBuffer);

		if(isOneTimeUpload)
		{
			StopStreamingData();
		}
	}

	void VulkanTexture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const
	{
		//TODO: Force main thread, or make it more flexible

/*		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

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
		vk::FreeMemory(device, downloadMemory, _renderer->GetAllocatorCallback());*/
	}

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

	void VulkanTexture::SetImageLayout(VkCommandBuffer buffer, VkImage image, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask, VkImageLayout fromLayout, VkImageLayout toLayout, BarrierIntent intent)
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

		VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		if(intent == BarrierIntent::ShaderSource)
		{
			srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destStageFlags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		}

		if(intent == BarrierIntent::CopySource)
		{
			srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

			if(fromLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) srcStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		if(intent == BarrierIntent::CopyDestination)
		{
			srcStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if(intent == BarrierIntent::UploadSource)
		{
			srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if(intent == BarrierIntent::UploadDestination)
		{
			srcStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if(intent == BarrierIntent::RenderTarget)
		{
			srcStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			if(fromLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}

			if(toLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) destStageFlags = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		if(intent == BarrierIntent::ExternalSource)
		{
			srcStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

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
			//destStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

		// Put barrier inside setup command buffer
		vk::CmdPipelineBarrier(buffer, srcStageFlags, destStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
}
