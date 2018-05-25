//
//  RNVulkanFramebuffer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANFRAMEBUFFER_H_
#define __RAYNE_VULKANFRAMEBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanTexture.h"
#include "RNVulkanSwapChain.h"

namespace RN
{
	class VulkanFramebuffer : public Framebuffer
	{
	public:
		friend class VulkanRenderer;
		friend class VulkanStateCoordinator;

		struct VulkanTargetView
		{
			TargetView targetView;
			VkImageViewCreateInfo vulkanTargetViewDescriptor;
		};

		VKAPI VulkanFramebuffer(const Vector2 &size, VulkanSwapChain *swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat);
		VKAPI VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer);
		VKAPI ~VulkanFramebuffer();

		VKAPI void SetColorTarget(const TargetView &target, uint32 index = 0) final;
		VKAPI void SetDepthStencilTarget(const TargetView &target) final;

		VKAPI Texture *GetColorTexture(uint32 index = 0) const final;
		VKAPI Texture *GetDepthStencilTexture() const final;
		VKAPI uint8 GetSampleCount() const final;

		VulkanSwapChain *GetSwapChain() const { return _swapChain; }

		VKAPI void WillUpdateSwapChain();
		VKAPI void DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat);

	private:
		void PrepareAsRendertargetForFrame(VulkanFramebuffer *resolveFramebuffer);
		void SetAsRendertarget(VkCommandBuffer commandBuffer, VulkanFramebuffer *resolveFramebuffer, const Color &clearColor, float depth, uint8 stencil) const;

		VulkanRenderer *_renderer;
		uint8 _sampleCount;
		uint32 _frameLastUsed;

		WeakRef<VulkanSwapChain> _swapChain;

		std::vector<VulkanTargetView *> _colorTargets;
		VulkanTargetView *_depthStencilTarget;

		//std::map<RenderPass *, VkRenderPass> _renderPasses;
		VkRenderPass _renderPass;
		VkFramebuffer _frameBuffer;

		RNDeclareMetaAPI(VulkanFramebuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANFRAMEBUFFER_H_ */
