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
	struct MetalDepthStencilState
	{
		MetalDepthStencilState() = default;
		MetalDepthStencilState(Material *material, id<MTLDepthStencilState> tstate) :
			mode(material->GetDepthMode()),
			depthWriteEnabled(material->GetDepthWriteEnabled()),
			state(tstate)
		{}

		~MetalDepthStencilState()
		{
			[state release];
		}

		DepthMode mode;
		bool depthWriteEnabled;
		id<MTLDepthStencilState> state;

		RN_INLINE bool MatchesMaterial(Material *material) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled);
		}
	};

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


	class MetalStateCoordinator
	{
	public:
		MTLAPI MetalStateCoordinator();
		MTLAPI ~MetalStateCoordinator();

		MTLAPI void SetDevice(id<MTLDevice> device);

		MTLAPI id<MTLDepthStencilState> GetDepthStencilStateForMaterial(Material *material);
		MTLAPI id<MTLSamplerState> GetSamplerStateForTextureParameter(const Texture::Parameter &parameter);

		MTLAPI const MetalRenderingState *GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh);
		const MetalRenderingState *GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Camera *camera);

		id<MTLDevice> _device;

		std::mutex _samplerLock;
		std::vector<std::pair<id<MTLSamplerState>, Texture::Parameter>> _samplers;

		std::vector<MetalDepthStencilState *> _depthStencilStates;
		const MetalDepthStencilState *_lastDepthStencilState;

		std::vector<MetalRenderingStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
