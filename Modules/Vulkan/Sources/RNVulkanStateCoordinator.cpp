//
//  RNVulkanStateCoordinator.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanStateCoordinator.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanShader.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanWindow.h"

namespace RN
{
	VkFormat _vertexFormatLookup[] =
		{
			VK_FORMAT_R8G8_UINT,
			VK_FORMAT_R16G16_UINT,
			VK_FORMAT_R32_UINT,

			VK_FORMAT_R8G8_SINT,
			VK_FORMAT_R16G16_SINT,
			VK_FORMAT_R32_SINT,

			VK_FORMAT_R32_SFLOAT,

			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT,

			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT
		};

	VulkanStateCoordinator::VulkanStateCoordinator() :
		_renderer(nullptr)
	{}

	VulkanStateCoordinator::~VulkanStateCoordinator()
	{
		for(VulkanRenderingStateCollection *collection : _renderingStates)
			delete collection;
	}

	VKAPI void VulkanStateCoordinator::SetRenderer(VulkanRenderer *renderer)
	{
		_renderer = renderer;
	}

	const VulkanRenderingState *VulkanStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		VulkanShader *vertexShader = static_cast<VulkanShader *>(material->GetVertexShader());
		VulkanShader *fragmentShader = static_cast<VulkanShader *>(material->GetFragmentShader());

		for(VulkanRenderingStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->vertexShader == vertexShader && collection->fragmentShader == fragmentShader)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, camera);
				}
			}
		}

		VulkanRenderingStateCollection *collection = new VulkanRenderingStateCollection(descriptor, vertexShader, fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, camera);
	}

	const VulkanRenderingState *VulkanStateCoordinator::GetRenderPipelineStateInCollection(VulkanRenderingStateCollection *collection, Mesh *mesh, Camera *camera)
	{
		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VkDevice device = renderer->GetVulkanDevice()->GetDevice();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationState.flags = 0;
		rasterizationState.depthClampEnable = VK_TRUE;

		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.pNext = NULL;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.front = depthStencilState.back;
		depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.flags = 0;

		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = dynamicStateEnables.size();

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = collection->vertexShader->_shaderStage;
		shaderStages[1] = collection->fragmentShader->_shaderStage;


		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;

		VkPipelineLayout pipelineLayout;
		RNVulkanValidate(vk::CreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		VulkanFramebuffer *framebuffer = nullptr;
		if(camera)
			camera->GetFramebuffer()->Downcast<VulkanFramebuffer>();
		if(!framebuffer)
			framebuffer = Renderer::GetActiveRenderer()->GetMainWindow()->Downcast<VulkanWindow>()->GetFramebuffer();


		// Binding description
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = mesh->GetStride();
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		size_t offset = 0;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();

		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

			VkVertexInputAttributeDescription attributeDescription = {};
			attributeDescription.location = offset;
			attributeDescription.binding = 0;
			attributeDescription.format = _vertexFormatLookup[static_cast<VkFormat>(attribute.GetType())];
			attributeDescription.offset = attribute.GetOffset();

			attributeDescriptions.push_back(attributeDescription);

			//TODO: Remove (unused bindings confuse the validation layers...)
			break;

			offset ++;
		}

		VkPipelineVertexInputStateCreateInfo inputState = {};
		inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		inputState.pNext = NULL;
		inputState.vertexBindingDescriptionCount = 1;
		inputState.pVertexBindingDescriptions = &bindingDescription;
		inputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		inputState.pVertexAttributeDescriptions = attributeDescriptions.data();


		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = NULL;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = framebuffer->GetRenderPass();
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.pVertexInputState = &inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VkPipeline pipeline;
		//TODO: Use pipeline cache for creating related pipelines! (second parameter)
		RNVulkanValidate(vk::CreateGraphicsPipelines(device,  VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

/*



		MTLPixelFormat pixelFormat = MTLPixelFormatBGRA8Unorm;
		MTLPixelFormat depthFormat = MTLPixelFormatDepth24Unorm_Stencil8;
		MTLPixelFormat stencilFormat = MTLPixelFormatDepth24Unorm_Stencil8;

		for(const MetalRenderingState *state : collection->states)
		{
			if(state->pixelFormat == pixelFormat && state->depthFormat == depthFormat && state->stencilFormat == stencilFormat)
				return state;
		}

		MTLVertexDescriptor *descriptor = CreateVertexDescriptorFromMesh(mesh);

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = collection->vertexShader;
		pipelineStateDescriptor.fragmentFunction = collection->fragmentShader;
		pipelineStateDescriptor.vertexDescriptor = descriptor;
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
		pipelineStateDescriptor.depthAttachmentPixelFormat = depthFormat;
		pipelineStateDescriptor.stencilAttachmentPixelFormat = stencilFormat;

		MTLRenderPipelineReflection *reflection;
		id<MTLRenderPipelineState> pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor options:MTLPipelineOptionBufferTypeInfo reflection:&reflection error:NULL];

		[pipelineStateDescriptor release];
		[descriptor release];

		// TODO: Error handling, plox
*/

		// Create the rendering state
		VulkanRenderingState *state = new VulkanRenderingState();
		state->state = pipeline;

		collection->states.push_back(state);

		return state;
	}
}
