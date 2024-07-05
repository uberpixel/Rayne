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
#include "RNVulkanDynamicGPUBuffer.h"

namespace RN
{
	VkFormat _vertexFormatLookup[] =
		{
			VK_FORMAT_UNDEFINED,

			VK_FORMAT_R8G8_UINT,
			VK_FORMAT_R16G16_UINT,
			VK_FORMAT_R32_UINT,

			VK_FORMAT_R8G8_SINT,
			VK_FORMAT_R16G16_SINT,
			VK_FORMAT_R32_SINT,

			VK_FORMAT_R16_SFLOAT,
			VK_FORMAT_R16G16_SFLOAT,
			VK_FORMAT_R16G16B16_SFLOAT,
			VK_FORMAT_R16G16B16A16_SFLOAT,

            VK_FORMAT_R32_SFLOAT,
			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT,

			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT,

			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT
		};

	VkBlendFactor _blendFactorLookup[] =
	{
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_SRC_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_FACTOR_DST_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		VK_BLEND_FACTOR_DST_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
		VK_BLEND_FACTOR_CONSTANT_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		VK_BLEND_FACTOR_CONSTANT_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA
	};

	VulkanUniformState::VulkanUniformState() : instanceAttributesBuffer(nullptr), instanceAttributesArgumentBuffer(nullptr)
	{

	}
	VulkanUniformState::~VulkanUniformState()
	{
		for(VulkanDynamicBufferReference *buffer : vertexConstantBuffers)
		{
			buffer->Release();
		}
		for(VulkanDynamicBufferReference *buffer : fragmentConstantBuffers)
		{
			buffer->Release();
		}

		SafeRelease(instanceAttributesBuffer);
		SafeRelease(instanceAttributesArgumentBuffer);

		for(Shader::ArgumentBuffer *buffer : constantBufferToArgumentMapping)
			buffer->Release();
	}


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
		Array *samplerArray = new Array();
		samplerArray->Autorelease();
		std::vector<uint16> bindingIndex;
		std::vector<uint8> bindingType;

		VulkanShader *vertexShader = static_cast<VulkanShader *>(descriptor.vertexShader);
		VulkanShader *fragmentShader = static_cast<VulkanShader *>(descriptor.fragmentShader);

		const Shader::Signature *vertexShaderSignature = vertexShader? vertexShader->GetSignature() : nullptr;
		const Shader::Signature *fragmentShaderSignature = fragmentShader? fragmentShader->GetSignature() : nullptr;

		uint8 textureCount = 0;
		uint8 constantBufferCount = 0;

		if(vertexShaderSignature)
		{
			vertexShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer? 0 : 1);
			});

			vertexShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(2);
			});

			vertexShaderSignature->GetTextures()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(3);
			});

			textureCount += vertexShaderSignature->GetTextures()->GetCount();
			constantBufferCount += vertexShaderSignature->GetBuffers()->GetCount();

			samplerArray->AddObjectsFromArray(vertexShaderSignature->GetSamplers());
		}
		if(fragmentShaderSignature)
		{
			fragmentShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer? 4 : 5);
			});

			fragmentShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(6);
			});

			fragmentShaderSignature->GetTextures()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(7);
			});

			textureCount += fragmentShaderSignature->GetTextures()->GetCount();
			constantBufferCount += fragmentShaderSignature->GetBuffers()->GetCount();

			samplerArray->AddObjectsFromArray(fragmentShaderSignature->GetSamplers());
		}

		for(VulkanRootSignature *signature : _rootSignatures)
		{
			if(signature->bindingIndex != bindingIndex) continue;
			if(signature->bindingType != bindingType) continue;

			bool notEqual = false;
			signature->samplers->Enumerate<Shader::ArgumentSampler>([&](Shader::ArgumentSampler *sampler, size_t index, bool &stop) {
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
		signature->bindingIndex = bindingIndex;
		signature->bindingType = bindingType;
		signature->samplers = samplerArray->Retain();
		signature->textureCount = textureCount;
		signature->constantBufferCount = constantBufferCount;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		//Vertex buffer!?
		/*VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
		setUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		setUniformLayoutBinding.binding = setLayoutBindings.size();
		setUniformLayoutBinding.descriptorCount = 0;
		setLayoutBindings.push_back(setUniformLayoutBinding);*/

		if(vertexShader)
		{
			const Shader::Signature *signature = vertexShader->GetSignature();

			//Vertex shader constant buffers
			signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
				setUniformLayoutBinding.descriptorType = (buffer->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				setUniformLayoutBinding.binding = buffer->GetIndex();
				setUniformLayoutBinding.descriptorCount = 1;
				setLayoutBindings.push_back(setUniformLayoutBinding);
			});

			//Vertex shader textures
			signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
				setImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				setImageLayoutBinding.binding = texture->GetIndex();
				setImageLayoutBinding.descriptorCount = 1;
				setLayoutBindings.push_back(setImageLayoutBinding);
			});
		}

		if(fragmentShader)
		{
			const Shader::Signature *signature = fragmentShader->GetSignature();

			//Vertex shader constant buffers
			signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
				setUniformLayoutBinding.descriptorType = (buffer->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setUniformLayoutBinding.binding = buffer->GetIndex();
				setUniformLayoutBinding.descriptorCount = 1;
				setLayoutBindings.push_back(setUniformLayoutBinding);
			});

			//Fragment shader textures
			signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
				setImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setImageLayoutBinding.binding = texture->GetIndex();
				setImageLayoutBinding.descriptorCount = 1;
				setLayoutBindings.push_back(setImageLayoutBinding);
			});
		}

		// Create samplers
		std::vector<VkSampler> samplers;
		samplers.reserve(signature->samplers->GetCount());
		if(signature->samplers->GetCount() > 0)
		{
			signature->samplers->Enumerate<Shader::ArgumentSampler>([&](Shader::ArgumentSampler *sampler, size_t index, bool &stop) {

				VkFilter filter = VK_FILTER_LINEAR;
				switch(sampler->GetFilter())
				{
					case Shader::ArgumentSampler::Filter::Anisotropic:
						filter = VK_FILTER_LINEAR;
						break;
					case Shader::ArgumentSampler::Filter::Linear:
						filter = VK_FILTER_LINEAR;
						break;
					case Shader::ArgumentSampler::Filter::Nearest:
						filter = VK_FILTER_NEAREST;
						break;
				}

				VkCompareOp comparisonFunction = VK_COMPARE_OP_NEVER;
				if(sampler->GetComparisonFunction() != Shader::ArgumentSampler::ComparisonFunction::Never)
				{
					switch(sampler->GetComparisonFunction())
					{
						case Shader::ArgumentSampler::ComparisonFunction::Always:
							comparisonFunction = VK_COMPARE_OP_ALWAYS;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Equal:
							comparisonFunction = VK_COMPARE_OP_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::GreaterEqual:
							comparisonFunction = VK_COMPARE_OP_GREATER_OR_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Greater:
							comparisonFunction = VK_COMPARE_OP_GREATER;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Less:
							comparisonFunction = VK_COMPARE_OP_LESS;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::LessEqual:
							comparisonFunction = VK_COMPARE_OP_LESS_OR_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::NotEqual:
							comparisonFunction = VK_COMPARE_OP_NOT_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Never:
							comparisonFunction = VK_COMPARE_OP_NEVER;
							break;
					}
				}

				VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				switch(sampler->GetWrapMode())
				{
					case Shader::ArgumentSampler::WrapMode::Repeat:
						addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						break;
					case Shader::ArgumentSampler::WrapMode::Clamp:
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
				samplerInfo.compareEnable = comparisonFunction == VK_COMPARE_OP_NEVER ? VK_FALSE : VK_TRUE;
				samplerInfo.compareOp = comparisonFunction;
				samplerInfo.minLod = 0.0f;
				samplerInfo.maxLod = std::numeric_limits<float>::max();
				samplerInfo.maxAnisotropy = sampler->GetAnisotropy();
				samplerInfo.anisotropyEnable = (sampler->GetFilter() == Shader::ArgumentSampler::Filter::Anisotropic)? VK_TRUE:VK_FALSE;
				samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				samplerInfo.unnormalizedCoordinates = VK_FALSE;

				VkSampler vkSampler;
				RNVulkanValidate(vk::CreateSampler(device->GetDevice(), &samplerInfo, renderer->GetAllocatorCallback(), &vkSampler));
				//TODO: Store and release samplers with the pipeline? Maybe can immediately be released after creating pipeline?

				samplers.push_back(vkSampler);

				VkDescriptorSetLayoutBinding staticSamplerBinding = {};
				staticSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				staticSamplerBinding.stageFlags = VK_SHADER_STAGE_ALL;
				staticSamplerBinding.binding = sampler->GetIndex();
				staticSamplerBinding.descriptorCount = 1;
				staticSamplerBinding.pImmutableSamplers = &samplers[samplers.size()-1];

				setLayoutBindings.push_back(staticSamplerBinding);
			});
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

	const VulkanPipelineState *VulkanStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, VulkanFramebuffer *framebuffer, VulkanFramebuffer *resolveFramebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial, RenderPass::Flags flags, uint8 multiviewCount)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();
		const Material::Properties &mergedMaterialProperties = material->GetMergedProperties(overrideMaterial);
		VulkanPipelineStateDescriptor pipelineDescriptor;
		pipelineDescriptor.depthStencilFormat = (framebuffer->_depthStencilTarget) ? framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format : VK_FORMAT_UNDEFINED;
		pipelineDescriptor.sampleCount = framebuffer->GetSampleCount();
		//pipelineDescriptor.sampleQuality = 0;//(framebuffer->_colorTargets.size() > 0 && !framebuffer->GetSwapChain()) ? framebuffer->_colorTargets[0]->targetView.texture->GetDescriptor().sampleQuality : 0;
		pipelineDescriptor.renderPass = GetRenderPassState(framebuffer, resolveFramebuffer, flags, multiviewCount)->renderPass;
		pipelineDescriptor.shaderHint = shaderHint;
		pipelineDescriptor.vertexShader = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders))? overrideMaterial->GetVertexShader(pipelineDescriptor.shaderHint) : material->GetVertexShader(pipelineDescriptor.shaderHint);
		pipelineDescriptor.fragmentShader = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders)) ? overrideMaterial->GetFragmentShader(pipelineDescriptor.shaderHint) : material->GetFragmentShader(pipelineDescriptor.shaderHint);
		pipelineDescriptor.depthWriteEnabled = mergedMaterialProperties.depthWriteEnabled;
		pipelineDescriptor.colorWriteMask = mergedMaterialProperties.colorWriteMask;
		pipelineDescriptor.depthMode = mergedMaterialProperties.depthMode;
		pipelineDescriptor.cullMode = mergedMaterialProperties.cullMode;
		pipelineDescriptor.usePolygonOffset = mergedMaterialProperties.usePolygonOffset;
		pipelineDescriptor.polygonOffsetFactor = mergedMaterialProperties.polygonOffsetFactor;
		pipelineDescriptor.polygonOffsetUnits = mergedMaterialProperties.polygonOffsetUnits;
		pipelineDescriptor.useAlphaToCoverage = mergedMaterialProperties.useAlphaToCoverage;
		pipelineDescriptor.blendOperationRGB = mergedMaterialProperties.blendOperationRGB;
        pipelineDescriptor.blendOperationAlpha = mergedMaterialProperties.blendOperationAlpha;
        pipelineDescriptor.blendFactorSourceRGB = mergedMaterialProperties.blendFactorSourceRGB;
        pipelineDescriptor.blendFactorDestinationRGB = mergedMaterialProperties.blendFactorDestinationRGB;
        pipelineDescriptor.blendFactorSourceAlpha = mergedMaterialProperties.blendFactorSourceAlpha;
        pipelineDescriptor.blendFactorDestinationAlpha = mergedMaterialProperties.blendFactorDestinationAlpha;
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

		//TODO: Make sure all possible cases are covered...
		//TODO: Maybe solve nicer...
		for(const VulkanPipelineState *state : collection->states)
		{
			if(state->descriptor.renderPass == descriptor.renderPass && state->descriptor.depthStencilFormat == descriptor.depthStencilFormat && rootSignature->pipelineLayout == state->rootSignature->pipelineLayout)
			{
				if(state->descriptor.sampleCount == descriptor.sampleCount && state->descriptor.colorWriteMask == descriptor.colorWriteMask && state->descriptor.depthWriteEnabled == descriptor.depthWriteEnabled && state->descriptor.depthMode == descriptor.depthMode && state->descriptor.cullMode == descriptor.cullMode && state->descriptor.usePolygonOffset == descriptor.usePolygonOffset && state->descriptor.polygonOffsetFactor == descriptor.polygonOffsetFactor && state->descriptor.polygonOffsetUnits == descriptor.polygonOffsetUnits && state->descriptor.useAlphaToCoverage == descriptor.useAlphaToCoverage && state->descriptor.blendOperationRGB == descriptor.blendOperationRGB && state->descriptor.blendOperationAlpha == descriptor.blendOperationAlpha && state->descriptor.blendFactorSourceRGB == descriptor.blendFactorSourceRGB && state->descriptor.blendFactorSourceAlpha == descriptor.blendFactorSourceAlpha && state->descriptor.blendFactorDestinationRGB == descriptor.blendFactorDestinationRGB && state->descriptor.blendFactorDestinationAlpha == descriptor.blendFactorDestinationAlpha)
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
        bool vertexPositionsOnly = false;
		bool hasInstancing = vertexShaderRayne->GetHasInstancing() && vertexShaderRayne->_instancingAttributes != nullptr;
        const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions = CreateVertexElementDescriptorsFromMesh(mesh, vertexShaderRayne, vertexPositionsOnly);
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
		if(mesh->GetVertexPositionsSeparatedSize() > 0)
		{
			//Positions buffer
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = vertexBindingDescriptions.size();
			bindingDescription.stride = mesh->GetVertexPositionsSeparatedStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindingDescriptions.push_back(bindingDescription);
		}
        if(!vertexPositionsOnly) //vertexPositionsOnly will be true only if separated positions are used and no other vertex attributes exist
		{
			//Interleaved buffer
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = vertexBindingDescriptions.size();
			bindingDescription.stride = mesh->GetStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindingDescriptions.push_back(bindingDescription);
		}
		if(hasInstancing)
		{
			//Per instance data buffer binding
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = vertexBindingDescriptions.size();
			bindingDescription.stride = vertexShaderRayne->_instancingAttributes->GetTotalUniformSize();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			vertexBindingDescriptions.push_back(bindingDescription);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.pNext = NULL;
		vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		switch(mesh->GetDrawMode())
        {
            case DrawMode::Point:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                break;

            case DrawMode::Line:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                break;

            case DrawMode::LineStrip:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                break;

            case DrawMode::Triangle:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;

            case DrawMode::TriangleStrip:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                break;
        }
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.flags = 0;
		rasterizationState.depthClampEnable = VK_TRUE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		if(descriptor.usePolygonOffset)
		{
			rasterizationState.depthBiasEnable = VK_TRUE;
			rasterizationState.depthBiasConstantFactor = descriptor.polygonOffsetUnits*0.5f;
			rasterizationState.depthBiasSlopeFactor = descriptor.polygonOffsetFactor;
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
				rasterizationState.cullMode = VK_CULL_MODE_NONE;
				break;
		}

		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = descriptor.colorWriteMask;
		blendAttachmentState.blendEnable = VK_FALSE;
		if(descriptor.blendOperationRGB != BlendOperation::None && descriptor.blendOperationAlpha != BlendOperation::None)
		{
			blendAttachmentState.blendEnable = VK_TRUE;
			blendAttachmentState.colorBlendOp = static_cast<VkBlendOp>(descriptor.blendOperationRGB);
			blendAttachmentState.alphaBlendOp = static_cast<VkBlendOp>(descriptor.blendOperationAlpha);
			blendAttachmentState.srcColorBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorSourceRGB)];
			blendAttachmentState.srcAlphaBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorSourceAlpha)];
			blendAttachmentState.dstColorBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorDestinationRGB)];
			blendAttachmentState.dstAlphaBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorDestinationAlpha)];
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.pNext = NULL;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if(descriptor.depthStencilFormat != VK_FORMAT_UNDEFINED)
		{
			depthStencilState.depthTestEnable = descriptor.depthMode != DepthMode::Never;
			depthStencilState.depthWriteEnable = descriptor.depthWriteEnabled;

			switch(descriptor.depthMode)
			{
				case DepthMode::Never:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
					break;

				case DepthMode::Always:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
					break;

				case DepthMode::Less:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
					break;

				case DepthMode::LessOrEqual:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					break;

				case DepthMode::Equal:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;
					break;

				case DepthMode::NotEqual:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
					break;

				case DepthMode::GreaterOrEqual:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
					break;

				case DepthMode::Greater:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
					break;

				default:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					break;
			}


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
		multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(descriptor.sampleCount);
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.alphaToCoverageEnable = (descriptor.useAlphaToCoverage && descriptor.colorWriteMask > 0)? VK_TRUE : VK_FALSE;
		//TODO: Maybe set minSampleShading?

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
        state->vertexAttributeBufferCount = vertexBindingDescriptions.size();

		collection->states.push_back(state);
		return state;



/*
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.NumRenderTargets = descriptor.colorFormats.size();
		int counter = 0;
		for(DXGI_FORMAT format : descriptor.colorFormats)
		{
			psoDesc.RTVFormats[counter++] = format;
			if(counter >= 8)
				break;
		}*/
	}

	std::vector<VkVertexInputAttributeDescription> VulkanStateCoordinator::CreateVertexElementDescriptorsFromMesh(Mesh *mesh, VulkanShader *vertexShader, bool &vertexPositionsOnly)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        vertexPositionsOnly = true;

		uint8 vertexBinding = 0;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

            uint32 attributeLocation = vertexShader->_hasInputVertexAttribute[static_cast<uint32>(attribute.GetFeature())];
			if(attributeLocation != -1)
			{
				VkVertexInputAttributeDescription attributeDescription = {};
				attributeDescription.location = attributeLocation;
				attributeDescription.binding = vertexBinding;
				attributeDescription.format = _vertexFormatLookup[static_cast<VkFormat>(attribute.GetType())];
				attributeDescription.offset = attribute.GetOffset();

				attributeDescriptions.push_back(attributeDescription);

				if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Vertices && mesh->GetVertexPositionsSeparatedSize() > 0)
				{
					//Vertex positions are always the first attribute if GetVertexPositionsSeparatedSize is > 0, so just increasing the binding here like this should be fine
					vertexBinding += 1;
				}
                else
                {
                    vertexPositionsOnly = false;
                }
			}
		}

		if(vertexShader->_instancingAttributes && vertexShader->GetHasInstancing())
		{
			if(!vertexPositionsOnly) vertexBinding += 1;
			vertexShader->_instancingAttributes->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *attribute, size_t index, bool &stop){
                for(int i = 0; i < (attribute->GetType() == PrimitiveType::Matrix4x4? 4 : (attribute->GetType() == PrimitiveType::Matrix3x3? 3 : 1)); i++)
                {
                    VkVertexInputAttributeDescription attributeDescription = {};
                    attributeDescription.location = attribute->GetAttributeLocation() + i;
                    attributeDescription.binding = vertexBinding;
                    attributeDescription.format = _vertexFormatLookup[static_cast<VkFormat>(attribute->GetType())];
                    attributeDescription.offset = attribute->GetOffset() + i * 4 * 4;

                    attributeDescriptions.push_back(attributeDescription);
                }
			});
		}

		return attributeDescriptions;
	}

	VulkanUniformState *VulkanStateCoordinator::GetUniformStateForPipelineState(const VulkanPipelineState *pipelineState)
	{
		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

		VulkanUniformState *state = new VulkanUniformState();
		Shader *vertexShader = pipelineState->descriptor.vertexShader;
		Shader *fragmentShader = pipelineState->descriptor.fragmentShader;
		if(vertexShader && vertexShader->GetSignature())
		{
			VulkanShader *vulkanShader = vertexShader->Downcast<VulkanShader>();
			if(vulkanShader->GetHasInstancing() && vulkanShader->_instancingAttributes)
			{
				state->instanceAttributesArgumentBuffer = vulkanShader->_instancingAttributes->Retain();
				state->instanceAttributesBuffer = renderer->GetConstantBufferReference(vulkanShader->_instancingAttributes->GetTotalUniformSize(), vulkanShader->_instancingAttributes->GetIndex(), GPUResource::UsageOptions::Vertex)->Retain();
			}
			vertexShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					state->constantBufferToArgumentMapping.push_back(buffer->Retain());
					state->vertexConstantBuffers.push_back(renderer->GetConstantBufferReference(buffer->GetTotalUniformSize(), buffer->GetIndex())->Retain());
				}
			});
		}

		if(fragmentShader && fragmentShader->GetSignature())
		{
			fragmentShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					state->constantBufferToArgumentMapping.push_back(buffer->Retain());
					state->fragmentConstantBuffers.push_back(renderer->GetConstantBufferReference(buffer->GetTotalUniformSize(), buffer->GetIndex())->Retain());
				}
			});
		}

		return state;
	}

	VulkanRenderPassState *VulkanStateCoordinator::GetRenderPassState(const VulkanFramebuffer *framebuffer, const VulkanFramebuffer *resolveFramebuffer, RenderPass::Flags flags, uint8 multiviewCount)
	{
		//TODO: Maybe handle swapchain case better...
		RN_ASSERT(!resolveFramebuffer || framebuffer->_colorTargets.size() <= resolveFramebuffer->_colorTargets.size(), "Resolve framebuffer needs a target for each target in the framebuffer!");

		VulkanRenderPassState renderPassState;
		renderPassState.flags = flags;
		renderPassState.multiviewCount = multiviewCount;
		const VulkanFramebuffer *fragmentDensityFramebuffer = resolveFramebuffer? resolveFramebuffer : framebuffer;
		renderPassState.hasFragmentDensityMap = fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0;
		for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
		{
			renderPassState.imageFormats.push_back(targetView->vulkanTargetViewDescriptor.format);
		}

		if(resolveFramebuffer)
		{
			for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
			{
				renderPassState.resolveFormats.push_back(targetView->vulkanTargetViewDescriptor.format);
			}
		}

		if(framebuffer->_depthStencilTarget)
		{
			renderPassState.imageFormats.push_back(framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format);
		}

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			renderPassState.imageFormats.push_back(fragmentDensityFramebuffer->_fragmentDensityTargets[0]->vulkanTargetViewDescriptor.format);
		}

		for(VulkanRenderPassState *state : _renderPassStates)
		{
			if((*state) == renderPassState) return state;
		}

		VulkanRenderPassState *state = new VulkanRenderPassState;
		state->flags = renderPassState.flags;
		state->imageFormats = renderPassState.imageFormats;
		state->resolveFormats = renderPassState.resolveFormats;
		state->multiviewCount = renderPassState.multiviewCount;
		state->hasFragmentDensityMap = renderPassState.hasFragmentDensityMap;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		std::vector<VkAttachmentReference> resolveAttachmentRefs;

		uint32 counter = 0;
		for(VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
		{
			VkAttachmentReference colorReference = {};
			colorReference.attachment = attachments.size();
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentRefs.push_back(colorReference);

			VkAttachmentDescription attachment = {};
			attachment.format = targetView->vulkanTargetViewDescriptor.format;
			attachment.flags = 0;
			attachment.samples = static_cast<VkSampleCountFlagBits>(framebuffer->_sampleCount);
			if(flags & RenderPass::Flags::ClearColor) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else if(flags & RenderPass::Flags::LoadColor) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			else attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			if(flags & RenderPass::Flags::StoreColor) attachment.storeOp = resolveFramebuffer?VK_ATTACHMENT_STORE_OP_DONT_CARE:VK_ATTACHMENT_STORE_OP_STORE;
			else attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments.push_back(attachment);

			if(resolveFramebuffer)
			{
				VkAttachmentReference resolveReference = {};
				resolveReference.attachment = attachments.size();
				resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				resolveAttachmentRefs.push_back(resolveReference);

				VkAttachmentDescription resolveAttachment = {};
				if(resolveFramebuffer->_swapChain)
				{
					resolveAttachment.format = resolveFramebuffer->_colorTargets[0]->vulkanTargetViewDescriptor.format;
				}
				else
				{
					resolveAttachment.format = resolveFramebuffer->_colorTargets[counter]->vulkanTargetViewDescriptor.format;
				}
				resolveAttachment.flags = 0;
				resolveAttachment.samples = static_cast<VkSampleCountFlagBits>(resolveFramebuffer->_sampleCount);
				resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				if(flags & RenderPass::Flags::StoreColor) resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				else resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachments.push_back(resolveAttachment);
			}

			counter += 1;

			if(framebuffer->_swapChain || (resolveFramebuffer && resolveFramebuffer->_swapChain)) break;
		}

		subpass.colorAttachmentCount = colorAttachmentRefs.size();
		subpass.pColorAttachments = colorAttachmentRefs.data();

		if(resolveFramebuffer)
		{
			subpass.pResolveAttachments = resolveAttachmentRefs.data();
		}

		VkAttachmentReference depthReference = {};
		if(framebuffer->_depthStencilTarget)
		{
			depthReference.attachment = attachments.size();
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription attachment = {};
			attachment.format = framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format;
			attachment.flags = 0;
			attachment.samples = static_cast<VkSampleCountFlagBits>(framebuffer->_sampleCount);
			if(flags & RenderPass::Flags::ClearDepthStencil) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else if(flags & RenderPass::Flags::LoadDepthStencil) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			else attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			if(flags & RenderPass::Flags::StoreDepthStencil) attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			else attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments.push_back(attachment);

			subpass.pDepthStencilAttachment = &depthReference;

			//TODO: Figure out depth resolve!?
		}

		VkRenderPassFragmentDensityMapCreateInfoEXT fragmentDensityMapCreateInfo = {};
		fragmentDensityMapCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
		fragmentDensityMapCreateInfo.pNext = nullptr;

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			fragmentDensityMapCreateInfo.fragmentDensityMapAttachment.attachment = attachments.size();
			fragmentDensityMapCreateInfo.fragmentDensityMapAttachment.layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;

			VkAttachmentDescription attachment = {};
			attachment.format = fragmentDensityFramebuffer->_fragmentDensityTargets[0]->vulkanTargetViewDescriptor.format;
			attachment.flags = 0;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
			attachment.finalLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
			attachments.push_back(attachment);
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = nullptr;

		//Multiview stuff
		uint32 viewMask = 0;
		uint32 correlationMask = 0;
		if(multiviewCount > 1)
		{
			for(int i = 0; i < multiviewCount; i++)
			{
				viewMask |= (1 << i);
				correlationMask |= (1 << i);
			}
		}
		VkRenderPassMultiviewCreateInfoKHR multiviewPassInfo = {};
		multiviewPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		multiviewPassInfo.pNext = nullptr;
		multiviewPassInfo.subpassCount = 1;
		multiviewPassInfo.pViewMasks = &viewMask;
		multiviewPassInfo.dependencyCount = 0;
		multiviewPassInfo.correlationMaskCount = 1;
		multiviewPassInfo.pCorrelationMasks = &correlationMask;

		if(multiviewCount > 1)
		{
			renderPassInfo.pNext = &multiviewPassInfo;
		}

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			if(multiviewCount > 1)
			{
				multiviewPassInfo.pNext = &fragmentDensityMapCreateInfo;
			}
			else
			{
				renderPassInfo.pNext = &fragmentDensityMapCreateInfo;
			}
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();
		RNVulkanValidate(vk::CreateRenderPass(device->GetDevice(), &renderPassInfo, renderer->GetAllocatorCallback(), &state->renderPass));

		_renderPassStates.push_back(state);

		return state;
	}
}
