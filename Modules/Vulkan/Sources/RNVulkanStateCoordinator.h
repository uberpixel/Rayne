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
	RNExceptionType(VulkanStructArgumentUnsupported)

	class VulkanFramebuffer;
	class VulkanDynamicBufferReference;
	class VulkanShader;

	struct VulkanUniformState
	{
		std::vector<Shader::ArgumentBuffer*> constantBufferToArgumentMapping;
		VulkanDynamicBufferReference *instanceAttributesBuffer;
		Shader::ArgumentBuffer *instanceAttributesArgumentBuffer;
		std::vector<VulkanDynamicBufferReference *> vertexConstantBuffers;
		std::vector<VulkanDynamicBufferReference *> fragmentConstantBuffers;

		VkDescriptorSet descriptorSet;

		VulkanUniformState();
		~VulkanUniformState();
	};

	struct VulkanDepthStencilState
	{
		VulkanDepthStencilState() = default;

		~VulkanDepthStencilState()
		{

		}

		DepthMode mode;
		bool depthWriteEnabled;

		RN_INLINE bool MatchesMaterial(Material *material) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled);
		}
	};

	struct VulkanRootSignature
	{
		~VulkanRootSignature();

		std::vector<uint16> bindingIndex;
		std::vector<uint8> bindingType;
		Array *samplers;

		uint8 textureCount;
		uint8 constantBufferCount;

		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;
	};

	struct VulkanPipelineStateDescriptor
	{
		Shader::UsageHint shaderHint;

		uint8 sampleCount;
		//uint8 sampleQuality;
		VkRenderPass renderPass;
		VkFormat depthStencilFormat;
		
		uint8 colorWriteMask;
		DepthMode depthMode;
		bool depthWriteEnabled;

		Shader *vertexShader;
		Shader *fragmentShader;

		CullMode cullMode;
		bool usePolygonOffset;
		float polygonOffsetFactor;
		float polygonOffsetUnits;

		bool useAlphaToCoverage;

		BlendOperation blendOperationRGB;
		BlendOperation blendOperationAlpha;
		BlendFactor blendFactorSourceRGB;
		BlendFactor blendFactorSourceAlpha;
		BlendFactor blendFactorDestinationRGB;
		BlendFactor blendFactorDestinationAlpha;
	};

	struct VulkanPipelineState
	{
		~VulkanPipelineState();

		VulkanPipelineStateDescriptor descriptor;
		const VulkanRootSignature *rootSignature;

        uint8 vertexAttributeBufferCount; //This should maybe go into the descriptor and be checked cause pipelines could differ with this now!?

		VkPipeline state;
	};

	struct VulkanPipelineStateCollection
	{
		VulkanPipelineStateCollection() = default;
		VulkanPipelineStateCollection(const Mesh::VertexDescriptor &tdescriptor, Shader *vertex, Shader *fragment) :
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
		Shader *vertexShader;
		Shader *fragmentShader;

		std::vector<VulkanPipelineState *> states;
	};

	struct VulkanRenderPassState
	{
		RenderPass::Flags flags;
		uint8 multiviewCount;
		bool hasFragmentDensityMap;
		std::vector<VkFormat> imageFormats;
		std::vector<VkFormat> resolveFormats;
		VkRenderPass renderPass;

		RN_INLINE bool operator==(const VulkanRenderPassState &descriptor)
		{
			if(imageFormats.size() != descriptor.imageFormats.size()) return false;
			if(resolveFormats.size() != descriptor.resolveFormats.size()) return false;
			if(flags != descriptor.flags) return false;
			if(multiviewCount != descriptor.multiviewCount) return false;

			for(int i = 0; i < imageFormats.size(); i++)
			{
				if(imageFormats[i] != descriptor.imageFormats[i]) return false;
			}

			for(int i = 0; i < resolveFormats.size(); i++)
			{
				if(resolveFormats[i] != descriptor.resolveFormats[i]) return false;
			}

			return true;
		}
	};

	class VulkanStateCoordinator
	{
	public:
		VulkanStateCoordinator();
		~VulkanStateCoordinator();

		const VulkanRootSignature *GetRootSignature(const VulkanPipelineStateDescriptor &pipelineDescriptor);
		const VulkanPipelineState *GetRenderPipelineState(Material *material, Mesh *mesh, VulkanFramebuffer *framebuffer, VulkanFramebuffer *resolveFramebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial, RenderPass::Flags flags, uint8 multiviewCount);
		VulkanUniformState *GetUniformStateForPipelineState(const VulkanPipelineState *pipelineState);
		VulkanRenderPassState *GetRenderPassState(const VulkanFramebuffer *framebuffer, const VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags, uint8 multiviewCount);

	private:
		std::vector<VkVertexInputAttributeDescription> CreateVertexElementDescriptorsFromMesh(Mesh *mesh, VulkanShader *vertexShader, bool &vertexPositionsOnly);
		const VulkanPipelineState *GetRenderPipelineStateInCollection(VulkanPipelineStateCollection *collection, Mesh *mesh, const VulkanPipelineStateDescriptor &pipelineDescriptor);

		std::vector<VulkanDepthStencilState *> _depthStencilStates;
		const VulkanDepthStencilState *_lastDepthStencilState;

		std::vector<VulkanRenderPassState*> _renderPassStates;
		std::vector<VulkanPipelineStateCollection *> _renderingStates;
		std::vector<VulkanRootSignature *> _rootSignatures;
	};
}


#endif /* __RAYNE_VULKANSTATECOORDINATOR_H_ */
