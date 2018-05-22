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
#include "RNVulkanConstantBuffer.h"

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


	VulkanRootSignature::~VulkanRootSignature()
	{
		//signature->Release();
	}

	VulkanPipelineState::~VulkanPipelineState()
	{
		//state->Release();
	}

	VulkanStateCoordinator::VulkanStateCoordinator() :
		_lastDepthStencilState(nullptr)
	{}

	VulkanStateCoordinator::~VulkanStateCoordinator()
	{
		//TODO: Clean up correctly...
		for(VulkanPipelineStateCollection *collection : _renderingStates)
			delete collection;
	}

	const VulkanRootSignature *VulkanStateCoordinator::GetRootSignature(const VulkanPipelineStateDescriptor &descriptor)
	{
		VulkanShader *vertexShader = static_cast<VulkanShader *>(descriptor.vertexShader);
		const Shader::Signature *vertexSignature = vertexShader->GetSignature();
		uint16 textureCount = vertexSignature->GetTextureCount();
		const Array *vertexSamplers = vertexSignature->GetSamplers();
		Array *samplerArray = new Array(vertexSamplers);
		samplerArray->Autorelease();

		bool wantsDirectionalShadowTexture = false;//vertexShader->_wantsDirectionalShadowTexture;

		//TODO: Support multiple constant buffers per function signature
		uint16 constantBufferCount = (vertexSignature->GetTotalUniformSize() > 0) ? 1 : 0;

		VulkanShader *fragmentShader = static_cast<VulkanShader *>(descriptor.fragmentShader);
		if(fragmentShader)
		{
			const Shader::Signature *fragmentSignature = fragmentShader->GetSignature();
			textureCount = fmax(textureCount, fragmentSignature->GetTextureCount());
			const Array *fragmentSamplers = fragmentSignature->GetSamplers();
			samplerArray->AddObjectsFromArray(fragmentSamplers);

			//wantsDirectionalShadowTexture = (wantsDirectionalShadowTexture || fragmentShader->_wantsDirectionalShadowTexture);

			//TODO: Support multiple constant buffers per function signature
			constantBufferCount += (fragmentSignature->GetTotalUniformSize() > 0) ? 1 : 0;
		}

		for(VulkanRootSignature *signature : _rootSignatures)
		{

			if(signature->textureCount != textureCount)
			{
				continue;
			}

			//TODO: Doesn't really require an extra root signature...
			if(signature->wantsDirectionalShadowTexture != wantsDirectionalShadowTexture)
			{
				continue;
			}

			if(signature->constantBufferCount != constantBufferCount)
			{
				continue;
			}

			if(samplerArray->GetCount() != signature->samplers->GetCount())
			{
				continue;
			}

			bool notEqual = false;
			signature->samplers->Enumerate<Shader::Sampler>([&](Shader::Sampler *sampler, size_t index, bool &stop) {
				if(!(sampler == samplerArray->GetObjectAtIndex(index)))
				{
					notEqual = true;
					stop = true;
				}
			});
			if(notEqual)
			{
				continue;
			}

			return signature;
		}

		VulkanRenderer *renderer = static_cast<VulkanRenderer *>(Renderer::GetActiveRenderer());
		VulkanDevice *device = renderer->GetVulkanDevice();

		VulkanRootSignature *signature = new VulkanRootSignature();
		signature->constantBufferCount = constantBufferCount;
		signature->samplers = samplerArray->Retain();
		signature->textureCount = textureCount;
		signature->wantsDirectionalShadowTexture = wantsDirectionalShadowTexture;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		for(size_t i = 0; i < constantBufferCount; i++)
		{
			VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
			setUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
			setUniformLayoutBinding.binding = setLayoutBindings.size();
			setUniformLayoutBinding.descriptorCount = 1;
			setLayoutBindings.push_back(setUniformLayoutBinding);
		}


		// Create samplers
		std::vector<VkSampler> samplers;
		if(signature->samplers->GetCount() > 0)
		{
			signature->samplers->Enumerate<Shader::Sampler>([&](Shader::Sampler *sampler, size_t index, bool &stop) {

				VkFilter filter = VK_FILTER_LINEAR;
				switch(sampler->GetFilter())
				{
					case Shader::Sampler::Filter::Anisotropic:
						filter = VK_FILTER_LINEAR;
						break;
					case Shader::Sampler::Filter::Linear:
						filter = VK_FILTER_LINEAR;
						break;
					case Shader::Sampler::Filter::Nearest:
						filter = VK_FILTER_NEAREST;
						break;
				}

				VkCompareOp comparisonFunction = VK_COMPARE_OP_NEVER;
				if(sampler->GetComparisonFunction() != Shader::Sampler::ComparisonFunction::Never)
				{
					switch(sampler->GetComparisonFunction())
					{
						case Shader::Sampler::ComparisonFunction::Always:
							comparisonFunction = VK_COMPARE_OP_ALWAYS;
							break;
						case Shader::Sampler::ComparisonFunction::Equal:
							comparisonFunction = VK_COMPARE_OP_EQUAL;
							break;
						case Shader::Sampler::ComparisonFunction::GreaterEqual:
							comparisonFunction = VK_COMPARE_OP_GREATER_OR_EQUAL;
							break;
						case Shader::Sampler::ComparisonFunction::Greater:
							comparisonFunction = VK_COMPARE_OP_GREATER;
							break;
						case Shader::Sampler::ComparisonFunction::Less:
							comparisonFunction = VK_COMPARE_OP_LESS;
							break;
						case Shader::Sampler::ComparisonFunction::LessEqual:
							comparisonFunction = VK_COMPARE_OP_LESS_OR_EQUAL;
							break;
						case Shader::Sampler::ComparisonFunction::NotEqual:
							comparisonFunction = VK_COMPARE_OP_NOT_EQUAL;
							break;
					}
				}

				VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				switch(sampler->GetWrapMode())
				{
					case Shader::Sampler::WrapMode::Repeat:
						addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						break;
					case Shader::Sampler::WrapMode::Clamp:
						addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
						break;
				}

				// Create sampler
				VkSamplerCreateInfo samplerInfo = {};
				samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerInfo.magFilter = filter;
				samplerInfo.minFilter = filter;
				samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerInfo.addressModeU = addressMode;
				samplerInfo.addressModeV = addressMode;
				samplerInfo.addressModeW = addressMode;
				samplerInfo.mipLodBias = 0.0f;
				samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
				samplerInfo.minLod = 0.0f;
				samplerInfo.maxLod = 20;//std::numeric_limits<float>::max();
				samplerInfo.maxAnisotropy = 1;//sampler->GetAnisotropy();
				samplerInfo.anisotropyEnable = 0;//(sampler->GetFilter() == Shader::Sampler::Filter::Anisotropic)? VK_TRUE:VK_FALSE;
				samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				samplerInfo.unnormalizedCoordinates = VK_FALSE;

				VkSampler vkSampler;
				RNVulkanValidate(vk::CreateSampler(device->GetDevice(), &samplerInfo, renderer->GetAllocatorCallback(), &vkSampler));
				//TODO: Store and release samplers with the pipeline? Maybe can immediately be released after creating pipeline?

				samplers.push_back(vkSampler);

				VkDescriptorSetLayoutBinding staticSamplerBinding = {};
				staticSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				staticSamplerBinding.stageFlags = VK_SHADER_STAGE_ALL;
				staticSamplerBinding.binding = setLayoutBindings.size();
				staticSamplerBinding.descriptorCount = 1;
				staticSamplerBinding.pImmutableSamplers = &samplers[samplers.size()-1];

				setLayoutBindings.push_back(staticSamplerBinding);
			});
		}

		for(size_t i = 0; i < textureCount; i++)
		{
			VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
			setImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
			setImageLayoutBinding.binding = setLayoutBindings.size();
			setImageLayoutBinding.descriptorCount = 1;

			setLayoutBindings.push_back(setImageLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
		descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();

		RNVulkanValidate(vk::CreateDescriptorSetLayout(device->GetDevice(), &descriptorSetLayoutCreateInfo, renderer->GetAllocatorCallback(), &signature->descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &signature->descriptorSetLayout;

		RNVulkanValidate(vk::CreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo, renderer->GetAllocatorCallback(), &signature->pipelineLayout));

		_rootSignatures.push_back(signature);
		return signature;
	}

	const VulkanPipelineState *VulkanStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, VulkanFramebuffer *framebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();
		Material::Properties mergedMaterialProperties = material->GetMergedProperties(overrideMaterial);
		VulkanPipelineStateDescriptor pipelineDescriptor;
		pipelineDescriptor.depthStencilFormat = (framebuffer->_depthStencilTarget) ? framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format : VK_FORMAT_UNDEFINED;
		//pipelineDescriptor.sampleCount = framebuffer->GetSampleCount();
		//pipelineDescriptor.sampleQuality = 0;//(framebuffer->_colorTargets.size() > 0 && !framebuffer->GetSwapChain()) ? framebuffer->_colorTargets[0]->targetView.texture->GetDescriptor().sampleQuality : 0;
		pipelineDescriptor.renderPass = framebuffer->_renderPass;
		pipelineDescriptor.shaderHint = shaderHint;
		pipelineDescriptor.vertexShader = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders))? overrideMaterial->GetVertexShader(pipelineDescriptor.shaderHint) : material->GetVertexShader(pipelineDescriptor.shaderHint);
		pipelineDescriptor.fragmentShader = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders)) ? overrideMaterial->GetFragmentShader(pipelineDescriptor.shaderHint) : material->GetFragmentShader(pipelineDescriptor.shaderHint);
		pipelineDescriptor.cullMode = mergedMaterialProperties.cullMode;
		pipelineDescriptor.usePolygonOffset = mergedMaterialProperties.usePolygonOffset;
		pipelineDescriptor.polygonOffsetFactor = mergedMaterialProperties.polygonOffsetFactor;
		pipelineDescriptor.polygonOffsetUnits = mergedMaterialProperties.polygonOffsetUnits;
		pipelineDescriptor.useAlphaToCoverage = mergedMaterialProperties.useAlphaToCoverage;
		//TODO: Support all override flags and all the relevant material properties

		for(VulkanPipelineStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->vertexShader == pipelineDescriptor.vertexShader && collection->fragmentShader == pipelineDescriptor.fragmentShader)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, pipelineDescriptor);
				}
			}
		}

		VulkanPipelineStateCollection *collection = new VulkanPipelineStateCollection(descriptor, pipelineDescriptor.vertexShader, pipelineDescriptor.fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, pipelineDescriptor);
	}

	const VulkanPipelineState *VulkanStateCoordinator::GetRenderPipelineStateInCollection(VulkanPipelineStateCollection *collection, Mesh *mesh, const VulkanPipelineStateDescriptor &descriptor)
	{
		const VulkanRootSignature *rootSignature = GetRootSignature(descriptor);

		//TODO: Make sure all possible cases are covered... Depth bias for example... cullmode...
		//TODO: Maybe solve nicer...
		for(const VulkanPipelineState *state : collection->states)
		{
			if(state->descriptor.renderPass == descriptor.renderPass && state->descriptor.depthStencilFormat == descriptor.depthStencilFormat && rootSignature->pipelineLayout == state->rootSignature->pipelineLayout)
			{
				if(state->descriptor.cullMode == descriptor.cullMode && state->descriptor.usePolygonOffset == descriptor.usePolygonOffset && state->descriptor.polygonOffsetFactor == descriptor.polygonOffsetFactor && state->descriptor.polygonOffsetUnits == descriptor.polygonOffsetUnits && state->descriptor.useAlphaToCoverage == descriptor.useAlphaToCoverage)
				{
					return state;
				}
			}
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();

		//Collect shaders
		VulkanShader *vertexShaderRayne = collection->vertexShader->Downcast<VulkanShader>();
		VulkanShader *fragmentShaderRayne = collection->fragmentShader->Downcast<VulkanShader>();
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = vertexShaderRayne->_shaderStage;
		shaderStages[1] = fragmentShaderRayne->_shaderStage;

		//Handle vertex attributes
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = mesh->GetStride();
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions = CreateVertexElementDescriptorsFromMesh(mesh);

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.pNext = NULL;
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.flags = 0;
		rasterizationState.depthClampEnable = VK_TRUE;
		if(descriptor.usePolygonOffset)
		{
			rasterizationState.depthBiasConstantFactor = descriptor.polygonOffsetUnits;
			rasterizationState.depthBiasSlopeFactor = descriptor.polygonOffsetFactor;
			//psoDesc.RasterizerState.DepthBiasClamp = D3D12_FLOAT32_MAX;
		}
		switch(descriptor.cullMode)
		{
			case CullMode::BackFace:
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
				break;
			case CullMode::FrontFace:
				rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
				break;
			case CullMode::None:
				rasterizationState.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
				break;
		}

		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.pNext = NULL;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		//psoDesc.BlendState.AlphaToCoverageEnable = descriptor.useAlphaToCoverage? TRUE : FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if(descriptor.depthStencilFormat != VK_FORMAT_UNDEFINED)
		{
			depthStencilState.depthTestEnable = VK_TRUE;
			depthStencilState.depthWriteEnable = VK_TRUE;
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilState.front = depthStencilState.back;
		}
		else
		{
			depthStencilState.depthTestEnable = VK_FALSE;
			depthStencilState.depthWriteEnable = VK_FALSE;
		}

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

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = NULL;
		pipelineCreateInfo.layout = rootSignature->pipelineLayout;
		pipelineCreateInfo.renderPass = descriptor.renderPass;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
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
		RNVulkanValidate(vk::CreateGraphicsPipelines(device->GetDevice(),  VK_NULL_HANDLE, 1, &pipelineCreateInfo, renderer->GetAllocatorCallback(), &pipeline));

		// Create the rendering state
		VulkanPipelineState *state = new VulkanPipelineState();
		state->descriptor = std::move(descriptor);
		state->rootSignature = rootSignature;
		state->state = pipeline;

		collection->states.push_back(state);
		return state;




		// Describe and create the graphics pipeline state object (PSO).
/*		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = rootSignature->signature;

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = descriptor.colorFormats.size();
		int counter = 0;
		for(DXGI_FORMAT format : descriptor.colorFormats)
		{
			psoDesc.RTVFormats[counter++] = format;
			if(counter >= 8)
				break;
		}
		psoDesc.SampleDesc.Count = descriptor.sampleCount;
		psoDesc.SampleDesc.Quality = descriptor.sampleQuality;*/
	}

	std::vector<VkVertexInputAttributeDescription> VulkanStateCoordinator::CreateVertexElementDescriptorsFromMesh(Mesh *mesh)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		size_t offset = 0;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

			//TODO: Remove the if (unused bindings confuse the validation layers...)
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Vertices || attribute.GetFeature() == Mesh::VertexAttribute::Feature::Normals || attribute.GetFeature() == Mesh::VertexAttribute::Feature::UVCoords0)
			{
				VkVertexInputAttributeDescription attributeDescription = {};
				attributeDescription.location = offset;
				attributeDescription.binding = 0;
				attributeDescription.format = _vertexFormatLookup[static_cast<VkFormat>(attribute.GetType())];
				attributeDescription.offset = attribute.GetOffset();

				attributeDescriptions.push_back(attributeDescription);
			}

			offset ++;
		}

		return attributeDescriptions;
	}

	VulkanUniformState *VulkanStateCoordinator::GetUniformStateForPipelineState(const VulkanPipelineState *pipelineState)
	{
		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

		Shader *vertexShader = pipelineState->descriptor.vertexShader;
		Shader *fragmentShader = pipelineState->descriptor.fragmentShader;
		VulkanConstantBuffer *vertexBuffer = nullptr;
		VulkanConstantBuffer *fragmentBuffer = nullptr;
		if(vertexShader && vertexShader->GetSignature() && vertexShader->GetSignature()->GetTotalUniformSize())
		{
			vertexBuffer = new VulkanConstantBuffer(vertexShader->GetSignature()->GetTotalUniformSize());
		}
		if(fragmentShader && fragmentShader->GetSignature() && fragmentShader->GetSignature()->GetTotalUniformSize())
		{
			fragmentBuffer = new VulkanConstantBuffer(fragmentShader->GetSignature()->GetTotalUniformSize());
		}

		VulkanUniformState *state = new VulkanUniformState();
		state->vertexConstantBuffer = vertexBuffer;
		state->fragmentConstantBuffer = fragmentBuffer;

		return state;
	}

	VulkanRenderPassState *VulkanStateCoordinator::GetRenderPassState(const VulkanFramebuffer *framebuffer)
	{
		VulkanRenderPassState renderPassState;
		for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
		{
			renderPassState.imageFormats.push_back(targetView->vulkanTargetViewDescriptor.format);
		}

		if(framebuffer->_depthStencilTarget)
		{
			renderPassState.imageFormats.push_back(framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format);
		}

		for(VulkanRenderPassState *state : _renderPassStates)
		{
			if((*state) == renderPassState) return state;
		}

		VulkanRenderPassState *state = new VulkanRenderPassState;
		state->imageFormats = renderPassState.imageFormats;

		VkAttachmentDescription attachments[2];
		uint32_t count = 0;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		if(framebuffer->_colorTargets.size() > 0)
		{
			attachments[count].format = framebuffer->_colorTargets[0]->vulkanTargetViewDescriptor.format;
			attachments[count].flags = 0;
			attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[count].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[count].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			subpass.pColorAttachments = &colorReference;

			count ++;
		}

		if(framebuffer->_depthStencilTarget)
		{
			attachments[count].format = framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format;
			attachments[count].flags = 0;
			attachments[count].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[count].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[count].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depthReference;

			count ++;
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.attachmentCount = count;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = nullptr;

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();
		RNVulkanValidate(vk::CreateRenderPass(device->GetDevice(), &renderPassInfo, renderer->GetAllocatorCallback(), &state->renderPass));

		_renderPassStates.push_back(state);

		return state;
	}



	/*
		VulkanUniformState *VulkanStateCoordinator::GetUniformStateForPipelineState(const VulkanPipelineState *pipelineState, Material *material)
		{
			VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.pNext = NULL;
			descriptorSetAllocateInfo.descriptorPool = _descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &pipelineState->descriptorSetLayout;
			descriptorSetAllocateInfo.descriptorSetCount = 1;

			VkDescriptorSet descriptorSet;
			RNVulkanValidate(vk::AllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));

			VulkanGPUBuffer *gpuBuffer = _renderer->CreateBufferWithLength(sizeof(Matrix)*2 + sizeof(Color)*2, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite)->Downcast<VulkanGPUBuffer>();

			VkDescriptorBufferInfo uniformBufferDescriptorInfo = {};
			uniformBufferDescriptorInfo.buffer = gpuBuffer->GetVulkanBuffer();
			uniformBufferDescriptorInfo.offset = 0;
			uniformBufferDescriptorInfo.range = gpuBuffer->GetLength();

			VkWriteDescriptorSet writeUniformDescriptorSet = {};
			writeUniformDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeUniformDescriptorSet.pNext = NULL;
			writeUniformDescriptorSet.dstSet = descriptorSet;
			writeUniformDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeUniformDescriptorSet.dstBinding = 0;
			writeUniformDescriptorSet.pBufferInfo = &uniformBufferDescriptorInfo;
			writeUniformDescriptorSet.descriptorCount = 1;

			std::vector<VkDescriptorImageInfo*> imageBufferDescriptorInfoArray;
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = { writeUniformDescriptorSet };

			material->GetTextures()->Enumerate<VulkanTexture>([&](VulkanTexture *texture, size_t index, bool &stop) {
				VkDescriptorImageInfo *imageBufferDescriptorInfo = new VkDescriptorImageInfo;
				imageBufferDescriptorInfo->sampler = texture->GetSampler();
				imageBufferDescriptorInfo->imageView = texture->GetImageView();
				imageBufferDescriptorInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageBufferDescriptorInfoArray.push_back(imageBufferDescriptorInfo);

				VkWriteDescriptorSet writeImageDescriptorSet = {};
				writeImageDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeImageDescriptorSet.pNext = NULL;
				writeImageDescriptorSet.dstSet = descriptorSet;
				writeImageDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeImageDescriptorSet.dstBinding = index + 1;
				writeImageDescriptorSet.pImageInfo = imageBufferDescriptorInfoArray[index];
				writeImageDescriptorSet.descriptorCount = 1;

				writeDescriptorSets.push_back(writeImageDescriptorSet);
			});

			vk::UpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

			for(VkDescriptorImageInfo *imageBufferDescriptor : imageBufferDescriptorInfoArray)
			{
				delete imageBufferDescriptor;
			}

			VulkanUniformState *state = new VulkanUniformState();
			state->descriptorSet = descriptorSet;
			state->uniformBuffer = gpuBuffer;

			return state;
		}*/
}
