//
//  RNMetalStateCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalStateCoordinator.h"
#include "RNMetalShader.h"

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
			[pair.first release];
	}

	void MetalStateCoordinator::SetDevice(id<MTLDevice> device)
	{
		_device = device;
	}


	id<MTLDepthStencilState> MetalStateCoordinator::GetDepthStencilStateForMaterial(Material *material)
	{
		if(RN_EXPECT_TRUE(_lastDepthStencilState != nullptr) && _lastDepthStencilState->MatchesMaterial(material))
			return _lastDepthStencilState->state;

		for(const MetalDepthStencilState *state : _depthStencilStates)
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
		_lastDepthStencilState = new MetalDepthStencilState(material, state);

		_depthStencilStates.push_back(const_cast<MetalDepthStencilState *>(_lastDepthStencilState));
		[descriptor release];

		return _lastDepthStencilState->state;
	}

	id<MTLSamplerState> MetalStateCoordinator::GetSamplerStateForTextureParameter(const Texture::Parameter &parameter)
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

		MTLSamplerMipFilter mipFilter = MTLSamplerMipFilterNotMipmapped;

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
	}


	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		MetalShader *vertexShader = static_cast<MetalShader *>(material->GetVertexShader());
		MetalShader *fragmentShader = static_cast<MetalShader *>(material->GetFragmentShader());

		id<MTLFunction> vertexFunction = (id<MTLFunction>)vertexShader->_shader;
		id<MTLFunction> fragmentFunction = (id<MTLFunction>)fragmentShader->_shader;

		for(MetalRenderingStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->fragmentShader == fragmentFunction && collection->vertexShader == vertexFunction)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, camera);
				}
			}
		}

		MetalRenderingStateCollection *collection = new MetalRenderingStateCollection(descriptor, vertexFunction, fragmentFunction);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, camera);

	}

	const MetalRenderingState *MetalStateCoordinator::GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Camera *camera)
	{
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

		// TODO: Error handling, plox

		// Create the rendering state
		MetalRenderingState *state = new MetalRenderingState();

		state->vertexArguments.reserve([[reflection vertexArguments] count]);
		state->fragmentArguments.reserve([[reflection fragmentArguments] count]);

		for(MTLArgument *argument in [reflection vertexArguments])
		{
			MetalRenderingStateArgument *parsed = nullptr;

			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
					parsed = new MetalRenderingStateUniformBufferArgument(argument);
					break;
				default:
					parsed = new MetalRenderingStateArgument(argument);
					break;
			}

			state->vertexArguments.push_back(parsed);
		}

		for(MTLArgument *argument in [reflection fragmentArguments])
		{
			MetalRenderingStateArgument *parsed = nullptr;

			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
					parsed = new MetalRenderingStateUniformBufferArgument(argument);
					break;
				default:
					parsed = new MetalRenderingStateArgument(argument);
					break;
			}

			state->fragmentArguments.push_back(parsed);
		}


		state->state = [pipelineState retain];
		state->pixelFormat = pixelFormat;
		state->depthFormat = depthFormat;
		state->stencilFormat = stencilFormat;

		collection->states.push_back(state);

		return state;
	}

	MTLVertexDescriptor *MetalStateCoordinator::CreateVertexDescriptorFromMesh(Mesh *mesh)
	{
		MTLVertexDescriptor *descriptor = [[MTLVertexDescriptor vertexDescriptor] retain];
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
