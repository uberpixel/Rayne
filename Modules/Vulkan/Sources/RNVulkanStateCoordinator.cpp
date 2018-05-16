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
#include "RNVulkanGPUBuffer.h"

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





		VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
		setUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		setUniformLayoutBinding.binding = 0;
		setUniformLayoutBinding.descriptorCount = 1;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = { setUniformLayoutBinding };
		for(size_t i = 0; i < textureCount; i++)
		{
			VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
			setImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setImageLayoutBinding.binding = i + 1;
			setImageLayoutBinding.descriptorCount = 1;

			setLayoutBindings.push_back(setImageLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
		descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();

		VkDescriptorSetLayout descriptorSetLayout;
		RNVulkanValidate(vk::CreateDescriptorSetLayout(device->GetDevice(), &descriptorSetLayoutCreateInfo, renderer->GetAllocatorCallback(), &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		VkPipelineLayout pipelineLayout;
		RNVulkanValidate(vk::CreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo, renderer->GetAllocatorCallback(), &pipelineLayout));







/*		int numberOfTables = (signature->textureCount > 0) + (signature->constantBufferCount > 0);

		CD3DX12_DESCRIPTOR_RANGE *srvCbvRanges = nullptr;
		CD3DX12_ROOT_PARAMETER *rootParameters = nullptr;

		if(numberOfTables > 0)
		{
			srvCbvRanges = new CD3DX12_DESCRIPTOR_RANGE[numberOfTables];
			rootParameters = new CD3DX12_ROOT_PARAMETER[numberOfTables];
		}

		// Perfomance TIP: Order from most frequent to least frequent.
		int tableIndex = 0;
		if(signature->textureCount > 0)
		{
			srvCbvRanges[tableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, signature->textureCount, 0, 0);// , D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
			rootParameters[tableIndex].InitAsDescriptorTable(1, &srvCbvRanges[tableIndex], D3D12_SHADER_VISIBILITY_ALL);	//TODO: Restrict visibility to the shader actually using it
			tableIndex += 1;
		}
		if(signature->constantBufferCount > 0)
		{
			srvCbvRanges[tableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, signature->constantBufferCount, 0, 0);// , D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
			rootParameters[tableIndex].InitAsDescriptorTable(1, &srvCbvRanges[tableIndex], D3D12_SHADER_VISIBILITY_ALL);	//TODO: Restrict visibility to the shader actually using it
			tableIndex += 1;
		}

		// Create samplers
		D3D12_STATIC_SAMPLER_DESC *samplerDescriptors = nullptr;
		if(signature->samplers->GetCount() > 0)
		{
			samplerDescriptors = new D3D12_STATIC_SAMPLER_DESC[signature->samplers->GetCount()];
			signature->samplers->Enumerate<Shader::Sampler>([&](Shader::Sampler *sampler, size_t index, bool &stop) {
				D3D12_STATIC_SAMPLER_DESC &samplerDesc = samplerDescriptors[index];

				D3D12_FILTER filter = D3D12_FILTER_ANISOTROPIC;
				D3D12_COMPARISON_FUNC comparisonFunction = D3D12_COMPARISON_FUNC_NEVER;
				if(sampler->GetComparisonFunction() == Shader::Sampler::ComparisonFunction::Never)
				{
					switch (sampler->GetFilter())
					{
						case Shader::Sampler::Filter::Anisotropic:
							filter = D3D12_FILTER_ANISOTROPIC;
							break;
						case Shader::Sampler::Filter::Linear:
							filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
							break;
						case Shader::Sampler::Filter::Nearest:
							filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
							break;
					}
				}
				else
				{
					switch (sampler->GetFilter())
					{
						case Shader::Sampler::Filter::Anisotropic:
							filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
							break;
						case Shader::Sampler::Filter::Linear:
							filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
							break;
						case Shader::Sampler::Filter::Nearest:
							filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
							break;
					}

					switch(sampler->GetComparisonFunction())
					{
						case Shader::Sampler::ComparisonFunction::Always:
							comparisonFunction = D3D12_COMPARISON_FUNC_ALWAYS;
							break;
						case Shader::Sampler::ComparisonFunction::Equal:
							comparisonFunction = D3D12_COMPARISON_FUNC_EQUAL;
							break;
						case Shader::Sampler::ComparisonFunction::GreaterEqual:
							comparisonFunction = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
							break;
						case Shader::Sampler::ComparisonFunction::Greater:
							comparisonFunction = D3D12_COMPARISON_FUNC_GREATER;
							break;
						case Shader::Sampler::ComparisonFunction::Less:
							comparisonFunction = D3D12_COMPARISON_FUNC_LESS;
							break;
						case Shader::Sampler::ComparisonFunction::LessEqual:
							comparisonFunction = D3D12_COMPARISON_FUNC_LESS_EQUAL;
							break;
						case Shader::Sampler::ComparisonFunction::NotEqual:
							comparisonFunction = D3D12_COMPARISON_FUNC_NOT_EQUAL;
							break;
					}
				}

				D3D12_TEXTURE_ADDRESS_MODE addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
				switch(sampler->GetWrapMode())
				{
					case Shader::Sampler::WrapMode::Repeat:
						addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
						break;
					case Shader::Sampler::WrapMode::Clamp:
						addressMode = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						break;
				}

				samplerDesc.Filter = filter;
				samplerDesc.AddressU = addressMode;
				samplerDesc.AddressV = addressMode;
				samplerDesc.AddressW = addressMode;
				samplerDesc.MipLODBias = 0.0f;
				samplerDesc.ComparisonFunc = comparisonFunction;
				samplerDesc.MinLOD = 0.0f;
				samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
				samplerDesc.MaxAnisotropy = sampler->GetAnisotropy();
				samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
				samplerDesc.ShaderRegister = index;
				samplerDesc.RegisterSpace = 0;
				samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//TODO: Restrict visibility to the shader actually using it
			});
		}

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(numberOfTables, rootParameters, signature->samplers->GetCount(), samplerDescriptors, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);*/

		/*		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
				//If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

				if(FAILED(renderer->GetD3D12Device()->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
				{
					featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
				}*/

/*		ID3DBlob *signatureBlob = nullptr;
		ID3DBlob *error = nullptr;
		HRESULT success = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &error);

		if(FAILED(success))
		{
			String *errorString = RNCSTR("");
			if(error)
			{
				errorString = RNSTR((char*)error->GetBufferPointer());
				error->Release();
			}

			RNDebug(RNSTR("Failed to create root signature with error: " << errorString));
		}

		renderer->GetVulkanDevice()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&signature->signature));

		if (srvCbvRanges)
			delete[] srvCbvRanges;
		if (rootParameters)
			delete[] rootParameters;
		if (samplerDescriptors)
			delete[] samplerDescriptors;*/



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
			if(state->descriptor.renderPass == descriptor.renderPass && state->descriptor.depthStencilFormat == descriptor.depthStencilFormat/* && rootSignature->signature == state->rootSignature->signature*/)
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
//		pipelineCreateInfo.layout = pipelineLayout;
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
		VulkanGPUBuffer *vertexBuffer = nullptr;
		VulkanGPUBuffer *fragmentBuffer = nullptr;
/*		if(vertexShader && vertexShader->GetSignature() && vertexShader->GetSignature()->GetTotalUniformSize())
		{
			vertexBuffer = new VulkanGPUBuffer(vertexShader->GetSignature()->GetTotalUniformSize());
		}
		if(fragmentShader && fragmentShader->GetSignature() && fragmentShader->GetSignature()->GetTotalUniformSize())
		{
			fragmentBuffer = new VulkanGPUBuffer(fragmentShader->GetSignature()->GetTotalUniformSize());
		}*/

		VulkanUniformState *state = new VulkanUniformState();
		state->vertexUniformBuffer = vertexBuffer;
		state->fragmentUniformBuffer = fragmentBuffer;

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

		return state;
	}



	/*	const VulkanPipelineState *VulkanStateCoordinator::GetRenderPipelineStateInCollection(VulkanPipelineStateCollection *collection, Mesh *mesh, Material *material, Camera *camera)
		{
			for(VulkanPipelineState *state : collection->states)
			{
				if(state->textureCount == material->GetTextures()->GetCount())
					return state;
			}

			VkDevice device = _renderer->GetVulkanDevice()->GetDevice();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyState.flags = 0;
			inputAssemblyState.primitiveRestartEnable = VK_FALSE;

			VkPipelineRasterizationStateCreateInfo rasterizationState = {};
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationState.lineWidth = 1.0f;
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
			depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilState.front = depthStencilState.back;

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


			VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
			setUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			setUniformLayoutBinding.binding = 0;
			setUniformLayoutBinding.descriptorCount = 1;

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = { setUniformLayoutBinding };

			size_t textureCount = material->GetTextures()->GetCount();
			for(size_t i = 0; i < textureCount; i++)
			{
				VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
				setImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setImageLayoutBinding.binding = i + 1;
				setImageLayoutBinding.descriptorCount = 1;

				setLayoutBindings.push_back(setImageLayoutBinding);
			}

			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pNext = NULL;
			descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
			descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();

			VkDescriptorSetLayout descriptorSetLayout;
			RNVulkanValidate(vk::CreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, _renderer->GetAllocatorCallback(), &descriptorSetLayout));

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.pNext = NULL;
			pipelineLayoutCreateInfo.setLayoutCount = 1;
			pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

			VkPipelineLayout pipelineLayout;
			RNVulkanValidate(vk::CreatePipelineLayout(device, &pipelineLayoutCreateInfo, _renderer->GetAllocatorCallback(), &pipelineLayout));

			VulkanFramebuffer *framebuffer = nullptr;
			if(camera)
				camera->GetFramebuffer()->Downcast<VulkanFramebuffer>();
			if(!framebuffer)
				framebuffer = Renderer::GetActiveRenderer()->GetMainWindow()->Downcast<VulkanWindow>()->GetFramebuffer();




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
			RNVulkanValidate(vk::CreateGraphicsPipelines(device,  VK_NULL_HANDLE, 1, &pipelineCreateInfo, _renderer->GetAllocatorCallback(), &pipeline));

			// Create the rendering state
			VulkanPipelineState *state = new VulkanPipelineState();
			state->state = pipeline;
			state->descriptorSetLayout = descriptorSetLayout;
			state->pipelineLayout = pipelineLayout;
			state->textureCount = material->GetTextures()->GetCount();

			collection->states.push_back(state);

			return state;
		}

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
