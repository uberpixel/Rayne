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
#include "RND3D12Framebuffer.h"
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

	D3D12RootSignature::~D3D12RootSignature()
	{
		signature->Release();
	}

	D3D12PipelineState::~D3D12PipelineState()
	{
		state->Release();
	}

	D3D12StateCoordinator::D3D12StateCoordinator() :
		_lastDepthStencilState(nullptr)
	{}

	D3D12StateCoordinator::~D3D12StateCoordinator()
	{
		//TODO: Clean up correctly...
	}

	const D3D12RootSignature *D3D12StateCoordinator::GetRootSignature(Material *material)
	{
		D3D12Shader *vertexShader = static_cast<D3D12Shader *>(material->GetVertexShader());
		D3D12Shader *fragmentShader = static_cast<D3D12Shader *>(material->GetFragmentShader());
		const Shader::Signature *vertexSignature = vertexShader->GetSignature();
		const Shader::Signature *fragmentSignature = fragmentShader->GetSignature();
		uint16 textureCount = fmax(vertexSignature->GetTextureCount(), fragmentSignature->GetTextureCount());
		const Array *vertexSamplers = vertexSignature->GetSamplers();
		const Array *fragmentSamplers = fragmentSignature->GetSamplers();
		Array *samplerArray = new Array(vertexSamplers);
		samplerArray->AddObjectsFromArray(fragmentSamplers);
		samplerArray->Autorelease();

		//TODO: Support multiple constant buffers per function signature
		uint16 constantBufferCount = ((vertexSignature->GetTotalUniformSize() > 0) ? 1 : 0) + ((fragmentSignature->GetTotalUniformSize() > 0) ? 1 : 0);

		for(D3D12RootSignature *signature : _rootSignatures)
		{
			
			if(signature->textureCount != textureCount)
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

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		D3D12RootSignature *signature = new D3D12RootSignature();
		signature->constantBufferCount = constantBufferCount;
		signature->samplers = samplerArray->Retain();
		signature->textureCount = textureCount;

		int numberOfTables = (signature->textureCount > 0) + (signature->constantBufferCount > 0);
		CD3DX12_DESCRIPTOR_RANGE1 *srvCbvRanges = nullptr;
		CD3DX12_ROOT_PARAMETER1 *rootParameters = nullptr;

		if(numberOfTables > 0)
		{
			srvCbvRanges = new CD3DX12_DESCRIPTOR_RANGE1[numberOfTables];
			rootParameters = new CD3DX12_ROOT_PARAMETER1[numberOfTables];
		}

		// Perfomance TIP: Order from most frequent to least frequent.
		int tableIndex = 0;
		if(signature->textureCount > 0)
		{
			srvCbvRanges[tableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, signature->textureCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
			rootParameters[tableIndex].InitAsDescriptorTable(1, &srvCbvRanges[tableIndex], D3D12_SHADER_VISIBILITY_ALL);	//TODO: Restrict visibility to the shader actually using it
			tableIndex += 1;
		}
		if(signature->constantBufferCount > 0)
		{
			srvCbvRanges[tableIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, signature->constantBufferCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
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
				switch(sampler->GetFilter())
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
				samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
				samplerDesc.MinLOD = 0.0f;
				samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
				samplerDesc.MaxAnisotropy = sampler->GetAnisotropy();
				samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
				samplerDesc.ShaderRegister = index;
				samplerDesc.RegisterSpace = 0;
				samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//TODO: Restrict visibility to the shader actually using it
			});
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(numberOfTables, rootParameters, signature->samplers->GetCount(), samplerDescriptors, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		//If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if(FAILED(renderer->GetD3D12Device()->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		ID3DBlob *signatureBlob = nullptr;
		ID3DBlob *error = nullptr;
		HRESULT success = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signatureBlob, &error);

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

		renderer->GetD3D12Device()->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&signature->signature));

		if (srvCbvRanges)
			delete[] srvCbvRanges;
		if (rootParameters)
			delete[] rootParameters;
		if (samplerDescriptors)
			delete[] samplerDescriptors;

		_rootSignatures.push_back(signature);
		return signature;
	}

	const D3D12PipelineState *D3D12StateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, D3D12Framebuffer *framebuffer)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		D3D12Shader *vertexShader = static_cast<D3D12Shader *>(material->GetVertexShader());
		D3D12Shader *fragmentShader = static_cast<D3D12Shader *>(material->GetFragmentShader());

		void *vertexFunction = vertexShader->_shader;
		void *fragmentFunction = fragmentShader->_shader;

		for(D3D12PipelineStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->fragmentShader == fragmentFunction && collection->vertexShader == vertexFunction)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, framebuffer, material);
				}
			}
		}

		D3D12PipelineStateCollection *collection = new D3D12PipelineStateCollection(descriptor, vertexFunction, fragmentFunction);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, framebuffer, material);
	}

	const D3D12PipelineState *D3D12StateCoordinator::GetRenderPipelineStateInCollection(D3D12PipelineStateCollection *collection, Mesh *mesh, D3D12Framebuffer *framebuffer, Material *material)
	{
		DXGI_FORMAT pixelFormat = framebuffer->_colorFormat;
		DXGI_FORMAT depthStencilFormat = framebuffer->_depthFormat;

		//TODO: Fix this shit
		for(const D3D12PipelineState *state : collection->states)
		{
			if(state->pixelFormat == pixelFormat && state->depthStencilFormat == depthStencilFormat)
				return state;
		}

		const D3D12RootSignature *rootSignature = GetRootSignature(material);

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		ID3DBlob *vertexShader = static_cast<ID3DBlob*>(collection->vertexShader);
		ID3DBlob *fragmentShader = static_cast<ID3DBlob*>(collection->fragmentShader);

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = rootSignature->signature;
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
		state->rootSignature = rootSignature;
		HRESULT success = renderer->GetD3D12Device()->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&state->state));

		if(FAILED(success))
		{
			return nullptr;
		}

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

		Shader *vertexShader = material->GetVertexShader();
		Shader *fragmentShader = material->GetFragmentShader();
		D3D12UniformBuffer *vertexBuffer = nullptr;
		D3D12UniformBuffer *fragmentBuffer = nullptr;
		if(vertexShader && vertexShader->GetSignature() && vertexShader->GetSignature()->GetTotalUniformSize())
		{
			vertexBuffer = new D3D12UniformBuffer(renderer, vertexShader->GetSignature()->GetTotalUniformSize());
		}
		if(fragmentShader && fragmentShader->GetSignature() && fragmentShader->GetSignature()->GetTotalUniformSize())
		{
			fragmentBuffer = new D3D12UniformBuffer(renderer, fragmentShader->GetSignature()->GetTotalUniformSize());
		}

		D3D12UniformState *state = new D3D12UniformState();
		state->vertexUniformBuffer = vertexBuffer;
		state->fragmentUniformBuffer = fragmentBuffer;

		return state;
	}
}
