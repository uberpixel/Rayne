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

	class MetalStateCoordinator
	{
	public:
		MetalStateCoordinator();

		id<MTLRenderPipelineState> GetRenderPipelineState(id<MTLDevice> device, Material *material, Mesh *mesh);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh);

		Set *_set;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
