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

#include <vk_mem_alloc.h>

namespace RN
{
	class VulkanRenderer;
	class VulkanTexture : public Texture
	{
	public:
		friend class VulkanRenderer;

		enum BarrierIntent
		{
			UploadSource,
			UploadDestination,
			CopySource,
			CopyDestination,
			ShaderSource,
			RenderTarget,
			ExternalSource
		};

		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer);
		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image, bool fromSwapchain);
		VKAPI ~VulkanTexture() override;

		VKAPI void StartStreamingData(const Region &region) override;
		VKAPI void StopStreamingData() override;

		VKAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow, size_t numberOfRows) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow, size_t numberOfRows) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow, size_t numberOfRows) final;
		VKAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow, std::function<void(void)> callback) const final;

		VKAPI void GenerateMipMaps() final;

		VkImage GetVulkanImage() const { return _image; }
		VkFormat GetVulkanFormat() const { return _format; }

/*		VKAPI void TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask);
		VKAPI void TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout);*/

		VkImageLayout GetCurrentLayout() const { return _currentLayout; }
		void SetCurrentLayout(VkImageLayout layout) { _currentLayout = layout; }

		static VkFormat VulkanImageFormatFromTextureFormat(Texture::Format format);

		VKAPI static void SetImageLayout(VkCommandBuffer buffer, VkImage image, uint32 baseMipmap, uint32 mipmapCount, uint32 baseLayer, uint32 layerCount, VkImageAspectFlags aspectMask, VkImageLayout fromLayout, VkImageLayout toLayout, BarrierIntent intent);

	private:
		VulkanRenderer *_renderer;

		VkImage _uploadImage;
		VmaAllocation _uploadAllocation;
		VkSubresourceLayout _uploadSubresourceLayout;
		void *_uploadData;
		bool _isFirstUpload;
		bool _isFromSwapchain;

		VkFormat _format;
		VkImage _image;
		VkImageView _imageView;
		VmaAllocation _allocation;
		VkMemoryRequirements _requirements;
		VkImageLayout _currentLayout;

		RNDeclareMetaAPI(VulkanTexture, VKAPI);
	};
}


#endif /* __RAYNE_VULKANTEXTURE_H_ */
