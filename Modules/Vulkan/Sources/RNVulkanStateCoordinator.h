//
//  VulkanStateCoordinator.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANSTATECOORDINATOR_H_
#define __RAYNE_VULKANSTATECOORDINATOR_H_

#include "RNVulkan.h"

namespace RN
{
	struct VulkanPipelineState
	{
		~VulkanPipelineState()
		{
			//vk::DestroyPipeline(device, state, nullptr);
		}

		VkPipeline state;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
		uint8 textureCount;
	};

	struct VulkanUniformState
	{
		VkDescriptorSet descriptorSet;
		GPUBuffer *uniformBuffer;
	};

	class VulkanShader;
	struct VulkanPipelineStateCollection
	{
		VulkanPipelineStateCollection() = default;
		VulkanPipelineStateCollection(const Mesh::VertexDescriptor &tdescriptor, VulkanShader *vertex, VulkanShader *fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex),
			fragmentShader(fragment)
		{}

		~VulkanPipelineStateCollection()
		{
			for(VulkanPipelineState *state : states)
				delete state;
		}

		Mesh::VertexDescriptor descriptor;
		class VulkanShader *vertexShader;
		class VulkanShader *fragmentShader;

		std::vector<VulkanPipelineState *> states;
	};


	class VulkanRenderer;
	class VulkanStateCoordinator
	{
	public:
		VKAPI VulkanStateCoordinator();
		VKAPI ~VulkanStateCoordinator();

		VKAPI const VulkanPipelineState *GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera);
		VKAPI VulkanUniformState *GetUniformStateForPipelineState(const VulkanPipelineState *pipelineState, Material *material);
		VKAPI void SetRenderer(VulkanRenderer *renderer);

	private:
		const VulkanPipelineState *GetRenderPipelineStateInCollection(VulkanPipelineStateCollection *collection, Mesh *mesh, Material *material, Camera *camera);

		VulkanRenderer *_renderer;
		VkDescriptorPool _descriptorPool;

		std::vector<VulkanPipelineStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_VULKANSTATECOORDINATOR_H_ */
