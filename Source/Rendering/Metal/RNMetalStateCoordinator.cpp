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

	MetalStateCoordinator::MetalStateCoordinator()
	{

	}

	id<MTLRenderPipelineState> MetalStateCoordinator::GetRenderPipelineState(id<MTLDevice> device, Material *material, Mesh *mesh)
	{
		MTLVertexDescriptor *descriptor = CreateVertexDescriptorFromMesh(mesh);

		MetalShader *vertexShader = static_cast<MetalShader *>(material->GetVertexShader());
		MetalShader *fragmentShader = static_cast<MetalShader *>(material->GetFragmentShader());

		MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineStateDescriptor.vertexFunction = id<MTLFunction>(vertexShader->_shader);
		pipelineStateDescriptor.fragmentFunction = id<MTLFunction>(fragmentShader->_shader);
		pipelineStateDescriptor.vertexDescriptor = descriptor;
		pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

		pipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
		pipelineStateDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;

		return [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:NULL];
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
