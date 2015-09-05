//
//  RNMetalStateCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALSTATECOORDINATOR_H_
#define __RAYNE_METALSTATECOORDINATOR_H_

#include "../../Base/RNBaseInternal.h"
#include "../../Base/RNBase.h"
#include "../../Objects/RNObject.h"
#include "../../Objects/RNSet.h"
#include "../RNRenderer.h"
#include "../RNMaterial.h"
#include "../RNMesh.h"
#include "../../scene/RNCamera.h"

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

		Material::DepthMode mode;
		bool depthWriteEnabled;
		id<MTLDepthStencilState> state;

		RN_INLINE bool MatchesMaterial(Material *material) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled);
		}
	};

	struct MetalRenderingState
	{
		MTLPixelFormat pixelFormat;
		MTLPixelFormat depthFormat;
		MTLPixelFormat stencilFormat;
		id<MTLRenderPipelineState> state;
	};

	struct MetalRenderingStateCollection
	{
		MetalRenderingStateCollection() = default;
		MetalRenderingStateCollection(const Mesh::VertexDescriptor &tdescriptor, id<MTLFunction> vertex, id<MTLFunction> fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex),
			fragmentShader(fragment)
		{}

		Mesh::VertexDescriptor descriptor;
		id<MTLFunction> vertexShader;
		id<MTLFunction> fragmentShader;

		std::vector<MetalRenderingState> states;
	};

	class MetalStateCoordinator
	{
	public:
		MetalStateCoordinator();

		void SetDevice(id<MTLDevice> device);

		id<MTLDepthStencilState> GetDepthStencilStateForMaterial(Material *material);
		id<MTLRenderPipelineState> GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh);
		id<MTLRenderPipelineState> GetRenderPipelineStateInCollection(MetalRenderingStateCollection &collection, Mesh *mesh, Camera *camera);

		id<MTLDevice> _device;

		std::vector<MetalDepthStencilState> _depthStencilStates;
		const MetalDepthStencilState *_lastDepthStencilState;

		std::vector<MetalRenderingStateCollection> _renderingStates;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
