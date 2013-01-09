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
#include "RNRenderingResource.h"
#include "RNArray.h"
#include "RNVector.h"

namespace RN
{
	typedef enum
	{
		kMeshFeatureVertices = 0,
		kMeshFeatureNormals = 1,
		kMeshFeatureColor0 = 2,
		kMeshFeatureColor1 = 3,
		kMeshFeatureUVSet0 = 4,
		kMeshFeatureUVSet1 = 5,
		kMeshFeatureIndices = 6,
		
		__kMaxMeshFeatures = 7
	} MeshFeature;
	
	class MeshLODStage;
	struct MeshDescriptor
	{
	friend class MeshLODStage;
	public:
		MeshFeature feature;
		
		int32 elementMember;
		size_t elementSize;
		size_t elementCount;
		
	private:
		uint8 *_pointer;
		size_t _offset;
		
		bool _available;
	};
	
	class MeshLODStage
	{
	public:
		MeshLODStage(const Array<MeshDescriptor>& descriptor);
		~MeshLODStage();
		
		template <typename T>
		T *Data(MeshFeature feature)
		{
			return (T *)(_descriptor[(int32)feature]._pointer);
		}
		
		MeshDescriptor *Descriptor(MeshFeature feature)
		{
			return &_descriptor[(int32)feature];
		}
		
		void GenerateMesh();
		
		GLuint VBO() const { return _vbo; }
		GLuint IBO() const { return _ibo; }
		
		bool SupportsFeature(MeshFeature feature);
		size_t OffsetForFeature(MeshFeature feature);
		size_t Stride() const { return _stride; };
		
	private:
		struct
		{
			GLuint _vbo;
			GLuint _ibo;
		};
		
		size_t _stride;
		
		size_t _meshSize;
		size_t _indicesSize;
		
		void *_meshData;
		void *_indices;
		MeshDescriptor _descriptor[__kMaxMeshFeatures];
	};
	
	class Mesh : public Object, public RenderingResource
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
