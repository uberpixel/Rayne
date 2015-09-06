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

	void MetalStateCoordinator::SetDevice(id<MTLDevice> device)
	{
		_device = device;
	}


	id<MTLDepthStencilState> MetalStateCoordinator::GetDepthStencilStateForMaterial(Material *material)
	{
		if(RN_EXPECT_TRUE(_lastDepthStencilState != nullptr) && _lastDepthStencilState->MatchesMaterial(material))
			return _lastDepthStencilState->state;

		for(const MetalDepthStencilState &state : _depthStencilStates)
		{
			if(state.MatchesMaterial(material))
			{
				_lastDepthStencilState = &state;
				return _lastDepthStencilState->state;
			}
		}

		MTLDepthStencilDescriptor *descriptor = [[MTLDepthStencilDescriptor alloc] init];
		[descriptor setDepthCompareFunction:CompareFunctionLookup[static_cast<uint32_t>(material->GetDepthMode())]];
		[descriptor setDepthWriteEnabled:material->GetDepthWriteEnabled()];

		id<MTLDepthStencilState> state = [_device newDepthStencilStateWithDescriptor:descriptor];
		_depthStencilStates.emplace_back(material, state);
		[descriptor release];

		_lastDepthStencilState = &_depthStencilStates.back();
		return _lastDepthStencilState->state;
	}

	id<MTLRenderPipelineState> MetalStateCoordinator::GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera)
	{
		const Mesh::VertexDescriptor &descriptor = mesh->GetVertexDescriptor();

		MetalShader *vertexShader = static_cast<MetalShader *>(material->GetVertexShader());
		MetalShader *fragmentShader = static_cast<MetalShader *>(material->GetFragmentShader());

		id<MTLFunction> vertexFunction = (id<MTLFunction>)vertexShader->_shader;
		id<MTLFunction> fragmentFunction = (id<MTLFunction>)fragmentShader->_shader;

		for(MetalRenderingStateCollection &collection : _renderingStates)
		{
			if(collection.descriptor.IsEqual(descriptor))
			{
				if(collection.fragmentShader == fragmentFunction && collection.vertexShader == vertexFunction)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, camera);
				}
			}
		}

		_renderingStates.emplace_back(descriptor, vertexFunction, fragmentFunction);
		return GetRenderPipelineStateInCollection(_renderingStates.back(), mesh, camera);

	}

	id<MTLRenderPipelineState> MetalStateCoordinator::GetRenderPipelineStateInCollection(MetalRenderingStateCollection &collection, Mesh *mesh, Camera *camera)
	{
		MTLPixelFormat pixelFormat = MTLPixelFormatBGRA8Unorm;
		MTLPixelFormat depthFormat = MTLPixelFormatDepth24Unorm_Stencil8;
		MTLPixelFormat stencilFormat = MTLPixelFormatDepth24Unorm_Stencil8;

		for(const MetalRenderingState &state : collection.states)
		{
			if(state.pixelFormat == pixelFormat && state.depthFormat == depthFormat && state.stencilFormat == stencilFormat)
				return state.state;
		}

		MTLVertexDescriptor *descriptor = CreateVertexDescriptorFromMesh(mesh);

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = collection.vertexShader;
		pipelineStateDescriptor.fragmentFunction = collection.fragmentShader;
		pipelineStateDescriptor.vertexDescriptor = descriptor;
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
		pipelineStateDescriptor.depthAttachmentPixelFormat = depthFormat;
		pipelineStateDescriptor.stencilAttachmentPixelFormat = stencilFormat;

		id<MTLRenderPipelineState> pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:NULL];

		MetalRenderingState state;
		state.state = pipelineState;
		state.pixelFormat = pixelFormat;
		state.depthFormat = depthFormat;
		state.stencilFormat = stencilFormat;

		collection.states.push_back(state);

		return pipelineState;
	}

	MTLVertexDescriptor *MetalStateCoordinator::CreateVertexDescriptorFromMesh(Mesh *mesh)
	{
		MTLVertexDescriptor *descriptor = [[MTLVertexDescriptor vertexDescriptor] retain];
		descriptor.layouts[0].stride = mesh->GetStride();
		descriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
		descriptor.layouts[0].stepRate = 1;

		size_t offset = 0;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh->GetVertexAttributes();
		const Mesh::VertexAttribute *indicesAttribute = nullptr;

		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
			{
				indicesAttribute = &attribute;
				continue;
			}

			MTLVertexAttributeDescriptor *attributeDescriptor = descriptor.attributes[offset];
			attributeDescriptor.format = _vertexFormatLookup[static_cast<MTLVertexFormat>(attribute.GetType())];
			attributeDescriptor.offset = attribute.GetOffset();
			attributeDescriptor.bufferIndex = 0;

			offset ++;
		}

		return descriptor;
	}
}
