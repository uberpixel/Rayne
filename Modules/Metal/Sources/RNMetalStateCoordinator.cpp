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
		MTLVertexFormatInvalid,
		
		MTLVertexFormatUChar2,
		MTLVertexFormatUShort2,
		MTLVertexFormatUInt,

		MTLVertexFormatChar2,
		MTLVertexFormatShort2,
		MTLVertexFormatInt,

		MTLVertexFormatHalf,
		MTLVertexFormatHalf2,
		MTLVertexFormatHalf3,
		MTLVertexFormatHalf4,

		MTLVertexFormatFloat,
		MTLVertexFormatFloat2,
		MTLVertexFormatFloat3,
		MTLVertexFormatFloat4,
		
		MTLVertexFormatFloat2,
		MTLVertexFormatFloat3,
		MTLVertexFormatFloat4,
		
		MTLVertexFormatFloat4,
		MTLVertexFormatFloat4
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
			[descriptor setDepthCompareFunction:CompareFunctionLookup[1]];
		}

		id<MTLDepthStencilState> state = [_device newDepthStencilStateWithDescriptor:descriptor];
		_lastDepthStencilState = new MetalDepthStencilState(materialProperties, state, renderingState->depthFormat, renderingState->stencilFormat);

		_depthStencilStates.push_back(const_cast<MetalDepthStencilState *>(_lastDepthStencilState));
		[descriptor release];

		return _lastDepthStencilState->depthStencilState;
	}

	id<MTLSamplerState> MetalStateCoordinator::GetSamplerStateForSampler(const Shader::ArgumentSampler *samplerDescriptor)
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
			case Shader::ArgumentSampler::WrapMode::Clamp:
				[descriptor setRAddressMode:MTLSamplerAddressModeClampToEdge];
				[descriptor setSAddressMode:MTLSamplerAddressModeClampToEdge];
				[descriptor setTAddressMode:MTLSamplerAddressModeClampToEdge];
				break;
			case Shader::ArgumentSampler::WrapMode::Repeat:
				[descriptor setRAddressMode:MTLSamplerAddressModeRepeat];
				[descriptor setSAddressMode:MTLSamplerAddressModeRepeat];
				[descriptor setTAddressMode:MTLSamplerAddressModeRepeat];
				break;
		}

		MTLSamplerMipFilter mipFilter;
		switch(samplerDescriptor->GetFilter())
		{
			case Shader::ArgumentSampler::Filter::Anisotropic:
			{
				NSUInteger anisotropy = std::min(static_cast<uint8>(16), std::max(static_cast<uint8>(1), samplerDescriptor->GetAnisotropy()));
				[descriptor setMaxAnisotropy:anisotropy];
			}

			case Shader::ArgumentSampler::Filter::Linear:
				[descriptor setMinFilter:MTLSamplerMinMagFilterLinear];
				[descriptor setMagFilter:MTLSamplerMinMagFilterLinear];

				mipFilter = MTLSamplerMipFilterLinear;
				break;

			case Shader::ArgumentSampler::Filter::Nearest:
				[descriptor setMinFilter:MTLSamplerMinMagFilterNearest];
				[descriptor setMagFilter:MTLSamplerMinMagFilterNearest];

				mipFilter = MTLSamplerMipFilterNearest;
				break;
		}
		[descriptor setMipFilter:mipFilter];
		
		switch(samplerDescriptor->GetComparisonFunction())
		{
			case Shader::ArgumentSampler::ComparisonFunction::Never:
				[descriptor setCompareFunction:MTLCompareFunctionNever];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::Less:
				[descriptor setCompareFunction:MTLCompareFunctionLess];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::LessEqual:
				[descriptor setCompareFunction:MTLCompareFunctionLessEqual];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::Equal:
				[descriptor setCompareFunction:MTLCompareFunctionEqual];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::NotEqual:
				[descriptor setCompareFunction:MTLCompareFunctionNotEqual];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::GreaterEqual:
				[descriptor setCompareFunction:MTLCompareFunctionGreaterEqual];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::Greater:
				[descriptor setCompareFunction:MTLCompareFunctionGreater];
				break;
				
			case Shader::ArgumentSampler::ComparisonFunction::Always:
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
		
		Material::Properties materialProperties = material->GetMergedProperties(overrideMaterial);

		for(MetalRenderingStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->fragmentShader->IsEqual(fragmentShader) && collection->vertexShader->IsEqual(vertexShader))
				{
					return GetRenderPipelineStateInCollection(collection, mesh, framebuffer, materialProperties);
				}
			}
		}

		MetalRenderingStateCollection *collection = new MetalRenderingStateCollection(descriptor, vertexShader, fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, framebuffer, materialProperties);
	}

	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Framebuffer *framebuffer, const Material::Properties &materialProperties)
	{
		MetalFramebuffer *metalFramebuffer = framebuffer->Downcast<MetalFramebuffer>();
		MTLPixelFormat pixelFormat = metalFramebuffer->GetMetalColorFormat(0);
		MTLPixelFormat depthFormat = metalFramebuffer->GetMetalDepthFormat();
		MTLPixelFormat stencilFormat = metalFramebuffer->GetMetalStencilFormat();
		uint8 sampleCount = metalFramebuffer->GetSampleCount();
		
		for(const MetalRenderingState *state : collection->states)
		{
			//TODO: This might still be missing some things!?
			if(state->pixelFormat == pixelFormat && state->depthFormat == depthFormat && state->stencilFormat == stencilFormat && state->sampleCount == sampleCount && state->wantsAlphaToCoverage == materialProperties.useAlphaToCoverage && state->colorWriteMask == materialProperties.colorWriteMask && state->blendOperationRGB == materialProperties.blendOperationRGB && state->blendOperationAlpha == materialProperties.blendOperationAlpha && state->blendFactorSourceRGB == materialProperties.blendFactorSourceRGB && state->blendFactorSourceAlpha == materialProperties.blendFactorSourceAlpha && state->blendFactorDestinationRGB == materialProperties.blendFactorDestinationRGB && state->blendFactorDestinationAlpha == materialProperties.blendFactorDestinationAlpha)
				return state;
		}

		MTLVertexDescriptor *vertexDescriptor = CreateVertexDescriptorFromMesh(mesh, static_cast<MetalShader*>(collection->vertexShader));

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = static_cast<id>(collection->vertexShader->_shader);
		pipelineStateDescriptor.fragmentFunction = static_cast<id>(collection->fragmentShader->_shader);
		pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
		pipelineStateDescriptor.sampleCount = sampleCount;
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat; //TODO: Set correct pixel format for each framebuffer texture...
		pipelineStateDescriptor.colorAttachments[0].writeMask = 0;
		if(materialProperties.colorWriteMask & (1 << 0)) pipelineStateDescriptor.colorAttachments[0].writeMask |= MTLColorWriteMaskRed;
		if(materialProperties.colorWriteMask & (1 << 1)) pipelineStateDescriptor.colorAttachments[0].writeMask |= MTLColorWriteMaskGreen;
		if(materialProperties.colorWriteMask & (1 << 2)) pipelineStateDescriptor.colorAttachments[0].writeMask |= MTLColorWriteMaskBlue;
		if(materialProperties.colorWriteMask & (1 << 3)) pipelineStateDescriptor.colorAttachments[0].writeMask |= MTLColorWriteMaskAlpha;
		pipelineStateDescriptor.colorAttachments[0].blendingEnabled = false;
		if(pixelFormat != MTLPixelFormatInvalid && materialProperties.blendOperationRGB != BlendOperation::None && materialProperties.blendOperationAlpha != BlendOperation::None)
		{
			pipelineStateDescriptor.colorAttachments[0].blendingEnabled = true;
			pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = static_cast<MTLBlendOperation>(materialProperties.blendOperationRGB);
			pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = static_cast<MTLBlendFactor>(materialProperties.blendFactorSourceRGB);
			pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = static_cast<MTLBlendFactor>(materialProperties.blendFactorDestinationRGB);
			pipelineStateDescriptor.colorAttachments[0].rgbBlendOperation = static_cast<MTLBlendOperation>(materialProperties.blendOperationAlpha);
			pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = static_cast<MTLBlendFactor>(materialProperties.blendFactorSourceAlpha);
			pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = static_cast<MTLBlendFactor>(materialProperties.blendFactorDestinationAlpha);
		}
		pipelineStateDescriptor.depthAttachmentPixelFormat = depthFormat;
		pipelineStateDescriptor.stencilAttachmentPixelFormat = stencilFormat;
		pipelineStateDescriptor.alphaToCoverageEnabled = materialProperties.useAlphaToCoverage;

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

			RN_ASSERT(!error, "PipelineState creation failed with error: %s", error.localizedDescription.UTF8String);

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
		state->sampleCount = sampleCount;
		state->vertexShader = collection->vertexShader;
		state->fragmentShader = collection->fragmentShader;
		state->vertexPositionBufferShaderResourceIndex = mesh->GetVertexPositionsSeparatedSize() > 0? 29 : 255; //Hardcoded to match value CreateVertexDescriptorFromMesh
		state->vertexBufferShaderResourceIndex = 30; //Hardcoded to match value CreateVertexDescriptorFromMesh
		state->wantsAlphaToCoverage = materialProperties.useAlphaToCoverage;
		state->colorWriteMask = materialProperties.colorWriteMask;
		state->blendOperationRGB = materialProperties.blendOperationRGB;
		state->blendOperationAlpha = materialProperties.blendOperationAlpha;
		state->blendFactorSourceRGB = materialProperties.blendFactorSourceRGB;
		state->blendFactorDestinationRGB = materialProperties.blendFactorDestinationRGB;
		state->blendFactorSourceAlpha = materialProperties.blendFactorSourceAlpha;
		state->blendFactorDestinationAlpha = materialProperties.blendFactorDestinationAlpha;

		collection->states.push_back(state);

		return state;
	}

	MTLVertexDescriptor *MetalStateCoordinator::CreateVertexDescriptorFromMesh(Mesh *mesh, MetalShader *shader)
	{
		MTLVertexDescriptor *descriptor = [[MTLVertexDescriptor alloc] init];
		
		//Hardcoding the binding index to 29 for positions and 30 for everything else here as there is no way to figure out the ones in use before reflection, but need to create the pipeline for reflection to be available... 30 is the highest index allowed!
		
		if(mesh->GetVertexPositionsSeparatedSize() > 0)
		{
			bool didSetBufferAttributes = false;
			
			const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
			for(const Mesh::VertexAttribute &attribute : attributes)
			{
				if(attribute.GetFeature() != Mesh::VertexAttribute::Feature::Vertices) continue;

				uint32 attributeIndex = shader->_hasInputVertexAttribute[static_cast<int>(attribute.GetFeature())];
				if(attributeIndex == -1) continue;
				
				MTLVertexAttributeDescriptor *attributeDescriptor = descriptor.attributes[attributeIndex];
				attributeDescriptor.format = _vertexFormatLookup[static_cast<MTLVertexFormat>(attribute.GetType())];
				attributeDescriptor.offset = attribute.GetOffset();
				attributeDescriptor.bufferIndex = 29;
				
				didSetBufferAttributes = true;
				
				break;
			}
			
			if(didSetBufferAttributes)
			{
				descriptor.layouts[29].stride = mesh->GetVertexPositionsSeparatedStride();
				descriptor.layouts[29].stepFunction = MTLVertexStepFunctionPerVertex;
				descriptor.layouts[29].stepRate = 1;
			}
		}

		bool didSetBufferAttributes = false;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices) continue;
			if(mesh->GetVertexPositionsSeparatedSize() > 0 && attribute.GetFeature() == Mesh::VertexAttribute::Feature::Vertices) continue;

			uint32 attributeIndex = shader->_hasInputVertexAttribute[static_cast<int>(attribute.GetFeature())];
			if(attributeIndex == -1) continue;
			
			MTLVertexAttributeDescriptor *attributeDescriptor = descriptor.attributes[attributeIndex];
			attributeDescriptor.format = _vertexFormatLookup[static_cast<MTLVertexFormat>(attribute.GetType())];
			attributeDescriptor.offset = attribute.GetOffset();
			attributeDescriptor.bufferIndex = 30;
			
			didSetBufferAttributes = true;
		}
		
		if(didSetBufferAttributes)
		{
			descriptor.layouts[30].stride = mesh->GetStride();
			descriptor.layouts[30].stepFunction = MTLVertexStepFunctionPerVertex;
			descriptor.layouts[30].stepRate = 1;
		}

		return descriptor;
	}
}
