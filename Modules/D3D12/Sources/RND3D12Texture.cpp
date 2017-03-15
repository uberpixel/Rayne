//
//  RND3D12Texture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Texture.h"
#include "RND3D12Renderer.h"
#include "RND3D12StateCoordinator.h"
#include "RND3D12Internals.h"

namespace RN
{
	RNDefineMeta(D3D12Texture, Texture)

	static D3D12_RESOURCE_DIMENSION D3D12ImageTypeFromTextureType(Texture::Descriptor::Type type)
	{
		switch(type)
		{
		case Texture::Descriptor::Type::Type1D:
		case Texture::Descriptor::Type::Type1DArray:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

		case Texture::Descriptor::Type::Type2D:
		case Texture::Descriptor::Type::Type2DArray:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		case Texture::Descriptor::Type::Type3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

		default:
			throw InconsistencyException("Invalid texture type for Vulkan");
		}
	}

	static DXGI_FORMAT D3D12ImageFormatFromTextureFormat(Texture::Format format)
	{
		switch(format)
		{
			case Texture::Format::RGBA8888:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::RGB10A2:
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			case Texture::Format::R8:
				return DXGI_FORMAT_R8_UNORM;
			case Texture::Format::RG88:
				return DXGI_FORMAT_R8G8_UNORM;
			case Texture::Format::RGB888:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::R16F:
				return DXGI_FORMAT_R16_FLOAT;
			case Texture::Format::RG16F:
				return DXGI_FORMAT_R16G16_FLOAT;
			case Texture::Format::RGB16F:
				return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case Texture::Format::RGBA16F:
				return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case Texture::Format::R32F:
				return DXGI_FORMAT_R32_FLOAT;
			case Texture::Format::RG32F:
				return DXGI_FORMAT_R32G32_FLOAT;
			case Texture::Format::RGB32F:
				return DXGI_FORMAT_R32G32B32_FLOAT;
			case Texture::Format::RGBA32F:
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case Texture::Format::Depth24I:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			case Texture::Format::Depth32F:
				return DXGI_FORMAT_D32_FLOAT;
			case Texture::Format::Stencil8:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			case Texture::Format::Depth24Stencil8:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			case Texture::Format::Depth32FStencil8:
				return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			default:
				return DXGI_FORMAT_UNKNOWN;
		}
	}

/*	static VkImageViewType VkImageViewTypeFromTextureType(Texture::Descriptor::Type type)
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
			flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
	}*/

/*	D3D12Texture::D3D12Texture(const Descriptor &descriptor, D3D12Renderer *renderer) :
		Texture(descriptor),
		_renderer(renderer),
		_image(VK_NULL_HANDLE),
		_imageView(VK_NULL_HANDLE),
		_memory(VK_NULL_HANDLE),
		_format(renderer->GetVulkanFormatForName(descriptor.GetFormat())),
		_sampler(VK_NULL_HANDLE)
	{
		D3D12Device *device = renderer->GetD3D12Device();

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
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.mipLevels = descriptor.mipMaps;

		if(descriptor.usageHint & Descriptor::UsageHint::RenderTarget)
		{
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else
		{
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.tiling = (descriptor.mipMaps > 1) ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
		}

		RNVulkanValidate(vk::CreateImage(device->GetDevice(), &imageInfo, _renderer->GetAllocatorCallback(), &_image));


		vk::GetImageMemoryRequirements(device->GetDevice(), _image, &_requirements);

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = _requirements.size;

		if(descriptor.usageHint & Descriptor::UsageHint::RenderTarget)
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
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.image = _image;

		RNVulkanValidate(vk::CreateImageView(device->GetDevice(), &imageViewInfo, _renderer->GetAllocatorCallback(), &_imageView));

		if(!(descriptor.usageHint & Descriptor::UsageHint::RenderTarget))
		{
			VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
			commandBuffer->Begin();
			SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			commandBuffer->End();
			_renderer->SubmitCommandBuffer(commandBuffer);
		}
		else if(descriptor.GetFormat()->IsEqual(RNCSTR("Depth24Stencil8")))
		{
			VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
			commandBuffer->Begin();
			SetImageLayout(commandBuffer->GetCommandBuffer(), _image, 0, _descriptor.mipMaps, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			commandBuffer->End();
			_renderer->SubmitCommandBuffer(commandBuffer);
		}

		SetParameter(_parameter);
	}*/

	D3D12Texture::D3D12Texture(const Descriptor &descriptor, D3D12Renderer *renderer) :
		Texture(descriptor),
		_renderer(renderer),
		_format(D3D12ImageFormatFromTextureFormat(descriptor.format)),
		_isReady(false),
		_needsMipMaps(false)
	{
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();

		D3D12_RESOURCE_DESC imageDesc = {};
		imageDesc.Dimension = D3D12ImageTypeFromTextureType(_descriptor.type);
		imageDesc.Alignment = 0;
		imageDesc.Width = descriptor.width;
		imageDesc.Height = descriptor.height;
		imageDesc.DepthOrArraySize = descriptor.depth;
		imageDesc.MipLevels = descriptor.mipMaps;
		imageDesc.Format = _format;
		imageDesc.SampleDesc.Count = 1;
		imageDesc.SampleDesc.Quality = 0;
		imageDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		imageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		// create the final texture buffer
		device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &imageDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_textureBuffer));
		_currentState = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	D3D12Texture::~D3D12Texture()
	{
		_textureBuffer->Release();
	}

	void D3D12Texture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		SetData(Region(0, 0, 0, _descriptor.GetWidthForMipMapLevel(mipmapLevel), _descriptor.GetHeightForMipMapLevel(mipmapLevel), _descriptor.depth), mipmapLevel, bytes, bytesPerRow);
	}

	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		SetData(region, mipmapLevel, 0, bytes, bytesPerRow);
	}
	
	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow)
	{
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();

		D3D12_RESOURCE_DESC imageDesc = {};
		imageDesc.Dimension = D3D12ImageTypeFromTextureType(_descriptor.type);
		imageDesc.Alignment = 0;
		imageDesc.Width = region.width;
		imageDesc.Height = region.height;
		imageDesc.DepthOrArraySize = region.depth;
		imageDesc.MipLevels = GetDescriptor().mipMaps;
		imageDesc.Format = _format;
		imageDesc.SampleDesc.Count = 1;
		imageDesc.SampleDesc.Quality = 0;
		imageDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		imageDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// create a temporary buffer to upload the texture from
		ID3D12Resource *textureUploadBuffer;
		UINT64 textureUploadBufferSize;
		device->GetCopyableFootprints(&imageDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
		device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadBuffer));

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = bytes;
		textureData.RowPitch = bytesPerRow;
		textureData.SlicePitch = bytesPerRow * region.height;

		D3D12CommandListWithCallback *commandList = _renderer->GetCommandListWithCallback();
		commandList->SetFinishedCallback([this, textureUploadBuffer]{
			_isReady = true;
			textureUploadBuffer->Release();
		});

		// Now we copy the upload buffer contents to the default heap
		UpdateSubresources(commandList->GetCommandList(), _textureBuffer, textureUploadBuffer, 0, 0, 1, &textureData);

		// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
		TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Now we execute the command list to upload the initial assets (triangle data)
		commandList->End();
		_renderer->SubmitCommandList(commandList);
	}

	void D3D12Texture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const
	{
/*		//TODO: Force main thread, or make it more flexible

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
		vk::FreeMemory(device, downloadMemory, _renderer->GetAllocatorCallback());*/
	}

	void D3D12Texture::SetParameter(const Parameter &parameter)
	{
		//TODO: Have the state coordinator pool these.
		Texture::SetParameter(parameter);
		
		D3D12_TEXTURE_ADDRESS_MODE addressMode;
		switch(parameter.wrapMode)
		{
		case Texture::WrapMode::Clamp:
			addressMode = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		case Texture::WrapMode::Repeat:
			addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			break;
		}

		D3D12_FILTER filter;
		switch(parameter.filter)
		{
		case Texture::Filter::Nearest:
			filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		case Texture::Filter::Linear:
			filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		}

		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();

		// Create sampler
		_samplerDesc = {};
		_samplerDesc.Filter = filter;
		_samplerDesc.AddressU = addressMode;
		_samplerDesc.AddressV = addressMode;
		_samplerDesc.AddressW = addressMode;
		_samplerDesc.MipLODBias = 0.0f;
		_samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		_samplerDesc.MinLOD = 0.0f;
		_samplerDesc.MaxLOD = _descriptor.mipMaps;
		_samplerDesc.MaxAnisotropy = parameter.anisotropy;
		_samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		_samplerDesc.ShaderRegister = 0;
		_samplerDesc.RegisterSpace = 0;
		_samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	void D3D12Texture::GenerateMipMaps()
	{
		_needsMipMaps = true;
		_renderer->CreateMipMapsForTexture(this);
	}

	bool D3D12Texture::HasColorChannel(ColorChannel channel) const
	{
		return true;

/*#define ColorChannel(format, r, g, b, a) \
		case format: \
		{ \
			switch(channel) \
			{ \
				case ColorChannel::Red: \
					return r; \
				case ColorChannel::Green: \
					return g; \
				case ColorChannel::Blue: \
					return b; \
				case ColorChannel::Alpha: \
					return a; \
			} \
			return false; \
		}

		switch(GetFormat())
		{
			ColorChannel(VK_FORMAT_R8G8B8A8_UNORM, true, true, true, true)
			ColorChannel(VK_FORMAT_A2R10G10B10_UNORM_PACK32, true, true, true, true)
			ColorChannel(VK_FORMAT_R8_UNORM, true, false, false, false)
			ColorChannel(VK_FORMAT_R8G8_UNORM, true, true, false, false)

			ColorChannel(VK_FORMAT_R16_SFLOAT, true, false, false, false)
			ColorChannel(VK_FORMAT_R16G16_SFLOAT, true, true, false, false)
			ColorChannel(VK_FORMAT_R16G16B16A16_SFLOAT, true, true, true, true)

			ColorChannel(VK_FORMAT_R32_SFLOAT, true, false, false, false)
			ColorChannel(VK_FORMAT_R32G32_SFLOAT, true, true, false, false)
			ColorChannel(VK_FORMAT_R32G32B32A32_SFLOAT, true, true, true, true)

			ColorChannel(VK_FORMAT_D32_SFLOAT, false, false, false, false)
			ColorChannel(VK_FORMAT_S8_UINT, false, false, false, false)
			ColorChannel(VK_FORMAT_D24_UNORM_S8_UINT, false, false, false, false)
			ColorChannel(VK_FORMAT_D32_SFLOAT_S8_UINT, false, false, false, false)

			default:
				return false;
		}*/
	}

	void D3D12Texture::TransitionToState(D3D12CommandList *commandList, D3D12_RESOURCE_STATES targetState)
	{
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_textureBuffer, _currentState, targetState));
		_currentState = targetState;
	}
}
