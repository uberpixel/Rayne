//
//  RNVulkanTexture.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANTEXTURE_H_
#define __RAYNE_VULKANTEXTURE_H_

#include "RNVulkan.h"

namespace RN
{
	class VulkanRenderer;
	class VulkanTexture : public Texture
	{
	public:
		friend class VulkanRenderer;

		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer);
		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image);
		VKAPI ~VulkanTexture() override;

		VKAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;
		VKAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const final;

		VKAPI void GenerateMipMaps() final;

		VkImage GetVulkanImage() const { return _image; }
		VkFormat GetVulkanFormat() const { return _format; }

/*		VKAPI void TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask);
		VKAPI void TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout);*/

		static VkFormat VulkanImageFormatFromTextureFormat(Texture::Format format);

		VKAPI static void SetImageLayout(VkCommandBuffer buffer, VkImage image, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask, VkImageLayout fromLayout, VkImageLayout toLayout);

	private:
/*		D3D12Renderer *_renderer;
		D3D12StateCoordinator *_coordinator;

		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDescriptor;
		ID3D12Resource *_resource;
		D3D12_RESOURCE_STATES _currentState;

		bool _isReady;
		bool _needsMipMaps;*/

		VulkanRenderer *_renderer;

		VkFormat _format;
		VkImage _image;
		VkDeviceMemory _memory;
		VkMemoryRequirements _requirements;
		VkImageLayout _currentLayout;

		RNDeclareMetaAPI(VulkanTexture, VKAPI);
	};
}


#endif /* __RAYNE_VULKANTEXTURE_H_ */
