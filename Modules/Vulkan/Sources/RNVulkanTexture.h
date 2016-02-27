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
		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer);
		VKAPI VulkanTexture(const Descriptor &descriptor, VulkanRenderer *renderer, VkImage image, VkImageView imageView);
		VKAPI ~VulkanTexture();

		VKAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		VKAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;
		VKAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const final;

		VKAPI void SetParameter(const Parameter &parameter) final;

		VKAPI void GenerateMipMaps() final;
		VKAPI bool HasColorChannel(ColorChannel channel) const final;

		VkImage GetImage() const { return _image; }
		VkImageView GetImageView() const { return _imageView; }
		VkFormat GetFormat() const { return _format; }

		VKAPI static void SetImageLayout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);

	private:
		VulkanRenderer *_renderer;

		VkFormat _format;

		VkImage _image;
		VkImageView _imageView;

		VkDeviceMemory _memory;

		RNDeclareMetaAPI(VulkanTexture, VKAPI);
	};
}


#endif /* __RAYNE_VULKANTEXTURE_H_ */
