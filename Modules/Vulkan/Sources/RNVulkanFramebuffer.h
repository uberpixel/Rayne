//
//  RNVulkanFramebuffer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANFRAMEBUFFER_H_
#define __RAYNE_VULKANFRAMEBUFFER_H_

#include <Rayne.h>
#include "RNVulkan.h"
#include "RNVulkanTexture.h"

namespace RN
{
	class VulkanRenderer;

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		struct VulkanTargetView
		{
			TargetView targetView;
		};

		VKAPI VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer);
		VKAPI VulkanFramebuffer(const Vector2 &size, VkSwapchainKHR swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat);
		VKAPI ~VulkanFramebuffer();

		VKAPI void SetColorTarget(const TargetView &target, uint32 index = 0) final;
		VKAPI void SetDepthStencilTarget(const TargetView &target) final;

		VKAPI Texture *GetColorTexture(uint32 index = 0) const final;
		VKAPI Texture *GetDepthStencilTexture() const final;

		VKAPI virtual uint8 GetSampleCount() const final;

	private:
		VulkanRenderer *_renderer;
		uint8 _sampleCount;

		std::vector<VulkanTargetView *> _colorTargets;
		VulkanTargetView *_depthStencilTarget;

		RNDeclareMetaAPI(VulkanFramebuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANFRAMEBUFFER_H_ */
