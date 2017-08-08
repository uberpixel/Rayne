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


	id<MTLDepthStencilState> MetalStateCoordinator::GetDepthStencilStateForMaterial(Material *material, const MetalRenderingState *renderingState)
	{
		if(RN_EXPECT_TRUE(_lastDepthStencilState != nullptr) && _lastDepthStencilState->MatchesMaterial(material, renderingState->depthFormat, renderingState->stencilFormat))
			return _lastDepthStencilState->depthStencilState;

		for(const MetalDepthStencilState *state : _depthStencilStates)
		{
			if(state->MatchesMaterial(material, renderingState->depthFormat, renderingState->stencilFormat))
			{
				_lastDepthStencilState = state;
				return _lastDepthStencilState->depthStencilState;
			}
		}

		MTLDepthStencilDescriptor *descriptor = [[MTLDepthStencilDescriptor alloc] init];
		
		if(renderingState->depthFormat != MTLPixelFormatInvalid)
		{
			[descriptor setDepthWriteEnabled:material->GetDepthWriteEnabled()];
			[descriptor setDepthCompareFunction:CompareFunctionLookup[static_cast<uint32_t>(material->GetDepthMode())]];
		}
		else
		{
			[descriptor setDepthWriteEnabled:NO];
			[descriptor setDepthCompareFunction:CompareFunctionLookup[0]];
		}

		id<MTLDepthStencilState> state = [_device newDepthStencilStateWithDescriptor:descriptor];
		_lastDepthStencilState = new MetalDepthStencilState(material, state, renderingState->depthFormat, renderingState->stencilFormat);

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

		id<MTLSamplerState> sampler = [_device newSamplerStateWithDescriptor:descriptor];
		[descriptor release];

		_samplers.emplace_back(std::make_pair(sampler, samplerDescriptor->Retain()));

		return sampler;
	}


	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, Framebuffer *framebuffer)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		MetalShader *vertexShader = static_cast<MetalShader *>(material->GetVertexShader());
		MetalShader *fragmentShader = static_cast<MetalShader *>(material->GetFragmentShader());

		for(MetalRenderingStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->fragmentShader->IsEqual(fragmentShader) && collection->vertexShader->IsEqual(vertexShader))
				{
					return GetRenderPipelineStateInCollection(collection, mesh, framebuffer);
				}
			}
		}

		MetalRenderingStateCollection *collection = new MetalRenderingStateCollection(descriptor, vertexShader, fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, framebuffer);

	}

	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Framebuffer *framebuffer)
	{
		MetalFramebuffer *metalFramebuffer = framebuffer->Downcast<MetalFramebuffer>();
		MTLPixelFormat pixelFormat = metalFramebuffer->GetMetalColorFormat();
		MTLPixelFormat depthFormat = metalFramebuffer->GetMetalDepthFormat();
		MTLPixelFormat stencilFormat = metalFramebuffer->GetMetalStencilFormat();
		
		for(const MetalRenderingState *state : collection->states)
		{
			if(state->pixelFormat == pixelFormat && state->depthFormat == depthFormat && state->stencilFormat == stencilFormat)
				return state;
		}

		MTLVertexDescriptor *descriptor = CreateVertexDescriptorFromMesh(mesh);

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = static_cast<id>(collection->vertexShader->_shader);
		pipelineStateDescriptor.fragmentFunction = static_cast<id>(collection->fragmentShader->_shader);
		pipelineStateDescriptor.vertexDescriptor = descriptor;
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
		pipelineStateDescriptor.depthAttachmentPixelFormat = depthFormat;
		pipelineStateDescriptor.stencilAttachmentPixelFormat = stencilFormat;

		id<MTLRenderPipelineState> pipelineState = nil;
		if(collection->vertexShader->GetSignature() && collection->fragmentShader->GetSignature())
		{
			pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:NULL];
		}
		else
		{
			MTLRenderPipelineReflection * reflection;
			pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor options:MTLPipelineOptionBufferTypeInfo reflection:&reflection error:NULL];

			// TODO: Error handling, plox

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
		[descriptor release];

		// Create the rendering state
		MetalRenderingState *state = new MetalRenderingState();
		state->state = pipelineState;
		state->pixelFormat = pixelFormat;
		state->depthFormat = depthFormat;
		state->stencilFormat = stencilFormat;
		state->vertexShader = collection->vertexShader;
		state->fragmentShader = collection->fragmentShader;
		state->wantsShadowTexture = collection->fragmentShader->_wantsDirectionalShadowTexture; //TODO: also support in vertex shader/generalize special texture handling

		collection->states.push_back(state);

		return state;
	}

	MTLVertexDescriptor *MetalStateCoordinator::CreateVertexDescriptorFromMesh(Mesh *mesh)
	{
		MTLVertexDescriptor *descriptor = [[MTLVertexDescriptor alloc] init];
		descriptor.layouts[0].stride = mesh->GetStride();
		descriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
		descriptor.layouts[0].stepRate = 1;

		size_t offset = 0;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();

		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

			MTLVertexAttributeDescriptor *attributeDescriptor = descriptor.attributes[offset];
			attributeDescriptor.format = _vertexFormatLookup[static_cast<MTLVertexFormat>(attribute.GetType())];
			attributeDescriptor.offset = attribute.GetOffset();
			attributeDescriptor.bufferIndex = 0;

			offset ++;
		}

		return descriptor;
	}
}
