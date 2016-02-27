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
		VKAPI VulkanFramebuffer(const Vector2 &size, const Descriptor &descriptor, VkSwapchainKHR swapChain,VulkanRenderer *renderer);
		VKAPI ~VulkanFramebuffer();

		VKAPI Texture *GetColorTexture() const final;
		VKAPI Texture *GetDepthTexture() const final;
		VKAPI Texture *GetStencilTexture() const final;

		VkRenderPass GetRenderPass() const { return _renderPass; }
		VkCommandBuffer GetCommandBuffer() const;
		VkFramebuffer GetFramebuffer() const;

	private:
		struct FramebufferData
		{
			FramebufferData() :
				colorTexture(nullptr),
				depthTexture(nullptr),
				stencilTexture(nullptr)
			{}

			VkFramebuffer framebuffer;
			VkCommandBuffer commandBuffer;
			VulkanTexture *colorTexture;
			VulkanTexture *depthTexture;
			VulkanTexture *stencilTexture;
		};

		void InitializeRenderPass();

		VulkanRenderer *_renderer;

		VkRenderPass _renderPass;
		std::vector<FramebufferData *> _framebuffers;

		RNDeclareMetaAPI(VulkanFramebuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANFRAMEBUFFER_H_ */
