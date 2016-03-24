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
	struct VulkanRenderingState
	{
		~VulkanRenderingState()
		{
			//vk::DestroyPipeline(device, state, nullptr);
		}

		VkPipeline state;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
		VkPipelineLayout pipelineLayout;

		GPUBuffer *uniformBuffer;
	};

	class VulkanShader;
	struct VulkanRenderingStateCollection
	{
		VulkanRenderingStateCollection() = default;
		VulkanRenderingStateCollection(const Mesh::VertexDescriptor &tdescriptor, VulkanShader *vertex, VulkanShader *fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex),
			fragmentShader(fragment)
		{}

		~VulkanRenderingStateCollection()
		{
			for(VulkanRenderingState *state : states)
				delete state;
		}

		Mesh::VertexDescriptor descriptor;
		class VulkanShader *vertexShader;
		class VulkanShader *fragmentShader;

		std::vector<VulkanRenderingState *> states;
	};


	class VulkanRenderer;
	class VulkanStateCoordinator
	{
	public:
		VKAPI VulkanStateCoordinator();
		VKAPI ~VulkanStateCoordinator();

		VKAPI const VulkanRenderingState *GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera);
		VKAPI void SetRenderer(VulkanRenderer *renderer);

	private:
		const VulkanRenderingState *GetRenderPipelineStateInCollection(VulkanRenderingStateCollection *collection, Mesh *mesh, Camera *camera);

		VulkanRenderer *_renderer;
		VkDescriptorPool _descriptorPool;

		std::vector<VulkanRenderingStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_VULKANSTATECOORDINATOR_H_ */
