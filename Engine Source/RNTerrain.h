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
#include "RNPointGrid.h"
#include "RNSculptable.h"

namespace RN
{
	class Terrain : public Sculptable
	{
	public:
		RNAPI Terrain();
		RNAPI Terrain(const Terrain *other);
		RNAPI Terrain(Deserializer *deserializer);
		RNAPI ~Terrain() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
		RNAPI void SetResolution(const Vector3 &resolution);
		Vector3 GetResolution() const { return _resolution; }
		
		RNAPI void MakePlane(uint32 height);
		RNAPI void SetCubeLocal(Vector3 position, Vector3 size, uint32 density=255);
		RNAPI void SetSphereLocal(Vector3 position, float radius, uint32 density=255);
		
		RNAPI void SetSphere(Vector3 position, float radius) override;
		RNAPI void RemoveSphere(Vector3 position, float radius) override;
		
		RNAPI void GenerateMeshWithMarchingCubes();
		
	private:
		void Initialize();
		
		Observable<Vector3, Terrain> _resolution;
		
		Mesh *_mesh;
		Material *_material;
		Matrix _transform;
		
		PointGrid *_voxels;
		
		RNDeclareMeta(Terrain)
	};
}

#endif /* __RAYNE_TERRAIN_H__ */
