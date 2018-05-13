//
//  RNVulkanInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINTERNALS_H__
#define __RAYNE_VULKANINTERNALS_H__

#include "RNVulkan.h"
#include "RNVulkanStateCoordinator.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanSwapChain.h"

namespace RN
{
	struct VulkanDrawable : public Drawable
	{
		struct CameraSpecific
		{
			const VulkanPipelineState *pipelineState;
			VulkanUniformState *uniformState; //TODO: Check if needs to be deleted when done
			bool dirty;
		};

		~VulkanDrawable(){}

		void AddUniformStateIfNeeded(size_t cameraID)
		{
			while(_cameraSpecifics.size() <= cameraID)
			{
				_cameraSpecifics.push_back({ nullptr, nullptr, true });
			}
		}

		void UpdateRenderingState(size_t cameraID, const VulkanPipelineState *pipelineState, VulkanUniformState *uniformState)
		{
			_cameraSpecifics[cameraID].pipelineState = pipelineState;
			_cameraSpecifics[cameraID].uniformState = uniformState;
			_cameraSpecifics[cameraID].dirty = false;
		}

		virtual void MakeDirty() override
		{
			for(int i = 0; i < _cameraSpecifics.size(); i++)
			{
				_cameraSpecifics[i].dirty = true;
			}
		}

		//TODO: This can get somewhat big with lots of post processing stages...
		std::vector<CameraSpecific> _cameraSpecifics;
	};

	struct VulkanLightDirectional
	{
		Vector3 direction;
		float padding;
		Color color;
	};

	struct VulkanRenderPass
	{
		enum Type
		{
			Default,
			ResolveMSAA,
			Blit,
			Convert
		};

		Type type;
		RenderPass *renderPass;
		RenderPass *previousRenderPass;

		VulkanFramebuffer *framebuffer;
		Shader::UsageHint shaderHint;
		Material *overrideMaterial;

		Vector3 viewPosition;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		std::vector<VulkanDrawable *> drawables;
		std::vector<VulkanLightDirectional> directionalLights;

		std::vector<Matrix> directionalShadowMatrices;
		VulkanTexture *directionalShadowDepthTexture;
		Vector2 directionalShadowInfo;
	};

	struct VulkanRendererInternals
	{
		std::vector<VulkanRenderPass> renderPasses;
		VulkanStateCoordinator stateCoordinator;

		std::vector<VulkanSwapChain*> swapChains;

		size_t currentRenderPassIndex;
		size_t currentDrawableResourceIndex;
		size_t totalDrawableCount;

		size_t totalDescriptorTables;
	};

	class VulkanCommandBuffer : public Object
	{
	public:
		friend RN::VulkanRenderer;

		~VulkanCommandBuffer();

		void Begin();
		void End();

		VkCommandBuffer GetCommandBuffer() const {return _commandBuffer;}

	protected:
		VulkanCommandBuffer(VkDevice device, VkCommandPool pool);

	private:
		VkCommandBuffer _commandBuffer;
		VkDevice _device;
		VkCommandPool _pool;

		RNDeclareMetaAPI(VulkanCommandBuffer, VKAPI)
	};

	class VulkanCommandBufferWithCallback : public VulkanCommandBuffer
	{
	public:
		friend RN::VulkanRenderer;

		~VulkanCommandBufferWithCallback();
		void SetFinishedCallback(std::function<void()> callback);

	protected:
		VulkanCommandBufferWithCallback(VkDevice device, VkCommandPool pool);

	private:
		std::function<void()> _finishedCallback;

		RNDeclareMetaAPI(VulkanCommandBufferWithCallback, VKAPI)
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */
