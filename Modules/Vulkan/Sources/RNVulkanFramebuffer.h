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

		struct VulkanFramebufferVariant
		{
			//Used to check if the current vulkan framebuffer can be reused or needs to be recreated
			VulkanFramebuffer *resolveFramebuffer;
			RenderPass::Flags renderPassFlags;
			uint8 multiviewLayer;
			uint8 multiviewCount;

			uint8 swapchainImageIndex;
			uint8 resolveSwapchainImageIndex;
			uint8 fragmentDensitySwapchainImageIndex;

			VkRenderPass renderPass;
			VkFramebuffer framebuffer;

			std::vector<VkImageView> attachments;
		};

		VKAPI VulkanFramebuffer(const Vector2 &size, uint8 layerCount, VulkanSwapChain *swapChain, VulkanRenderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat, Texture::Format fragmentDensityFormat);
		VKAPI VulkanFramebuffer(const Vector2 &size, VulkanRenderer *renderer);
		VKAPI ~VulkanFramebuffer();

		VKAPI void SetColorTarget(const TargetView &target, uint32 index = 0) final;
		VKAPI void SetDepthStencilTarget(const TargetView &target) final;

		VKAPI Texture *GetColorTexture(uint32 index = 0) const final;
		VKAPI Texture *GetDepthStencilTexture() const final;
		VKAPI uint8 GetSampleCount() const final;

		VulkanSwapChain *GetSwapChain() const { return _swapChain; }

		VKAPI void WillUpdateSwapChain();
		VKAPI void DidUpdateSwapChain(Vector2 size, uint8 layerCount, Texture::Format colorFormat, Texture::Format depthStencilFormat, Texture::Format fragmentDensityFormat);

	private:
		void PrepareAsRendertargetForFrame(VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags, uint8 multiviewLayer, uint8 multiviewCount);
		void SetAsRendertarget(VkCommandBuffer commandBuffer, VulkanFramebuffer *resolveFramebuffer, const Color &clearColor, float depth, uint8 stencil) const;

		VulkanRenderer *_renderer;
		uint8 _sampleCount;
		uint32 _frameLastUsed;

		WeakRef<VulkanSwapChain> _swapChain;

		std::vector<VulkanTargetView *> _colorTargets;
		VulkanTargetView *_depthStencilTarget;
		std::vector<VulkanTargetView *> _fragmentDensityTargets;

		uint8 _currentVariantIndex;
		std::vector<VulkanFramebufferVariant> _framebufferVariants;

		RNDeclareMetaAPI(VulkanFramebuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANFRAMEBUFFER_H_ */
