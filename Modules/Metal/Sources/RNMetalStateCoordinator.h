//
//  RNMetalStateCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALSTATECOORDINATOR_H_
#define __RAYNE_METALSTATECOORDINATOR_H_

#include "RNMetal.h"
#include "RNMetalShader.h"

namespace RN
{
	struct MetalRenderingState
	{
		~MetalRenderingState()
		{
			[state release];
		}

		MTLPixelFormat pixelFormat;
		MTLPixelFormat depthFormat;
		MTLPixelFormat stencilFormat;
		uint8 sampleCount;
		id<MTLRenderPipelineState> state;
		Shader *vertexShader;
		Shader *fragmentShader;
		uint8 vertexPositionBufferShaderResourceIndex;
		uint8 vertexBufferShaderResourceIndex;
		bool wantsAlphaToCoverage;
		uint8 colorWriteMask;
		
		BlendOperation blendOperationRGB;
		BlendOperation blendOperationAlpha;
		BlendFactor blendFactorSourceRGB;
		BlendFactor blendFactorSourceAlpha;
		BlendFactor blendFactorDestinationRGB;
		BlendFactor blendFactorDestinationAlpha;
	};

	struct MetalRenderingStateCollection
	{
		MetalRenderingStateCollection(const Mesh::VertexDescriptor &tdescriptor, MetalShader *vertex, MetalShader *fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex->Retain()),
			fragmentShader(fragment->Retain())
		{}

		~MetalRenderingStateCollection()
		{
			for(MetalRenderingState *state : states)
				delete state;
		}

		Mesh::VertexDescriptor descriptor;
		MetalShader *vertexShader;
		MetalShader *fragmentShader;

		std::vector<MetalRenderingState *> states;
	};
	
	
	struct MetalDepthStencilState
	{
		MetalDepthStencilState() = default;
		MetalDepthStencilState(const Material::Properties &materialProperties, id<MTLDepthStencilState> depthStencilState, MTLPixelFormat depth, MTLPixelFormat stencil) :
		mode(materialProperties.depthMode),
		depthWriteEnabled(materialProperties.depthWriteEnabled),
		depthStencilState(depthStencilState),
		depthFormat(depth),
		stencilFormat(stencil)
		{}
		
		~MetalDepthStencilState()
		{
			[depthStencilState release];
		}
		
		DepthMode mode;
		bool depthWriteEnabled;
		id<MTLDepthStencilState> depthStencilState;
		MTLPixelFormat depthFormat;
		MTLPixelFormat stencilFormat;
		
		RN_INLINE bool MatchesMaterial(const Material::Properties &materialProperties, MTLPixelFormat depth, MTLPixelFormat stencil) const
		{
			return (materialProperties.depthMode == mode && materialProperties.depthWriteEnabled == depthWriteEnabled && depth == depthFormat && stencil == stencilFormat);
		}
	};


	class MetalStateCoordinator
	{
	public:
		MTLAPI MetalStateCoordinator();
		MTLAPI ~MetalStateCoordinator();

		MTLAPI void SetDevice(id<MTLDevice> device);

		MTLAPI id<MTLDepthStencilState> GetDepthStencilStateForMaterial(const Material::Properties &materialProperties, const MetalRenderingState *renderingState);
		MTLAPI id<MTLSamplerState> GetSamplerStateForSampler(const Shader::ArgumentSampler *samplerDescriptor);

		MTLAPI const MetalRenderingState *GetRenderPipelineState(Material *material, Mesh *mesh, Framebuffer *framebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh, MetalShader *shader);
		const MetalRenderingState *GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Framebuffer *framebuffer, const Material::Properties &materialProperties);

		id<MTLDevice> _device;

		std::mutex _samplerLock;
		std::vector<std::pair<id<MTLSamplerState>, const Shader::ArgumentSampler *>> _samplers;

		std::vector<MetalDepthStencilState *> _depthStencilStates;
		const MetalDepthStencilState *_lastDepthStencilState;

		std::vector<MetalRenderingStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
