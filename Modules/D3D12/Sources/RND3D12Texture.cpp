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

	//TODO: Place these into a utility header or something
	static D3D12_RESOURCE_DIMENSION D3D12ImageTypeFromTextureType(Texture::Type type)
	{
		switch(type)
		{
		case Texture::Type::Type1D:
		case Texture::Type::Type1DArray:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

		case Texture::Type::Type2D:
		case Texture::Type::Type2DArray:
		case Texture::Type::TypeCube:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		case Texture::Type::Type3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;

		default:
			throw InconsistencyException("Invalid texture type for D3D12");
		}
	}

	static DXGI_FORMAT D3D12TypelessFormatFromDepthFormat(Texture::Format format)
	{
		switch(format)
		{
			case Texture::Format::Depth_16I:
				return DXGI_FORMAT_R16_TYPELESS;
			case Texture::Format::Depth_24I:
				return DXGI_FORMAT_R24G8_TYPELESS;
			case Texture::Format::Depth_32F:
				return DXGI_FORMAT_R32_TYPELESS;
			case Texture::Format::Stencil_8:
				return DXGI_FORMAT_R24G8_TYPELESS;
			case Texture::Format::Depth_24_Stencil_8:
				return DXGI_FORMAT_R24G8_TYPELESS;
			case Texture::Format::Depth_32F_Stencil_8:
				return DXGI_FORMAT_R32G8X24_TYPELESS;
			default:
				return DXGI_FORMAT_UNKNOWN;
		}
	}

	DXGI_FORMAT D3D12Texture::ImageFormatFromTextureFormat(Texture::Format format)
	{
		switch (format)
		{
		case Texture::Format::RGBA_8_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Texture::Format::BGRA_8_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case Texture::Format::RGBA_8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::BGRA_8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::Format::RGB_10_A_2:
		case Texture::Format::BGR_10_A_2:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Texture::Format::R_8:
			return DXGI_FORMAT_R8_UNORM;
		case Texture::Format::RG_8:
			return DXGI_FORMAT_R8G8_UNORM;
		case Texture::Format::RGB_8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::RGB_8_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Texture::Format::R_16F:
			return DXGI_FORMAT_R16_FLOAT;
		case Texture::Format::RG_16F:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Texture::Format::RGB_16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::Format::RGBA_16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::Format::R_32F:
			return DXGI_FORMAT_R32_FLOAT;
		case Texture::Format::RG_32F:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Texture::Format::RGB_32F:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Texture::Format::RGBA_32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case Texture::Format::RGBA_BC1_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case Texture::Format::RGBA_BC2_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case Texture::Format::RGBA_BC3_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case Texture::Format::RGBA_BC7_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;

		case Texture::Format::RGBA_BC1:
			return DXGI_FORMAT_BC1_UNORM;
		case Texture::Format::RGBA_BC2:
			return DXGI_FORMAT_BC2_UNORM;
		case Texture::Format::RGBA_BC3:
			return DXGI_FORMAT_BC3_UNORM;
		case Texture::Format::RGBA_BC4:
			return DXGI_FORMAT_BC4_UNORM;
		case Texture::Format::RGBA_BC7:
			return DXGI_FORMAT_BC7_UNORM;

		case Texture::Format::Depth_16I:
			return DXGI_FORMAT_D16_UNORM;
		case Texture::Format::Depth_24I:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::Format::Depth_32F:
			return DXGI_FORMAT_D32_FLOAT;
		case Texture::Format::Stencil_8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::Format::Depth_24_Stencil_8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::Format::Depth_32F_Stencil_8:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	static DXGI_FORMAT D3D12ImageShaderInputFormatFromTextureFormat(Texture::Format format)
	{
		switch (format)
		{
		case Texture::Format::RGBA_8_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Texture::Format::BGRA_8_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case Texture::Format::RGBA_8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::BGRA_8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::Format::RGB_10_A_2:
		case Texture::Format::BGR_10_A_2:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Texture::Format::R_8:
			return DXGI_FORMAT_R8_UNORM;
		case Texture::Format::RG_8:
			return DXGI_FORMAT_R8G8_UNORM;
		case Texture::Format::RGB_8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::RGB_8_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Texture::Format::R_16F:
			return DXGI_FORMAT_R16_FLOAT;
		case Texture::Format::RG_16F:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Texture::Format::RGB_16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::Format::RGBA_16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::Format::R_32F:
			return DXGI_FORMAT_R32_FLOAT;
		case Texture::Format::RG_32F:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Texture::Format::RGB_32F:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Texture::Format::RGBA_32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case Texture::Format::RGBA_BC1_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case Texture::Format::RGBA_BC2_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case Texture::Format::RGBA_BC3_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case Texture::Format::RGBA_BC7_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;

		case Texture::Format::RGBA_BC1:
			return DXGI_FORMAT_BC1_UNORM;
		case Texture::Format::RGBA_BC2:
			return DXGI_FORMAT_BC2_UNORM;
		case Texture::Format::RGBA_BC3:
			return DXGI_FORMAT_BC3_UNORM;
		case Texture::Format::RGBA_BC4:
			return DXGI_FORMAT_BC4_UNORM;
		case Texture::Format::RGBA_BC7:
			return DXGI_FORMAT_BC7_UNORM;

		case Texture::Format::Depth_16I:
			return DXGI_FORMAT_R16_UNORM;
		case Texture::Format::Depth_24I:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case Texture::Format::Depth_32F:
			return DXGI_FORMAT_R32_FLOAT;
		case Texture::Format::Stencil_8:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

		//TODO: Only depth of DepthStencil formats it currently accesible in shader, two resource views with different formats would be needed
		case Texture::Format::Depth_24_Stencil_8:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case Texture::Format::Depth_32F_Stencil_8:
			return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	static D3D12_RESOURCE_FLAGS D3D12TextureFlagsFromTextureDescriptor(const Texture::Descriptor &descriptor)
	{
		if(descriptor.usageHint & Texture::UsageHint::RenderTarget)
		{
			switch(descriptor.format)
			{
			case Texture::Format::RGBA_8_SRGB:
			case Texture::Format::BGRA_8_SRGB:
			case Texture::Format::RGBA_8:
			case Texture::Format::BGRA_8:
			case Texture::Format::RGB_10_A_2:
			case Texture::Format::BGR_10_A_2:
			case Texture::Format::R_8:
			case Texture::Format::RG_8:
			case Texture::Format::RGB_8:
			case Texture::Format::RGB_8_SRGB:
			case Texture::Format::R_16F:
			case Texture::Format::RG_16F:
			case Texture::Format::RGB_16F:
			case Texture::Format::RGBA_16F:
			case Texture::Format::R_32F:
			case Texture::Format::RG_32F:
			case Texture::Format::RGB_32F:
			case Texture::Format::RGBA_32F:
				return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			case Texture::Format::Depth_16I:
			case Texture::Format::Depth_24I:
			case Texture::Format::Depth_32F:
			case Texture::Format::Stencil_8:
			case Texture::Format::Depth_24_Stencil_8:
			case Texture::Format::Depth_32F_Stencil_8:
				return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			default:
				return D3D12_RESOURCE_FLAG_NONE;
			}
		}

		return D3D12_RESOURCE_FLAG_NONE;
	}

	static D3D12_RESOURCE_STATES D3D12TextureStateFromTextureDescriptor(const Texture::Descriptor &descriptor)
	{
		if (descriptor.usageHint & Texture::UsageHint::RenderTarget)
		{
			switch (descriptor.format)
			{
			case Texture::Format::RGBA_8_SRGB:
			case Texture::Format::BGRA_8_SRGB:
			case Texture::Format::RGBA_8:
			case Texture::Format::BGRA_8:
			case Texture::Format::RGB_10_A_2:
			case Texture::Format::BGR_10_A_2:
			case Texture::Format::R_8:
			case Texture::Format::RG_8:
			case Texture::Format::RGB_8:
			case Texture::Format::RGB_8_SRGB:
			case Texture::Format::R_16F:
			case Texture::Format::RG_16F:
			case Texture::Format::RGB_16F:
			case Texture::Format::RGBA_16F:
			case Texture::Format::R_32F:
			case Texture::Format::RG_32F:
			case Texture::Format::RGB_32F:
			case Texture::Format::RGBA_32F:
				return D3D12_RESOURCE_STATE_RENDER_TARGET;

			case Texture::Format::Depth_16I:
			case Texture::Format::Depth_24I:
			case Texture::Format::Depth_32F:
			case Texture::Format::Stencil_8:
			case Texture::Format::Depth_24_Stencil_8:
			case Texture::Format::Depth_32F_Stencil_8:
				return D3D12_RESOURCE_STATE_DEPTH_WRITE;

			default:
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
		}

		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}


	D3D12Texture::D3D12Texture(const Descriptor &descriptor, D3D12Renderer *renderer) :
		Texture(descriptor),
		_renderer(renderer),
		_isReady(false),
		_needsMipMaps(false), 
		_srvDescriptor{},
		_allocation(nullptr),
		_resource(nullptr)
	{
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();

		_srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_srvDescriptor.Format = D3D12ImageShaderInputFormatFromTextureFormat(descriptor.format);
		
		//TODO: Support multisampled types and cubemaps
		switch(descriptor.type)
		{
			case Texture::Type::Type1D:
				_srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
				_srvDescriptor.Texture1D.MipLevels = descriptor.mipMaps;
				_srvDescriptor.Texture1D.MostDetailedMip = 0;
				_srvDescriptor.Texture1D.ResourceMinLODClamp = 0.0f;
				break;
			case Texture::Type::Type1DArray:
				_srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				_srvDescriptor.Texture1DArray.MipLevels = descriptor.mipMaps;
				_srvDescriptor.Texture1DArray.MostDetailedMip = 0;
				_srvDescriptor.Texture1DArray.ResourceMinLODClamp = 0.0f;
				_srvDescriptor.Texture1DArray.FirstArraySlice = 0;
				_srvDescriptor.Texture1DArray.ArraySize = descriptor.depth;
				break;

			case Texture::Type::Type2D:
				_srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				_srvDescriptor.Texture2D.MipLevels = descriptor.mipMaps;
				_srvDescriptor.Texture2D.MostDetailedMip = 0;
				_srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
				_srvDescriptor.Texture2D.PlaneSlice = 0;
				break;

			case Texture::Type::Type2DArray:
				_srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				_srvDescriptor.Texture2DArray.MipLevels = descriptor.mipMaps;
				_srvDescriptor.Texture2DArray.MostDetailedMip = 0;
				_srvDescriptor.Texture2DArray.ResourceMinLODClamp = 0.0f;
				_srvDescriptor.Texture2DArray.PlaneSlice = 0;
				_srvDescriptor.Texture2DArray.FirstArraySlice = 0;
				_srvDescriptor.Texture2DArray.ArraySize = descriptor.depth;
				break;

			case Texture::Type::Type3D:
				_srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				_srvDescriptor.Texture3D.MipLevels = descriptor.mipMaps;
				_srvDescriptor.Texture3D.MostDetailedMip = 0;
				_srvDescriptor.Texture3D.ResourceMinLODClamp = 0.0f;
				break;

			case Texture::Type::TypeCube:
				_srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				_srvDescriptor.TextureCube.MipLevels = descriptor.mipMaps;
				_srvDescriptor.TextureCube.MostDetailedMip = 0;
				_srvDescriptor.TextureCube.ResourceMinLODClamp = 0.0f;
				break;
		}

		D3D12_RESOURCE_DESC imageDesc = {};
		imageDesc.Dimension = D3D12ImageTypeFromTextureType(descriptor.type);
		imageDesc.Alignment = 0;
		imageDesc.Width = descriptor.width;
		imageDesc.Height = descriptor.height;
		imageDesc.DepthOrArraySize = descriptor.depth;
		imageDesc.MipLevels = descriptor.mipMaps;
		imageDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(descriptor.format);
		imageDesc.SampleDesc.Count = descriptor.sampleCount;
		imageDesc.SampleDesc.Quality = 0;
		imageDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		imageDesc.Flags = D3D12TextureFlagsFromTextureDescriptor(descriptor);

		if(descriptor.usageHint & UsageHint::RenderTarget)
		{
			D3D12_CLEAR_VALUE clearValue = {};
			clearValue.Format = imageDesc.Format;

			//TODO: descriptor.usageHint & UsageHint::ShaderRead could be checked here, but makes error handling tricky and maybe there is no disadvantage in always using a typeless format here...
			if(descriptor.format >= Texture::Format::Depth_16I)
			{
				imageDesc.Format = D3D12TypelessFormatFromDepthFormat(descriptor.format);
				clearValue.DepthStencil.Depth = descriptor.preferredClearDepth;
				clearValue.DepthStencil.Stencil = descriptor.preferredClearStencil;
			}
			else
			{
				clearValue.Color[0] = descriptor.preferredClearColor.r;
				clearValue.Color[1] = descriptor.preferredClearColor.g;
				clearValue.Color[2] = descriptor.preferredClearColor.b;
				clearValue.Color[3] = descriptor.preferredClearColor.a;
			}

			// create the final texture buffer
			_currentState = D3D12TextureStateFromTextureDescriptor(descriptor);

			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			_renderer->GetD3D12Device()->GetMemoryAllocator()->CreateResource(&allocationDesc, &imageDesc, _currentState, &clearValue, &_allocation, IID_NULL, NULL);
			_resource = _allocation->GetResource();

			_isReady = true;
		}
		else
		{
			// create the final texture buffer
			_currentState = D3D12_RESOURCE_STATE_COPY_DEST;
			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			_renderer->GetD3D12Device()->GetMemoryAllocator()->CreateResource(&allocationDesc, &imageDesc, _currentState, NULL, &_allocation, IID_NULL, NULL);
			_resource = _allocation->GetResource();
		}
	}

	D3D12Texture::~D3D12Texture()
	{
		D3D12Renderer *renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		renderer->AddFrameResouce(_allocation);
	}

	void D3D12Texture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow, size_t numberOfRows)
	{
		SetData(Region(0, 0, 0, _descriptor.GetWidthForMipMapLevel(mipmapLevel), _descriptor.GetHeightForMipMapLevel(mipmapLevel), _descriptor.depth), mipmapLevel, bytes, bytesPerRow, numberOfRows);
	}

	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow, size_t numberOfRows)
	{
		SetData(region, mipmapLevel, 0, bytes, bytesPerRow, numberOfRows);
	}
	
	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow, size_t numberOfRows)
	{
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();

		D3D12_RESOURCE_DESC imageDesc = {};
		imageDesc.Dimension = D3D12ImageTypeFromTextureType(_descriptor.type);
		imageDesc.Alignment = 0;
		imageDesc.Width = std::max(region.width, 4u); //TODO: Not sure if this works for the last two mipmap levels, but needed for alignment requirements or something...
		imageDesc.Height = std::max(region.height, 4u);
		imageDesc.DepthOrArraySize = region.depth;
		imageDesc.MipLevels = 1;
		imageDesc.Format = _srvDescriptor.Format;
		imageDesc.SampleDesc.Count = 1;
		imageDesc.SampleDesc.Quality = 0;
		imageDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		imageDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// create a temporary buffer to upload the texture from
		D3D12MA::Allocation *textureUploadAllocation;
		UINT64 textureUploadBufferSize;
		device->GetCopyableFootprints(&imageDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		_renderer->GetD3D12Device()->GetMemoryAllocator()->CreateResource(&allocationDesc, &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &textureUploadAllocation, IID_NULL, NULL);

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = bytes;
		textureData.RowPitch = bytesPerRow;
		textureData.SlicePitch = bytesPerRow * numberOfRows;

		D3D12CommandList *commandList = _renderer->GetCommandList();
		commandList->SetFinishedCallback([this, textureUploadAllocation]{
			_isReady = true;
			textureUploadAllocation->Release();
		});

		TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

		// Now we copy the upload buffer contents to the default heap
		UpdateSubresources(commandList->GetCommandList(), _resource, textureUploadAllocation->GetResource(), 0, slice * _descriptor.mipMaps + mipmapLevel, 1, &textureData);

		// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
		TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Now we execute the command list to upload the initial assets (triangle data)
		commandList->End();
		_renderer->SubmitCommandList(commandList);
	}

	void D3D12Texture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow, std::function<void(void)> callback) const
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

	void D3D12Texture::GenerateMipMaps()
	{
		_needsMipMaps = true;
		_renderer->CreateMipMapsForTexture(this);
	}

	bool D3D12Texture::HasColorChannel(ColorChannel channel) const
	{
#define ColorChannel(format, r, g, b, a) \
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

		switch(_srvDescriptor.Format)
		{
			ColorChannel(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, true, true, true, true)
			ColorChannel(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, true, true, true, true)
			ColorChannel(DXGI_FORMAT_R8G8B8A8_UNORM, true, true, true, true)
			ColorChannel(DXGI_FORMAT_B8G8R8A8_UNORM, true, true, true, true)
			ColorChannel(DXGI_FORMAT_R10G10B10A2_UNORM, true, true, true, true)
			ColorChannel(DXGI_FORMAT_R8_UNORM, true, false, false, false)
			ColorChannel(DXGI_FORMAT_R8G8_UNORM, true, true, false, false)

			ColorChannel(DXGI_FORMAT_R16_FLOAT, true, false, false, false)
			ColorChannel(DXGI_FORMAT_R16G16_FLOAT, true, true, false, false)
			ColorChannel(DXGI_FORMAT_R16G16B16A16_FLOAT, true, true, true, true)

			ColorChannel(DXGI_FORMAT_R32_FLOAT, true, false, false, false)
			ColorChannel(DXGI_FORMAT_R32G32_FLOAT, true, true, false, false)
			ColorChannel(DXGI_FORMAT_R32G32B32_FLOAT, true, true, true, false)
			ColorChannel(DXGI_FORMAT_R32G32B32A32_FLOAT, true, true, true, true)

			ColorChannel(DXGI_FORMAT_BC1_UNORM_SRGB, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC2_UNORM_SRGB, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC3_UNORM_SRGB, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC7_UNORM_SRGB, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC1_UNORM, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC2_UNORM, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC3_UNORM, true, true, true, true)
			ColorChannel(DXGI_FORMAT_BC4_UNORM, true, false, false, false)
			ColorChannel(DXGI_FORMAT_BC7_UNORM, true, true, true, true)

			default:
				return false;
		}
	}

	void D3D12Texture::TransitionToState(D3D12CommandList *commandList, D3D12_RESOURCE_STATES targetState)
	{
		if (_currentState == targetState) return;

		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_resource, _currentState, targetState));
		_currentState = targetState;
	}
}
