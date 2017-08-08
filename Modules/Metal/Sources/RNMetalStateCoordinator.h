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
		id<MTLRenderPipelineState> state;
		Shader *vertexShader;
		Shader *fragmentShader;
		bool wantsShadowTexture;
	};

	struct MetalRenderingStateCollection
	{
		MetalRenderingStateCollection() = default;
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
		MetalDepthStencilState(Material *material, id<MTLDepthStencilState> depthStencilState, MTLPixelFormat depth, MTLPixelFormat stencil) :
		mode(material->GetDepthMode()),
		depthWriteEnabled(material->GetDepthWriteEnabled()),
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
		
		RN_INLINE bool MatchesMaterial(Material *material, MTLPixelFormat depth, MTLPixelFormat stencil) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled && depth == depthFormat && stencil == stencilFormat);
		}
	};


	class MetalStateCoordinator
	{
	public:
		MTLAPI MetalStateCoordinator();
		MTLAPI ~MetalStateCoordinator();

		MTLAPI void SetDevice(id<MTLDevice> device);

		MTLAPI id<MTLDepthStencilState> GetDepthStencilStateForMaterial(Material *material, const MetalRenderingState *renderingState);
		MTLAPI id<MTLSamplerState> GetSamplerStateForSampler(const Shader::Sampler *samplerDescriptor);

		MTLAPI const MetalRenderingState *GetRenderPipelineState(Material *material, Mesh *mesh, Framebuffer *framebuffer);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh);
		const MetalRenderingState *GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Framebuffer *framebuffer);

		id<MTLDevice> _device;

		std::mutex _samplerLock;
		std::vector<std::pair<id<MTLSamplerState>, const Shader::Sampler *>> _samplers;

		std::vector<MetalDepthStencilState *> _depthStencilStates;
		const MetalDepthStencilState *_lastDepthStencilState;

		std::vector<MetalRenderingStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
