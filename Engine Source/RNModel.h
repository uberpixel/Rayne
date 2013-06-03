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
		Model(const std::string& path);
		Model(Mesh *mesh, Material *material, const std::string& name="Unnamed");
		
		virtual ~Model();
		
		static Model *Empty();
		static Model *WithFile(const std::string& path);
		static Model *WithMesh(Mesh *mesh, Material *material, const std::string& name="Unnamed");
		static Model *WithSkyCube(const std::string& up, const std::string& down, const std::string& left, const std::string& right, const std::string& front, const std::string& back, const std::string& shader="shader/rn_Sky");
		
		uint32 AddLODStage(float distance);
		void RemoveLODStage(uint32 stage);
		
		void AddMesh(Mesh *mesh, Material *material, uint32 lodStage, const std::string& name="Unnamed");
		void RemoveMesh(Mesh *mesh, uint32 lodStage);
		
		uint32 LODStageForDistance(float distance) const;
		
		uint32 Meshes(uint32 lodStage) const;
		
		Mesh *MeshAtIndex(uint32 lodStage, uint32 index) const;
		Material *MaterialAtIndex(uint32 lodStage, uint32 index) const;
		
		const AABB& BoundingBox() const { return _boundingBox; }
		const Sphere& BoundingSphere() const { return _boundingSphere; }
		
	private:
		class MeshGroup
		{
		public:
			MeshGroup(Mesh *tmesh, Material *tmaterial, const std::string& tname) :
				name(tname)
			{
				RN_ASSERT0(tmesh && tmaterial);
				
				mesh = tmesh->Retain();
				material = tmaterial->Retain();
			}
			
			~MeshGroup()
			{
				mesh->Release();
				material->Release();
			}
			
			std::string name;
			
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
		
		void ReadFileAtPath(const std::string& path, LODGroup *group);
		void ReadModelVersion1(File *file, LODGroup *group);
		void CalculateBoundingBox();
		
		Shader *PickShaderForMaterialAndMesh(Material *material, Mesh *mesh);
		Material *PickMaterialForMesh(Mesh *mesh);
		
		AABB _boundingBox;
		Sphere _boundingSphere;
		
		std::vector<LODGroup *> _groups;
		
		RNDefineMetaWithTraits(Model, Object, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_MODEL_H__ */
