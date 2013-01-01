//
//  RNMesh.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MESH_H__
#define __RAYNE_MESH_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNVector.h"

namespace RN
{
	typedef enum
	{
		kMeshFeatureVertices = 0,
		kMeshFeatureColor0 = 1,
		kMeshFeatureColor1 = 2,
		kMeshFeatureUVSet0 = 3,
		kMeshFeatureUVSet1 = 4,
		kMeshFeatureIndices = 5,
		
		kMaxMeshFeatures = 6
	} MeshFeature;
	
	typedef enum
	{
		kMeshFeatureTypeVector2,
		kMeshFeatureTypeVector3,
		kMeshFeatureTypeVector4,
		kMeshFeatureTypeUint16
	} MeshFeatureType;
	
	class MeshLODStage;
	struct MeshDescriptor
	{
	friend class MeshLODStage;
	public:
		MeshFeature feature;
		MeshFeatureType type;
		
		int32 elements;
		
	private:
		uint8 *_pointer;
		size_t _offset;
		
		bool _dirty;
	};
	
	class MeshLODStage
	{
	public:
		MeshLODStage(const Array<MeshDescriptor>& descriptor);
		~MeshLODStage();
		
		template <typename T>
		T *Data(int32 index) const
		{
			return static_cast<T *>(_descriptor[index]._pointer);
		}
		
		void GenerateMesh();
		
	private:
		static size_t SizeForMeshFeatureType(MeshFeatureType type);
		
		bool _vboToggled;
		
		GLuint _vbo;
		GLuint _ibo;
		
		size_t _meshSize;
		size_t _indicesSize;
		
		void *_meshData;
		void *_indices;
		Array<MeshDescriptor> _descriptor;
	};
	
	class Mesh : public Object
	{
	public:
		Mesh();
		virtual ~Mesh();
		
		MeshLODStage *AddLODStage(const Array<MeshDescriptor>& descriptor);
		
		MeshLODStage *LODStage(int index);
		machine_uint LODStages() const { return _LODStages.Count(); }
		
		void UpdateMesh();
		
	private:
		Array<MeshLODStage *> _LODStages;
	};
}

#endif /* __RAYNE_MESH_H__ */
