//
//  RND3D12StateCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "d3dx12.h"
#include "RND3D12StateCoordinator.h"
#include "RND3D12Renderer.h"
#include "RND3D12UniformBuffer.h"

namespace RN
{
	DXGI_FORMAT _vertexFormatLookup[] =
	{
		DXGI_FORMAT_R8_UINT,
		DXGI_FORMAT_R16_UINT,
		DXGI_FORMAT_R32_UINT,
		
		DXGI_FORMAT_R8_SINT,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R32_SINT,
		
		DXGI_FORMAT_R32_FLOAT,
		
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT
		};

	const char* _vertexFeatureLookup[]
	{
		"POSITION",
		"NORMAL",
		"TANGENT",
		"COLOR",
		"COLOR",
		"TEXCOORD",
		"TEXCOORD",

		"INDEX",

		"CUSTOM"
	};

/*	MTLCompareFunction CompareFunctionLookup[] =
		{
			MTLCompareFunctionNever,
			MTLCompareFunctionAlways,
			MTLCompareFunctionLess,
			MTLCompareFunctionLessEqual,
			MTLCompareFunctionEqual,
			MTLCompareFunctionNotEqual,
			MTLCompareFunctionGreaterEqual,
			MTLCompareFunctionGreater
		};*/

	D3D12PipelineState::~D3D12PipelineState()
	{
		for(D3D12RenderingStateArgument *argument : vertexArguments)
			delete argument;
		for(D3D12RenderingStateArgument *argument : fragmentArguments)
			delete argument;

		state->Release();
	}

	D3D12StateCoordinator::D3D12StateCoordinator() :
//		_device(nullptr),
		_lastDepthStencilState(nullptr)
	{}

	D3D12StateCoordinator::~D3D12StateCoordinator()
	{
/*		for(D3D12RenderingStateCollection *collection : _renderingStates)
			delete collection;

		for(D3D12DepthStencilState *state : _depthStencilStates)
			delete state;

		for(auto &pair : _samplers)
			[pair.first release];*/
	}

/*	void D3D12StateCoordinator::SetDevice(id<MTLDevice> device)
	{
		_device = device;
	}*/


/*	id<MTLDepthStencilState> D3D12StateCoordinator::GetDepthStencilStateForMaterial(Material *material)
	{
		if(RN_EXPECT_TRUE(_lastDepthStencilState != nullptr) && _lastDepthStencilState->MatchesMaterial(material))
			return _lastDepthStencilState->state;

		for(const D3D12DepthStencilState *state : _depthStencilStates)
		{
			if(state->MatchesMaterial(material))
			{
				_lastDepthStencilState = state;
				return _lastDepthStencilState->state;
			}
		}

		MTLDepthStencilDescriptor *descriptor = [[MTLDepthStencilDescriptor alloc] init];
		[descriptor setDepthCompareFunction:CompareFunctionLookup[static_cast<uint32_t>(material->GetDepthMode())]];
		[descriptor setDepthWriteEnabled:material->GetDepthWriteEnabled()];

		id<MTLDepthStencilState> state = [_device newDepthStencilStateWithDescriptor:descriptor];
		_lastDepthStencilState = new D3D12DepthStencilState(material, state);

		_depthStencilStates.push_back(const_cast<D3D12DepthStencilState *>(_lastDepthStencilState));
		[descriptor release];

		return _lastDepthStencilState->state;
	}

	id<MTLSamplerState> D3D12StateCoordinator::GetSamplerStateForTextureParameter(const Texture::Parameter &parameter)
	{
		std::lock_guard<std::mutex> lock(_samplerLock);

		for(auto &pair : _samplers)
		{
			if(pair.second == parameter)
				return pair.first;
		}


		MTLSamplerDescriptor *descriptor = [[MTLSamplerDescriptor alloc] init];

		switch(parameter.wrapMode)
		{
			case Texture::WrapMode::Clamp:
				[descriptor setRAddressMode:MTLSamplerAddressModeClampToEdge];
				[descriptor setSAddressMode:MTLSamplerAddressModeClampToEdge];
				[descriptor setTAddressMode:MTLSamplerAddressModeClampToEdge];
				break;
			case Texture::WrapMode::Repeat:
				[descriptor setRAddressMode:MTLSamplerAddressModeRepeat];
				[descriptor setSAddressMode:MTLSamplerAddressModeRepeat];
				[descriptor setTAddressMode:MTLSamplerAddressModeRepeat];
				break;
		}

		MTLSamplerMipFilter mipFilter;

		switch(parameter.filter)
		{
			case Texture::Filter::Linear:
				[descriptor setMinFilter:MTLSamplerMinMagFilterLinear];
				[descriptor setMagFilter:MTLSamplerMinMagFilterLinear];

				mipFilter = MTLSamplerMipFilterLinear;
				break;

			case Texture::Filter::Nearest:
				[descriptor setMinFilter:MTLSamplerMinMagFilterNearest];
				[descriptor setMagFilter:MTLSamplerMinMagFilterNearest];

				mipFilter = MTLSamplerMipFilterNearest;
				break;
		}

		[descriptor setMipFilter:mipFilter];

		NSUInteger anisotropy = std::min(static_cast<uint32>(16), std::max(static_cast<uint32>(1), parameter.anisotropy));
		[descriptor setMaxAnisotropy:anisotropy];

		id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:descriptor];
		[descriptor release];

		_samplers.emplace_back(std::make_pair(sampler, parameter));

		return sampler;
	}*/


	const D3D12PipelineState *D3D12StateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		D3D12Shader *vertexShader = static_cast<D3D12Shader *>(material->GetVertexShader());
		D3D12Shader *fragmentShader = static_cast<D3D12Shader *>(material->GetFragmentShader());

		void *vertexFunction = vertexShader->_shader;
		void *fragmentFunction = fragmentShader->_shader;

		for(D3D12RenderingStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->fragmentShader == fragmentFunction && collection->vertexShader == vertexFunction)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, camera);
				}
			}
		}

		D3D12RenderingStateCollection *collection = new D3D12RenderingStateCollection(descriptor, vertexFunction, fragmentFunction);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, camera);
	}

	const D3D12PipelineState *D3D12StateCoordinator::GetRenderPipelineStateInCollection(D3D12RenderingStateCollection *collection, Mesh *mesh, Camera *camera)
	{
		DXGI_FORMAT pixelFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		//TODO: Fix this shit
		for(const D3D12PipelineState *state : collection->states)
		{
			if(state->pixelFormat == pixelFormat && state->depthStencilFormat == depthStencilFormat)
				return state;
		}

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		ID3DBlob *vertexShader = static_cast<ID3DBlob*>(collection->vertexShader);
		ID3DBlob *fragmentShader = static_cast<ID3DBlob*>(collection->fragmentShader);

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = renderer->_rootSignature;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs = CreateVertexElementDescriptorsFromMesh(mesh);
		psoDesc.InputLayout = { inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()) };
		psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<UINT8*>(fragmentShader->GetBufferPointer()), fragmentShader->GetBufferSize() };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.DSVFormat = depthStencilFormat;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = pixelFormat;
		psoDesc.SampleDesc.Count = 1;

		D3D12PipelineState *state = new D3D12PipelineState();
		state->pixelFormat = pixelFormat;
		state->depthStencilFormat = depthStencilFormat;
		HRESULT success = renderer->GetD3D12Device()->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&state->state));

		if(FAILED(success))
		{
			return nullptr;
		}

/*		// Create the rendering state
		D3D12RenderingState *state = new D3D12RenderingState();

		state->vertexArguments.reserve([[reflection vertexArguments] count]);
		state->fragmentArguments.reserve([[reflection fragmentArguments] count]);

		for(MTLArgument *argument in [reflection vertexArguments])
		{
			D3D12RenderingStateArgument *parsed = nullptr;

			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
					parsed = new D3D12RenderingStateUniformBufferArgument(argument);
					break;
				default:
					parsed = new D3D12RenderingStateArgument(argument);
					break;
			}

			state->vertexArguments.push_back(parsed);
		}

		for(MTLArgument *argument in [reflection fragmentArguments])
		{
			D3D12RenderingStateArgument *parsed = nullptr;

			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
					parsed = new D3D12RenderingStateUniformBufferArgument(argument);
					break;
				default:
					parsed = new D3D12RenderingStateArgument(argument);
					break;
			}

			state->fragmentArguments.push_back(parsed);
		}*/

		collection->states.push_back(state);

		return state;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> D3D12StateCoordinator::CreateVertexElementDescriptorsFromMesh(Mesh *mesh)
	{
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

			D3D12_INPUT_ELEMENT_DESC element = {};
			element.SemanticName = _vertexFeatureLookup[static_cast<int>(attribute.GetFeature())];
			element.SemanticIndex = 0;
			element.Format = _vertexFormatLookup[static_cast<int>(attribute.GetType())];
			element.InputSlot = 0;
			element.AlignedByteOffset = attribute.GetOffset();
			element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;
			inputElementDescs.push_back(element);
		}

		return inputElementDescs;
	}

	D3D12UniformState *D3D12StateCoordinator::GetUniformStateForPipelineState(const D3D12PipelineState *pipelineState, Material *material)
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());
		D3D12UniformBuffer *gpuBuffer = new D3D12UniformBuffer(renderer, sizeof(Matrix) * 2 + sizeof(Color) * 2);//renderer->CreateBufferWithLength(sizeof(Matrix) * 2 + sizeof(Color) * 2, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite)->Downcast<D3D12GPUBuffer>();

/*		VkDescriptorBufferInfo uniformBufferDescriptorInfo = {};
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
		}*/

		D3D12UniformState *state = new D3D12UniformState();
		state->uniformBuffer = gpuBuffer;

		return state;
	}
}
