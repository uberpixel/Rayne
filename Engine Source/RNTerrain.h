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
#include "RNAlgorithm.h"

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
		RNAPI void SetCube(Vector3 position, Vector3 size) override;
		RNAPI void RemoveCube(Vector3 position, Vector3 size) override;
		
		RNAPI void GenerateMeshWithMarchingCubes();
		
	private:
		struct __SurfaceNetNode
		{
			Vector3 position;
			uint32 connections[6];
		};
		
		struct __LookupHashMarchingCubes
		{
			size_t operator()(const std::pair<uint32, uint32>& lookup) const
			{
				size_t hash = std::hash<uint32>{}(std::min(lookup.first, lookup.second));
				HashCombine(hash, std::max(lookup.first, lookup.second));
				return hash;
			}
		};
		
		struct __LookupCompareMarchingCubes
		{
			bool operator()(const std::pair<uint32, uint32>& lookup1, const std::pair<uint32, uint32>& lookup2) const
			{
				return ((lookup1.first == lookup2.first && lookup1.second == lookup2.second) || (lookup1.first == lookup2.second && lookup1.second == lookup2.first));
			}
		};
		
		void Initialize();
		Vector3 LerpSurface(const Vector3 &p1, const Vector3 &p2, uint8 d1, uint8 d2) const;
		
		Observable<Vector3, Terrain> _resolution;
		uint8 _surface;
		
		Mesh *_mesh;
		Material *_material;
		Matrix _transform;
		
		PointGrid *_voxels;
		
		RNDeclareMeta(Terrain)
	};
	
	RNObjectClass(Terrain)
}

#endif /* __RAYNE_TERRAIN_H__ */
