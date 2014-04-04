//
//  RNTerrain.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TERRAIN_H__
#define __RAYNE_TERRAIN_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNRenderer.h"

namespace RN
{
	class Terrain : public SceneNode
	{
	public:
		RNAPI Terrain();
		RNAPI Terrain(const Terrain *other);
		RNAPI Terrain(Deserializer *deserializer);
		RNAPI ~Terrain() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
	private:
		void Initialize();
		
		Mesh *_mesh;
		Material *_material;
		Matrix _transform;
		
		RNDeclareMeta(Terrain)
	};
}

#endif /* __RAYNE_TERRAIN_H__ */
