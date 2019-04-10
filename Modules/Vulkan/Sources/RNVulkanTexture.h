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

		enum BarrierIntent
		{
			UploadSource,
			UploadDestination,
			CopySource,
			CopyDestination,
			ShaderSource,
			RenderTarget
		};

		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer);
		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image);
		VKAPI ~VulkanTexture() override;

		VKAPI void StartStreamingData(const Region &region);
		VKAPI void StopStreamingData();

		VKAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;
		VKAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const final;

		VKAPI void GenerateMipMaps() final;

		VkImage GetVulkanImage() const { return _image; }
		VkFormat GetVulkanFormat() const { return _format; }

/*		VKAPI void TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask);
		VKAPI void TransitionToLayout(VkCommandBuffer buffer, VkImageLayout targetLayout);*/

		VkImageLayout GetCurrentLayout() const { return _currentLayout; }
		void SetCurrentLayout(VkImageLayout layout) { _currentLayout = layout; }

		static VkFormat VulkanImageFormatFromTextureFormat(Texture::Format format);

		VKAPI static void SetImageLayout(VkCommandBuffer buffer, VkImage image, uint32 baseMipmap, uint32 mipmapCount, VkImageAspectFlags aspectMask, VkImageLayout fromLayout, VkImageLayout toLayout, BarrierIntent intent);

	private:
		VulkanRenderer *_renderer;

		VkImage _uploadImage;
		VkDeviceMemory _uploadMemory;
		void *_uploadData;
		bool _isFirstUpload;

		VkFormat _format;
		VkImage _image;
		VkImageView _imageView;
		VkDeviceMemory _memory;
		VkMemoryRequirements _requirements;
		VkImageLayout _currentLayout;

		RNDeclareMetaAPI(VulkanTexture, VKAPI);
	};
}


#endif /* __RAYNE_VULKANTEXTURE_H_ */
