//
//  RNModel.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODEL_H_
#define __RAYNE_MODEL_H_

#include "../Base/RNBase.h"
#include "../Assets/RNAsset.h"
#include "RNMesh.h"
#include "RNMaterial.h"

namespace RN
{
	class Model : public Asset
	{
	public:
		class LODStage : public Object
		{
		public:
			friend class Model;

			LODStage(float distance) :
				_distance(distance)
			{}

			LODStage(const LODStage *other) :
				_distance(other->_distance),
				_groups(other->_groups),
				_index(other->_index)
			{}

			void AddMesh(Mesh *mesh, Material *material)
			{
				_groups.emplace_back(mesh, material);
			}

			void ReplaceMesh(Mesh *mesh, size_t index)
			{
				auto &group = _groups[index];
				group._mesh->Autorelease();
				group._mesh = mesh->Retain();
			}
			void ReplaceMaterial(Material *material, size_t index)
			{
				auto &group = _groups[index];
				group._material->Autorelease();
				group._material = material->Retain();
			}

			Mesh *GetMeshAtIndex(size_t index) const { return _groups[index]._mesh; }
			Material *GetMaterialAtIndex(size_t index) const { return _groups[index]._material;	}

			size_t GetCount() const { return _groups.size(); }
			size_t GetIndex() const { return _index; }
			float GetDistance() const { return _distance; }

		private:
			struct Group
			{
				Group(Mesh *mesh, Material *material) :
					_material(material->Retain()),
					_mesh(mesh->Retain())
				{}

				Group(const Group &other) :
					_material(other._material->Retain()),
					_mesh(other._mesh->Retain())
				{}

				Group &operator =(const Group &other)
				{
					_material->Autorelease();
					_mesh->Autorelease();

					_material = other._material->Retain();
					_mesh = other._mesh->Retain();

					return *this;
				}

				~Group()
				{
					_material->Release();
					_mesh->Release();
				}


				Material *_material;
				Mesh *_mesh;
			};

			float _distance;
			std::vector<Group> _groups;
			size_t _index;

			RNDeclareMeta(LODStage)
		};

		RNAPI Model();
		RNAPI Model(Mesh *mesh, Material *material);
		RNAPI Model(const Model *other);
		RNAPI ~Model();

		RNAPI static Model *WithName(const String *name, const Dictionary *settings = nullptr);

		RNAPI LODStage *AddLODStage(float distance);
		RNAPI void RemoveLODStage(size_t index);

		RNAPI LODStage *GetLODStage(size_t index) const;
		RNAPI LODStage *GetLODStageForDistance(float distance) const;
		size_t GetLODStageCount() const { return _lodStages->GetCount(); }

		RNAPI void CalculateBoundingVolumes();

		const AABB &GetBoundingBox() const { return _boundingBox; }
		const Sphere &GetBoundingSphere() const { return _boundingSphere; }

		RNAPI static const std::vector<float> &GetDefaultLODFactors();
		RNAPI static void SetDefaultLODFactors(const std::vector<float> &factors);

	private:
		Array *_lodStages;

		AABB _boundingBox;
		Sphere _boundingSphere;

		RNDeclareMeta(Model)
	};
}


#endif /* __RAYNE_MODEL_H_ */
