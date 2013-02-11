//
//  RNModel.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODEL_H__
#define __RAYNE_MODEL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNArray.h"

namespace RN
{
	class Model : public Object
	{
	public:
		Model(const std::string& path);
		Model(Mesh *mesh, Material *material, const std::string& name="Unnamed");
		
		virtual ~Model();
		
		static Model *WithFile(const std::string& path);
		static Model *WithMesh(Mesh *mesh, Material *material, const std::string& name="Unnamed");
		
		uint32 Meshes() const;
		uint32 Materials() const;
		
		Mesh *MeshAtIndex(uint32 index) const;
		
		Material *MaterialForMesh(const Mesh *mesh) const;
		
	private:
		struct MeshGroup
		{
			std::string name;
			Mesh *mesh;
			Material *material;
		};
		
		void ReadModelVersion1(File *file);
/*		void ReadMaterials(File *file, uint32 count);
		void ReadGroups(File *file, uint32 count);
		
		FeatureDescriptor ReadFeature(File *file);*/
		
		Array<Material> _materials;
		std::vector<MeshGroup> _groups;
	};
}

#endif /* __RAYNE_MODEL_H__ */
