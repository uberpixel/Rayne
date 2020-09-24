//
//  RNVoxelEntity.h
//  Rayne
//
//  Copyright 2020 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __Rayne__VOXEL_ENTITY__
#define __Rayne__VOXEL_ENTITY__

#include "RNSceneNode.h"
#include "../Math/RNAlgorithm.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNMaterial.h"
#include "../Rendering/RNMesh.h"

namespace RN
{
	class VoxelEntity : public SceneNode
	{
	public:
		struct Point
		{
		public:
			Point(uint8 d = 0) : density(d), smoothDensity(d) {}
			uint8 density;
			uint8 smoothDensity;
		};
		
		RNAPI VoxelEntity(uint32 resolutionX = 64, uint32 resolutionY = 32, uint32 resolutionZ = 64);
		RNAPI ~VoxelEntity();
		
		RNAPI void SetVoxel(uint32 x, uint32 y, uint32 z, const Point &voxel);
		RNAPI uint8 GetVoxel(uint32 x, uint32 y, uint32 z) const;
		RNAPI uint8 GetVoxel(const Vector3 &position) const;
		RNAPI uint8 GetSmooth(const Vector3 &position) const;
		
		RNAPI void ApplyBlur(Vector3 from, Vector3 to, uint8 radius);
		
		RNAPI uint32 GetResolutionX() const { return _resolutionX; }
		RNAPI uint32 GetResolutionY() const { return _resolutionY; }
		RNAPI uint32 GetResolutionZ() const { return _resolutionZ; }
		
		RNAPI void MakePlane(uint32 height);
		RNAPI void SetCubeLocal(Vector3 position, Vector3 size, uint32 density=255);
		RNAPI void SetSphereLocal(Vector3 position, float radius, uint32 density=255);
		
		RNAPI void SetSphere(Vector3 position, float radius);
		RNAPI void RemoveSphere(Vector3 position, float radius);
		RNAPI void SetCube(Vector3 position, Vector3 size);
		RNAPI void RemoveCube(Vector3 position, Vector3 size);
		
		RNAPI void SetMaterial(Material *material);
		RNAPI void UpdateMesh();
		
		RNAPI bool CanRender(Renderer *renderer, Camera *camera) const override;
		RNAPI void Render(Renderer *renderer, Camera *camera) const override;
		
	private:
		Vector3 LerpSurface(const Vector3 &p1, const Vector3 &p2, uint8 d1, uint8 d2) const;
		uint32 GetID(const Vector3 &position) const;
		
		Point *_voxels;
		
		uint32 _resolutionX;
		uint32 _resolutionY;
		uint32 _resolutionZ;

		uint8 _surface;

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
		
		Mesh *_mesh;
		Material *_material;
		Drawable *_drawable;

		RNDeclareMeta(VoxelEntity)
	};
}

#endif /* defined(__Rayne__VOXEL_ENTITY__) */
