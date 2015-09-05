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

namespace RN
{
	class MetalRenderingState : public Object
	{
	public:

	private:
		Shader *_fragmentShader;
		Shader *_vertexShader;
		Material *_material;
		Texture::Format _pixelFormat;


	};

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

	class MetalStateCoordinator
	{
	public:
		MetalStateCoordinator();

		void SetDevice(id<MTLDevice> device);

		id<MTLDepthStencilState> GetDepthStencilStateForMaterial(Material *material);

		id<MTLRenderPipelineState> GetRenderPipelineState(id<MTLDevice> device, Material *material, Mesh *mesh);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh);

		id<MTLDevice> _device;


		std::vector<MetalDepthStencilState> _depthStencilStates;
		const MetalDepthStencilState *_lastDepthStencilState;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
