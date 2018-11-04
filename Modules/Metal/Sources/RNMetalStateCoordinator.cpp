//
//  RNMetalStateCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalStateCoordinator.h"
#include "RNMetalShader.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalTexture.h"

namespace RN
{
	MTLVertexFormat _vertexFormatLookup[] =
		{
			MTLVertexFormatUChar2,
			MTLVertexFormatUShort2,
			MTLVertexFormatUInt,

			MTLVertexFormatChar2,
			MTLVertexFormatShort2,
			MTLVertexFormatInt,

			MTLVertexFormatFloat,

			MTLVertexFormatFloat2,
			MTLVertexFormatFloat3,
			MTLVertexFormatFloat4,

			MTLVertexFormatFloat4,
			MTLVertexFormatFloat4,
			MTLVertexFormatFloat4
		};
	
	uint32 _vertexFeatureLookup[]
	{
		0, //"POSITION",
		1, //"NORMAL",
		2, //"TANGENT",
		3, //"COLOR",
		4, //"COLOR",
		5, //"TEXCOORD",
		6, //"TEXCOORD",
		
		//"INDEX",
		
		7 //"CUSTOM"
	};

	MTLCompareFunction CompareFunctionLookup[] =
		{
			MTLCompareFunctionNever,
			MTLCompareFunctionAlways,
			MTLCompareFunctionLess,
			MTLCompareFunctionLessEqual,
			MTLCompareFunctionEqual,
			MTLCompareFunctionNotEqual,
			MTLCompareFunctionGreaterEqual,
			MTLCompareFunctionGreater
		};

	MetalStateCoordinator::MetalStateCoordinator() :
		_device(nullptr),
		_lastDepthStencilState(nullptr)
	{}

	MetalStateCoordinator::~MetalStateCoordinator()
	{
		for(MetalRenderingStateCollection *collection : _renderingStates)
			delete collection;

		for(MetalDepthStencilState *state : _depthStencilStates)
			delete state;

		for(auto &pair : _samplers)
		{
			[pair.first release];
			pair.second->Release();
		}
	}

	void MetalStateCoordinator::SetDevice(id<MTLDevice> device)
	{
		_device = device;
	}


	id<MTLDepthStencilState> MetalStateCoordinator::GetDepthStencilStateForMaterial(const Material::Properties &materialProperties, const MetalRenderingState *renderingState)
	{
		if(RN_EXPECT_TRUE(_lastDepthStencilState != nullptr) && _lastDepthStencilState->MatchesMaterial(materialProperties, renderingState->depthFormat, renderingState->stencilFormat))
			return _lastDepthStencilState->depthStencilState;

		for(const MetalDepthStencilState *state : _depthStencilStates)
		{
			if(state->MatchesMaterial(materialProperties, renderingState->depthFormat, renderingState->stencilFormat))
			{
				_lastDepthStencilState = state;
				return _lastDepthStencilState->depthStencilState;
			}
		}

		MTLDepthStencilDescriptor *descriptor = [[MTLDepthStencilDescriptor alloc] init];
		
		if(renderingState->depthFormat != MTLPixelFormatInvalid)
		{
			[descriptor setDepthWriteEnabled:materialProperties.depthWriteEnabled];
			[descriptor setDepthCompareFunction:CompareFunctionLookup[static_cast<uint32_t>(materialProperties.depthMode)]];
		}
		else
		{
			[descriptor setDepthWriteEnabled:NO];
			[descriptor setDepthCompareFunction:CompareFunctionLookup[0]];
		}

		id<MTLDepthStencilState> state = [_device newDepthStencilStateWithDescriptor:descriptor];
		_lastDepthStencilState = new MetalDepthStencilState(materialProperties, state, renderingState->depthFormat, renderingState->stencilFormat);

		_depthStencilStates.push_back(const_cast<MetalDepthStencilState *>(_lastDepthStencilState));
		[descriptor release];

		return _lastDepthStencilState->depthStencilState;
	}

	id<MTLSamplerState> MetalStateCoordinator::GetSamplerStateForSampler(const Shader::Sampler *samplerDescriptor)
	{
		std::lock_guard<std::mutex> lock(_samplerLock);

		for(auto &pair : _samplers)
		{
			if(pair.second == samplerDescriptor)
				return pair.first;
		}


		MTLSamplerDescriptor *descriptor = [[MTLSamplerDescriptor alloc] init];

		switch(samplerDescriptor->GetWrapMode())
		{
			case Shader::Sampler::WrapMode::Clamp:
				[descriptor setRAddressMode:MTLSamplerAddressModeClampToEdge];
				[descriptor setSAddressMode:MTLSamplerAddressModeClampToEdge];
				[descriptor setTAddressMode:MTLSamplerAddressModeClampToEdge];
				break;
			case Shader::Sampler::WrapMode::Repeat:
				[descriptor setRAddressMode:MTLSamplerAddressModeRepeat];
				[descriptor setSAddressMode:MTLSamplerAddressModeRepeat];
				[descriptor setTAddressMode:MTLSamplerAddressModeRepeat];
				break;
		}

		MTLSamplerMipFilter mipFilter;
		switch(samplerDescriptor->GetFilter())
		{
			case Shader::Sampler::Filter::Anisotropic:
			{
				NSUInteger anisotropy = std::min(static_cast<uint8>(16), std::max(static_cast<uint8>(1), samplerDescriptor->GetAnisotropy()));
				[descriptor setMaxAnisotropy:anisotropy];
			}

			case Shader::Sampler::Filter::Linear:
				[descriptor setMinFilter:MTLSamplerMinMagFilterLinear];
				[descriptor setMagFilter:MTLSamplerMinMagFilterLinear];

				mipFilter = MTLSamplerMipFilterLinear;
				break;

			case Shader::Sampler::Filter::Nearest:
				[descriptor setMinFilter:MTLSamplerMinMagFilterNearest];
				[descriptor setMagFilter:MTLSamplerMinMagFilterNearest];

				mipFilter = MTLSamplerMipFilterNearest;
				break;
		}
		[descriptor setMipFilter:mipFilter];
		
		switch(samplerDescriptor->GetComparisonFunction())
		{
			case Shader::Sampler::ComparisonFunction::Never:
				[descriptor setCompareFunction:MTLCompareFunctionNever];
				break;
				
			case Shader::Sampler::ComparisonFunction::Less:
				[descriptor setCompareFunction:MTLCompareFunctionLess];
				break;
				
			case Shader::Sampler::ComparisonFunction::LessEqual:
				[descriptor setCompareFunction:MTLCompareFunctionLessEqual];
				break;
				
			case Shader::Sampler::ComparisonFunction::Equal:
				[descriptor setCompareFunction:MTLCompareFunctionEqual];
				break;
				
			case Shader::Sampler::ComparisonFunction::NotEqual:
				[descriptor setCompareFunction:MTLCompareFunctionNotEqual];
				break;
				
			case Shader::Sampler::ComparisonFunction::GreaterEqual:
				[descriptor setCompareFunction:MTLCompareFunctionGreaterEqual];
				break;
				
			case Shader::Sampler::ComparisonFunction::Greater:
				[descriptor setCompareFunction:MTLCompareFunctionGreater];
				break;
				
			case Shader::Sampler::ComparisonFunction::Always:
				[descriptor setCompareFunction:MTLCompareFunctionAlways];
				break;
		}

		id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:descriptor];
		[descriptor release];

		_samplers.emplace_back(std::make_pair(sampler, samplerDescriptor->Retain()));

		return sampler;
	}

	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, Framebuffer *framebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		MetalShader *vertexShader = static_cast<MetalShader *>((overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders))? overrideMaterial->GetVertexShader(shaderHint) : material->GetVertexShader(shaderHint));
		MetalShader *fragmentShader = static_cast<MetalShader *>((overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupShaders) && !(material->GetOverride() & Material::Override::GroupShaders))? overrideMaterial->GetFragmentShader(shaderHint) : material->GetFragmentShader(shaderHint));
		bool wantsAlphaToCoverage = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::GroupAlphaToCoverage) && !(material->GetOverride() & Material::Override::GroupAlphaToCoverage))? overrideMaterial->GetUseAlphaToCoverage() : material->GetUseAlphaToCoverage();
		uint8 colorWriteMask = (overrideMaterial && !(overrideMaterial->GetOverride() & Material::Override::ColorWriteMask) && !(material->GetOverride() & Material::Override::ColorWriteMask))? overrideMaterial->GetColorWriteMask() : material->GetColorWriteMask();

		for(MetalRenderingStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->fragmentShader->IsEqual(fragmentShader) && collection->vertexShader->IsEqual(vertexShader))
				{
					return GetRenderPipelineStateInCollection(collection, mesh, framebuffer, wantsAlphaToCoverage, colorWriteMask);
				}
			}
		}

		MetalRenderingStateCollection *collection = new MetalRenderingStateCollection(descriptor, vertexShader, fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, framebuffer, wantsAlphaToCoverage, colorWriteMask);

	}

	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Framebuffer *framebuffer, bool wantsAlphaToCoverage, uint8 colorWriteMask)
	{
		MetalFramebuffer *metalFramebuffer = framebuffer->Downcast<MetalFramebuffer>();
		MTLPixelFormat pixelFormat = metalFramebuffer->GetMetalColorFormat(0);
		MTLPixelFormat depthFormat = metalFramebuffer->GetMetalDepthFormat();
		MTLPixelFormat stencilFormat = metalFramebuffer->GetMetalStencilFormat();
		
		for(const MetalRenderingState *state : collection->states)
		{
			//TODO: include things like different pixel formats and sample rate...
			if(state->pixelFormat == pixelFormat && state->depthFormat == depthFormat && state->stencilFormat == stencilFormat && state->wantsAlphaToCoverage == wantsAlphaToCoverage && state->colorWriteMask == colorWriteMask)
				return state;
		}

		MTLVertexDescriptor *vertexDescriptor = CreateVertexDescriptorFromMesh(mesh);

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = static_cast<id>(collection->vertexShader->_shader);
		pipelineStateDescriptor.fragmentFunction = static_cast<id>(collection->fragmentShader->_shader);
		pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
		pipelineStateDescriptor.sampleCount = metalFramebuffer->GetSampleCount();
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat; //TODO: Set correct pixel format for each framebuffer texture...
		pipelineStateDescriptor.colorAttachments[0].writeMask = static_cast<MTLColorWriteMask>(colorWriteMask);
		pipelineStateDescriptor.depthAttachmentPixelFormat = depthFormat;
		pipelineStateDescriptor.stencilAttachmentPixelFormat = stencilFormat;
		pipelineStateDescriptor.alphaToCoverageEnabled = wantsAlphaToCoverage;

		id<MTLRenderPipelineState> pipelineState = nil;
		if(collection->vertexShader->GetSignature() && collection->fragmentShader->GetSignature())
		{
			pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:NULL];
		}
		else
		{
			MTLRenderPipelineReflection * reflection;
			
			NSError *error = nil;
			pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor options:MTLPipelineOptionBufferTypeInfo reflection:&reflection error:&error];

			//TODO: Include error...
			RN_ASSERT(!error, "PipelineState creation failed");

			if(!collection->vertexShader->GetSignature())
			{
				collection->vertexShader->SetReflectedArguments([reflection vertexArguments]);
			}

			if(!collection->fragmentShader->GetSignature())
			{
				collection->fragmentShader->SetReflectedArguments([reflection fragmentArguments]);
			}
		}

		[pipelineStateDescriptor release];
		[vertexDescriptor release];

		// Create the rendering state
		MetalRenderingState *state = new MetalRenderingState();
		state->state = pipelineState;
		state->pixelFormat = pixelFormat;
		state->depthFormat = depthFormat;
		state->stencilFormat = stencilFormat;
		state->vertexShader = collection->vertexShader;
		state->fragmentShader = collection->fragmentShader;
		state->wantsShadowTexture = collection->fragmentShader->_wantsDirectionalShadowTexture; //TODO: also support in vertex shader/generalize special texture handling
		state->wantsAlphaToCoverage = wantsAlphaToCoverage;
		state->colorWriteMask = colorWriteMask;

		collection->states.push_back(state);

		return state;
	}

	MTLVertexDescriptor *MetalStateCoordinator::CreateVertexDescriptorFromMesh(Mesh *mesh)
	{
		MTLVertexDescriptor *descriptor = [[MTLVertexDescriptor alloc] init];
		descriptor.layouts[0].stride = mesh->GetStride();
		descriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
		descriptor.layouts[0].stepRate = 1;

		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
		
		for(int i = 0; i < 7; i++)
		{
			MTLVertexAttributeDescriptor *attributeDescriptor = descriptor.attributes[i];
			attributeDescriptor.format = MTLVertexFormatFloat2;
			attributeDescriptor.offset = 0;
			attributeDescriptor.bufferIndex = 0;
		}

		size_t index = 0;
		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

			MTLVertexAttributeDescriptor *attributeDescriptor = descriptor.attributes[_vertexFeatureLookup[static_cast<int>(attribute.GetFeature())]];
			attributeDescriptor.format = _vertexFormatLookup[static_cast<MTLVertexFormat>(attribute.GetType())];
			attributeDescriptor.offset = attribute.GetOffset();
			attributeDescriptor.bufferIndex = 0;

			index ++;
		}

		return descriptor;
	}
}