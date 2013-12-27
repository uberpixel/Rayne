//
//  RNModel.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODEL_H__
#define __RAYNE_MODEL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNArray.h"
#include "RNAABB.h"
#include "RNSphere.h"

namespace RN
{
	class Skeleton;
	class Model : public Object
	{
	public:
		Model();
		Model(Mesh *mesh, Material *material);
		
		~Model() override;
		
		static Model *Empty();
		static Model *WithFile(const std::string& path, bool guessMaterial = true);
		static Model *WithMesh(Mesh *mesh, Material *material);
		static Model *WithSkyCube(const std::string& up, const std::string& down, const std::string& left, const std::string& right, const std::string& front, const std::string& back, const std::string& shader="shader/rn_Sky");
		
		size_t AddLODStage(float distance);
		void RemoveLODStage(size_t stage);
		
		void SetSkeleton(Skeleton *skeleton){_skeleton = skeleton;}
		Skeleton *GetSkeleton(){return _skeleton;}
		
		void AddMesh(Mesh *mesh, Material *material, size_t lodStage);
		
		size_t GetLODStageForDistance(float distance) const;
		size_t GetMeshCount(size_t lodStage) const;
		size_t GetLODStageCount() const { return _groups.size(); }
		
		Mesh *GetMeshAtIndex(size_t lodStage, size_t index) const;
		Material *GetMaterialAtIndex(size_t lodStage, size_t index) const;
		
		void CalculateBoundingVolumes();
		
		const AABB& GetBoundingBox() const { return _boundingBox; }
		const Sphere& GetBoundingSphere() const { return _boundingSphere; }
		
	private:
		class MeshGroup
		{
		public:
			MeshGroup(Mesh *tmesh, Material *tmaterial)
			{
				RN_ASSERT(tmesh && tmaterial, "Mesh and Material must not be NULL");
				
				mesh = tmesh->Retain();
				material = tmaterial->Retain();
			}
			
			~MeshGroup()
			{
				mesh->Release();
				material->Release();
			}
			
			Mesh *mesh;
			Material *material;
		};
		
		class LODGroup
		{
		public:
			LODGroup(float distance) :
				lodDistance(distance)
			{}
			
			~LODGroup()
			{
				for(MeshGroup *group : groups)
				{
					delete group;
				}
			}
			
			std::vector<MeshGroup *> groups;
			float lodDistance;
		};
		
		Shader *PickShaderForMaterialAndMesh(Material *material, Mesh *mesh);
		Material *PickMaterialForMesh(Mesh *mesh);
		
		AABB _boundingBox;
		Sphere _boundingSphere;
		
		std::vector<LODGroup *> _groups;
		Skeleton *_skeleton;
		
		RNDefineMetaWithTraits(Model, Object, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_MODEL_H__ */
