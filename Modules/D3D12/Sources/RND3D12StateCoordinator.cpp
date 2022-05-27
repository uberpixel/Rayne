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
	D3D12_BLEND_OP _blendOpLookup[] =
	{
		D3D12_BLEND_OP_ADD,
		D3D12_BLEND_OP_SUBTRACT,
		D3D12_BLEND_OP_REV_SUBTRACT,
		D3D12_BLEND_OP_MIN,
		D3D12_BLEND_OP_MAX
	};

	D3D12_BLEND _blendFactorLookup[] =
	{
		D3D12_BLEND_ZERO,
		D3D12_BLEND_ONE,
		D3D12_BLEND_SRC_COLOR,
		D3D12_BLEND_INV_SRC_COLOR,
		D3D12_BLEND_SRC_ALPHA,
		D3D12_BLEND_INV_SRC_ALPHA,
		D3D12_BLEND_DEST_COLOR,
		D3D12_BLEND_INV_DEST_COLOR,
		D3D12_BLEND_DEST_ALPHA,
		D3D12_BLEND_INV_DEST_ALPHA,
		D3D12_BLEND_SRC_ALPHA_SAT,
		D3D12_BLEND_SRC1_COLOR,
		D3D12_BLEND_INV_SRC1_COLOR,
		D3D12_BLEND_SRC1_ALPHA,
		D3D12_BLEND_INV_SRC1_ALPHA
	};

	DXGI_FORMAT _vertexFormatLookup[] =
	{
		DXGI_FORMAT_UNKNOWN,

		DXGI_FORMAT_R8_UINT,
		DXGI_FORMAT_R16_UINT,
		DXGI_FORMAT_R32_UINT,
		
		DXGI_FORMAT_R8_SINT,
		DXGI_FORMAT_R16_SINT,
		DXGI_FORMAT_R32_SINT,

		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R16G16B16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,

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

		"BONEWEIGHTS",
		"BONEINDICES",

		"CUSTOM"
	};

	uint8 _vertexFeatureIndexLookup[]
	{
		0,
		0,
		0,
		0,
		1,
		0,
		1,

		0,

		0,
		0,

		0
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

	const D3D12RootSignature *D3D12StateCoordinator::GetRootSignature(const D3D12PipelineStateDescriptor &descriptor)
	{
		Array *samplerArray = new Array();
		samplerArray->Autorelease();
		std::vector<uint16> bindingIndex;
		std::vector<uint8> bindingType;

		D3D12Shader *vertexShader = static_cast<D3D12Shader *>(descriptor.vertexShader);
		D3D12Shader *fragmentShader = static_cast<D3D12Shader *>(descriptor.fragmentShader);

		const Shader::Signature *vertexShaderSignature = vertexShader ? vertexShader->GetSignature() : nullptr;
		const Shader::Signature *fragmentShaderSignature = fragmentShader ? fragmentShader->GetSignature() : nullptr;

		uint8 vertexTextureCount = 0;
		uint8 fragmentTextureCount = 0;
		uint8 vertexUniformBufferCount = 0;
		uint8 fragmentUniformBufferCount = 0;
		
		if(vertexShaderSignature)
		{
			vertexShaderSignature->GetBuffers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(0);
			});

			vertexShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(1);
			});

			vertexShaderSignature->GetTextures()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(2);
			});

			vertexTextureCount = vertexShaderSignature->GetTextures()->GetCount();
			vertexUniformBufferCount = vertexShaderSignature->GetBuffers()->GetCount();

			samplerArray->AddObjectsFromArray(vertexShaderSignature->GetSamplers());
		}
		
		if(fragmentShaderSignature)
		{
			fragmentShaderSignature->GetBuffers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(0);
			});

			fragmentShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(1);
			});

			fragmentShaderSignature->GetTextures()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(2);
			});

			fragmentTextureCount = fragmentShaderSignature->GetTextures()->GetCount();
			fragmentUniformBufferCount = fragmentShaderSignature->GetBuffers()->GetCount();

			samplerArray->AddObjectsFromArray(fragmentShaderSignature->GetSamplers());
		}

		for(D3D12RootSignature *signature : _rootSignatures)
		{
			if(signature->vertexTextureCount != vertexTextureCount) continue;
			if(signature->vertexUniformBufferCount != vertexUniformBufferCount) continue;
			if(signature->fragmentTextureCount != fragmentTextureCount) continue;
			if(signature->fragmentUniformBufferCount != fragmentUniformBufferCount) continue;
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

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		D3D12RootSignature *signature = new D3D12RootSignature();
		signature->bindingIndex = bindingIndex;
		signature->bindingType = bindingType;
		signature->samplers = samplerArray->Retain();
		signature->vertexTextureCount = vertexTextureCount;
		signature->vertexUniformBufferCount = vertexUniformBufferCount;
		signature->fragmentTextureCount = fragmentTextureCount;
		signature->fragmentUniformBufferCount = fragmentUniformBufferCount;

		int numberOfTables = (vertexTextureCount > 0 || vertexUniformBufferCount > 0) + (fragmentTextureCount > 0 || fragmentUniformBufferCount > 0);

		CD3DX12_DESCRIPTOR_RANGE *srvCbvRanges = nullptr;
		CD3DX12_ROOT_PARAMETER *rootParameters = nullptr;

		if(numberOfTables > 0)
		{
			srvCbvRanges = new CD3DX12_DESCRIPTOR_RANGE[vertexTextureCount + vertexUniformBufferCount + fragmentTextureCount + fragmentUniformBufferCount];
			rootParameters = new CD3DX12_ROOT_PARAMETER[numberOfTables];
		}

		// Perfomance TIP: Order from most frequent to least frequent.
		int tableIndex = 0;
		int parameterIndex = 0;
		
		if(vertexTextureCount > 0 || vertexUniformBufferCount > 0)
		{
			vertexShaderSignature->GetBuffers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				srvCbvRanges[tableIndex + index].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, argument->GetIndex(), 0);
			});
			
			vertexShaderSignature->GetTextures()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				srvCbvRanges[tableIndex + vertexUniformBufferCount + index].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, argument->GetIndex(), 0);
			});
			
			rootParameters[parameterIndex++].InitAsDescriptorTable(vertexTextureCount + vertexUniformBufferCount, &srvCbvRanges[tableIndex], D3D12_SHADER_VISIBILITY_VERTEX);
			tableIndex += vertexUniformBufferCount + vertexTextureCount;
		}

		if(fragmentTextureCount > 0 || fragmentUniformBufferCount > 0)
		{
			fragmentShaderSignature->GetBuffers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				srvCbvRanges[tableIndex + index].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, argument->GetIndex(), 0);
			});

			fragmentShaderSignature->GetTextures()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop) {
				srvCbvRanges[tableIndex + fragmentUniformBufferCount + index].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, argument->GetIndex(), 0);
			});

			rootParameters[parameterIndex++].InitAsDescriptorTable(fragmentTextureCount + fragmentUniformBufferCount, &srvCbvRanges[tableIndex], D3D12_SHADER_VISIBILITY_PIXEL);
			tableIndex += fragmentTextureCount + fragmentUniformBufferCount;
		}

		//TODO: Create samplers for both, vertex and fragment shaders
		// Create samplers
		D3D12_STATIC_SAMPLER_DESC *samplerDescriptors = nullptr;
		if(signature->samplers->GetCount() > 0)
		{
			samplerDescriptors = new D3D12_STATIC_SAMPLER_DESC[signature->samplers->GetCount()];
			signature->samplers->Enumerate<Shader::ArgumentSampler>([&](Shader::ArgumentSampler *sampler, size_t index, bool &stop) {
				D3D12_STATIC_SAMPLER_DESC &samplerDesc = samplerDescriptors[index];

				D3D12_FILTER filter = D3D12_FILTER_ANISOTROPIC;
				D3D12_COMPARISON_FUNC comparisonFunction = D3D12_COMPARISON_FUNC_NEVER;
				if(sampler->GetComparisonFunction() == Shader::ArgumentSampler::ComparisonFunction::Never)
				{
					switch (sampler->GetFilter())
					{
					case Shader::ArgumentSampler::Filter::Anisotropic:
						filter = D3D12_FILTER_ANISOTROPIC;
						break;
					case Shader::ArgumentSampler::Filter::Linear:
						filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
						break;
					case Shader::ArgumentSampler::Filter::Nearest:
						filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
						break;
					}
				}
				else
				{
					switch (sampler->GetFilter())
					{
					case Shader::ArgumentSampler::Filter::Anisotropic:
						filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
						break;
					case Shader::ArgumentSampler::Filter::Linear:
						filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
						break;
					case Shader::ArgumentSampler::Filter::Nearest:
						filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
						break;
					}

					switch(sampler->GetComparisonFunction())
					{
					case Shader::ArgumentSampler::ComparisonFunction::Always:
						comparisonFunction = D3D12_COMPARISON_FUNC_ALWAYS;
						break;
					case Shader::ArgumentSampler::ComparisonFunction::Equal:
						comparisonFunction = D3D12_COMPARISON_FUNC_EQUAL;
						break;
					case Shader::ArgumentSampler::ComparisonFunction::GreaterEqual:
						comparisonFunction = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
						break;
					case Shader::ArgumentSampler::ComparisonFunction::Greater:
						comparisonFunction = D3D12_COMPARISON_FUNC_GREATER;
						break;
					case Shader::ArgumentSampler::ComparisonFunction::Less:
						comparisonFunction = D3D12_COMPARISON_FUNC_LESS;
						break;
					case Shader::ArgumentSampler::ComparisonFunction::LessEqual:
						comparisonFunction = D3D12_COMPARISON_FUNC_LESS_EQUAL;
						break;
					case Shader::ArgumentSampler::ComparisonFunction::NotEqual:
						comparisonFunction = D3D12_COMPARISON_FUNC_NOT_EQUAL;
						break;
					}
				}

				D3D12_TEXTURE_ADDRESS_MODE addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
				switch(sampler->GetWrapMode())
				{
				case Shader::ArgumentSampler::WrapMode::Repeat:
					addressMode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
					break;
				case Shader::ArgumentSampler::WrapMode::Clamp:
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
		rootSignatureDesc.Init(numberOfTables, rootParameters, signature->samplers->GetCount(), samplerDescriptors, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

/*		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		//If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if(FAILED(renderer->GetD3D12Device()->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}*/

		ID3DBlob *signatureBlob = nullptr;
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

	const D3D12PipelineState *D3D12StateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, D3D12Framebuffer *framebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();
		Material::Properties mergedMaterialProperties = material->GetMergedProperties(overrideMaterial);
		D3D12PipelineStateDescriptor pipelineDescriptor;
		pipelineDescriptor.sampleCount = framebuffer->GetSampleCount();
		pipelineDescriptor.sampleQuality = (framebuffer->_colorTargets.size() > 0 && !framebuffer->GetSwapChain()) ? framebuffer->_colorTargets[0]->targetView.texture->GetDescriptor().sampleQuality : 0;

		for(D3D12Framebuffer::D3D12ColorTargetView *targetView : framebuffer->_colorTargets)
		{
			pipelineDescriptor.colorFormats.push_back(targetView->d3dTargetViewDesc.Format);
		}
		pipelineDescriptor.depthStencilFormat = (framebuffer->_depthStencilTarget) ? framebuffer->_depthStencilTarget->d3dTargetViewDesc.Format : DXGI_FORMAT_UNKNOWN;
		pipelineDescriptor.shaderHint = shaderHint;
		pipelineDescriptor.vertexShader = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders))? overrideMaterial->GetVertexShader(pipelineDescriptor.shaderHint) : material->GetVertexShader(pipelineDescriptor.shaderHint);
		pipelineDescriptor.fragmentShader = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders)) ? overrideMaterial->GetFragmentShader(pipelineDescriptor.shaderHint) : material->GetFragmentShader(pipelineDescriptor.shaderHint);
		pipelineDescriptor.cullMode = mergedMaterialProperties.cullMode;
		pipelineDescriptor.usePolygonOffset = mergedMaterialProperties.usePolygonOffset;
		pipelineDescriptor.polygonOffsetFactor = mergedMaterialProperties.polygonOffsetFactor;
		pipelineDescriptor.polygonOffsetUnits = mergedMaterialProperties.polygonOffsetUnits;
		pipelineDescriptor.blendOperationRGB = mergedMaterialProperties.blendOperationRGB;
		pipelineDescriptor.blendOperationAlpha = mergedMaterialProperties.blendOperationAlpha;
		pipelineDescriptor.blendFactorSourceRGB = mergedMaterialProperties.blendFactorSourceRGB;
		pipelineDescriptor.blendFactorSourceAlpha = mergedMaterialProperties.blendFactorSourceAlpha;
		pipelineDescriptor.blendFactorDestinationRGB = mergedMaterialProperties.blendFactorDestinationRGB;
		pipelineDescriptor.blendFactorDestinationAlpha = mergedMaterialProperties.blendFactorDestinationAlpha;
		pipelineDescriptor.useAlphaToCoverage = mergedMaterialProperties.useAlphaToCoverage;
		pipelineDescriptor.colorWriteMask = mergedMaterialProperties.colorWriteMask;
		pipelineDescriptor.depthMode = mergedMaterialProperties.depthMode;
		pipelineDescriptor.depthWriteEnabled = mergedMaterialProperties.depthWriteEnabled;
		//TODO: Support all override flags and all the relevant material properties

		for(D3D12PipelineStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->vertexShader == pipelineDescriptor.vertexShader && collection->fragmentShader == pipelineDescriptor.fragmentShader)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, pipelineDescriptor);
				}
			}
		}

		D3D12PipelineStateCollection *collection = new D3D12PipelineStateCollection(descriptor, pipelineDescriptor.vertexShader, pipelineDescriptor.fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, pipelineDescriptor);
	}

	const D3D12PipelineState *D3D12StateCoordinator::GetRenderPipelineStateInCollection(D3D12PipelineStateCollection *collection, Mesh *mesh, const D3D12PipelineStateDescriptor &descriptor)
	{
		const D3D12RootSignature *rootSignature = GetRootSignature(descriptor);

		//TODO: Make sure all possible cases are covered... Depth bias for example... cullmode...
		for(const D3D12PipelineState *state : collection->states)
		{
			if(state->descriptor.colorFormats == descriptor.colorFormats && state->descriptor.sampleCount == descriptor.sampleCount && state->descriptor.sampleQuality == descriptor.sampleQuality && state->descriptor.depthStencilFormat == descriptor.depthStencilFormat && rootSignature->signature == state->rootSignature->signature)
			{
				if(state->descriptor.cullMode == descriptor.cullMode && state->descriptor.usePolygonOffset == descriptor.usePolygonOffset && state->descriptor.polygonOffsetFactor == descriptor.polygonOffsetFactor && state->descriptor.polygonOffsetUnits == descriptor.polygonOffsetUnits && state->descriptor.depthMode == descriptor.depthMode && state->descriptor.depthWriteEnabled == descriptor.depthWriteEnabled)
				{
					if(state->descriptor.blendOperationRGB == descriptor.blendOperationRGB && state->descriptor.blendOperationAlpha == descriptor.blendOperationAlpha && state->descriptor.blendFactorSourceRGB == descriptor.blendFactorSourceRGB && state->descriptor.blendFactorSourceAlpha == descriptor.blendFactorSourceAlpha && state->descriptor.blendFactorDestinationRGB == descriptor.blendFactorDestinationRGB && state->descriptor.blendFactorDestinationAlpha == descriptor.blendFactorDestinationAlpha && state->descriptor.useAlphaToCoverage == descriptor.useAlphaToCoverage && state->descriptor.colorWriteMask == descriptor.colorWriteMask)
						return state;
				}
			}
		}

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		D3D12Shader *vertexShaderRayne = collection->vertexShader->Downcast<D3D12Shader>();
		D3D12Shader *fragmentShaderRayne = collection->fragmentShader->Downcast<D3D12Shader>();
		ID3DBlob *vertexShader = vertexShaderRayne?static_cast<ID3DBlob*>(vertexShaderRayne->_shader) : nullptr;
		ID3DBlob *fragmentShader = fragmentShaderRayne ? static_cast<ID3DBlob*>(fragmentShaderRayne->_shader) : nullptr;

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = rootSignature->signature;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs = CreateVertexElementDescriptorsFromMesh(mesh);
		psoDesc.InputLayout = { inputElementDescs.data(), static_cast<UINT>(inputElementDescs.size()) };
		psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		if(fragmentShader)
			psoDesc.PS = { reinterpret_cast<UINT8*>(fragmentShader->GetBufferPointer()), fragmentShader->GetBufferSize() };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FrontCounterClockwise = true;

		if(descriptor.usePolygonOffset)
		{
			psoDesc.RasterizerState.DepthBias = descriptor.polygonOffsetUnits;
			psoDesc.RasterizerState.SlopeScaledDepthBias = descriptor.polygonOffsetFactor;
			//psoDesc.RasterizerState.DepthBiasClamp = D3D12_FLOAT32_MAX;
		}

		switch(descriptor.cullMode)
		{
		case CullMode::BackFace:
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			break;
		case CullMode::FrontFace:
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			break;
		case CullMode::None:
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			break;
		}
		
		D3D12_BLEND_DESC blendDescription;
		blendDescription.AlphaToCoverageEnable = descriptor.useAlphaToCoverage ? TRUE : FALSE;
		blendDescription.IndependentBlendEnable = TRUE;

		D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc;
		renderTargetBlendDesc.BlendEnable = (descriptor.blendOperationRGB != BlendOperation::None && descriptor.blendOperationAlpha != BlendOperation::None);
		renderTargetBlendDesc.LogicOpEnable = FALSE;
		renderTargetBlendDesc.SrcBlend = _blendFactorLookup[static_cast<int>(descriptor.blendFactorSourceRGB)];
		renderTargetBlendDesc.DestBlend = _blendFactorLookup[static_cast<int>(descriptor.blendFactorDestinationRGB)];
		renderTargetBlendDesc.BlendOp = _blendOpLookup[static_cast<int>(descriptor.blendOperationRGB)];
		renderTargetBlendDesc.SrcBlendAlpha = _blendFactorLookup[static_cast<int>(descriptor.blendFactorSourceAlpha)];
		renderTargetBlendDesc.DestBlendAlpha = _blendFactorLookup[static_cast<int>(descriptor.blendFactorDestinationAlpha)];
		renderTargetBlendDesc.BlendOpAlpha = _blendOpLookup[static_cast<int>(descriptor.blendOperationAlpha)];
		renderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		renderTargetBlendDesc.RenderTargetWriteMask = descriptor.colorWriteMask;

		for(UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			blendDescription.RenderTarget[i] = renderTargetBlendDesc;
		}

		psoDesc.BlendState = blendDescription;

		if(descriptor.depthStencilFormat != DXGI_FORMAT_UNKNOWN)
		{
			psoDesc.DepthStencilState.DepthEnable = TRUE;
			switch(descriptor.depthMode)
			{
			case DepthMode::Never:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
				break;

			case DepthMode::Always:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
				break;

			case DepthMode::Less:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
				break;

			case DepthMode::LessOrEqual:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				break;

			case DepthMode::Equal:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
				break;

			case DepthMode::NotEqual:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
				break;

			case DepthMode::GreaterOrEqual:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				break;

			case DepthMode::Greater:
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
				break;
			}
			
			psoDesc.DepthStencilState.DepthWriteMask = descriptor.depthWriteEnabled?D3D12_DEPTH_WRITE_MASK_ALL: D3D12_DEPTH_WRITE_MASK_ZERO;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.DSVFormat = descriptor.depthStencilFormat;
		}
		else
		{
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
		}
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
		psoDesc.SampleDesc.Quality = descriptor.sampleQuality;

		D3D12PipelineState *state = new D3D12PipelineState();
		state->descriptor = std::move(descriptor);
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

			//TODO: support multiple texcoords/other stuff
			D3D12_INPUT_ELEMENT_DESC element = {};
			element.SemanticName = _vertexFeatureLookup[static_cast<int>(attribute.GetFeature())];
			element.SemanticIndex = _vertexFeatureIndexLookup[static_cast<int>(attribute.GetFeature())];
			element.Format = _vertexFormatLookup[static_cast<int>(attribute.GetType())];
			element.InputSlot = 0;
			element.AlignedByteOffset = attribute.GetOffset();
			element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;
			inputElementDescs.push_back(element);
		}

		return inputElementDescs;
	}

	D3D12UniformState *D3D12StateCoordinator::GetUniformStateForPipelineState(const D3D12PipelineState *pipelineState)
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		D3D12UniformState *state = new D3D12UniformState();
		Shader *vertexShader = pipelineState->descriptor.vertexShader;
		Shader *fragmentShader = pipelineState->descriptor.fragmentShader;
		if (vertexShader && vertexShader->GetSignature())
		{
			vertexShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop) {
				size_t totalSize = buffer->GetTotalUniformSize();
				if (totalSize > 0)
				{
					state->uniformBufferToArgumentMapping.push_back(buffer->Retain());
					state->vertexUniformBuffers.push_back(renderer->GetUniformBufferReference(buffer->GetTotalUniformSize(), buffer->GetIndex())->Retain());
				}
			});
		}

		if (fragmentShader && fragmentShader->GetSignature())
		{
			fragmentShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop) {
				size_t totalSize = buffer->GetTotalUniformSize();
				if (totalSize > 0)
				{
					state->uniformBufferToArgumentMapping.push_back(buffer->Retain());
					state->fragmentUniformBuffers.push_back(renderer->GetUniformBufferReference(buffer->GetTotalUniformSize(), buffer->GetIndex())->Retain());
				}
			});
		}

		return state;
	}
}
