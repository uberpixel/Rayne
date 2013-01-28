//
//  RNMesh.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MESH_H__
#define __RAYNE_MESH_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRenderingResource.h"
#include "RNArray.h"
#include "RNVector.h"
#include "RNColor.h"

namespace RN
{
	typedef enum
	{
		kMeshFeatureVertices = 0,
		kMeshFeatureNormals = 1,
		kMeshFeatureTangents = 2,
		kMeshFeatureColor0 = 3,
		kMeshFeatureColor1 = 4,
		kMeshFeatureUVSet0 = 5,
		kMeshFeatureUVSet1 = 6,
		kMeshFeatureIndices = 7,
		
		__kMaxMeshFeatures = 8
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
		RNAPI MeshLODStage(const Array<MeshDescriptor>& descriptor);
		RNAPI ~MeshLODStage();
		
		template <typename T>
		T *Data(MeshFeature feature)
		{
			return (T *)(_descriptor[(int32)feature]._pointer);
		}
		
		MeshDescriptor *Descriptor(MeshFeature feature)
		{
			return &_descriptor[(int32)feature];
		}
		
		RNAPI void GenerateMesh();
		
		GLuint VBO() const { return _vbo; }
		GLuint IBO() const { return _ibo; }
		
		RNAPI bool SupportsFeature(MeshFeature feature);
		RNAPI size_t OffsetForFeature(MeshFeature feature);
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
		RNAPI Mesh();
		RNAPI virtual ~Mesh();
		
		RNAPI MeshLODStage *AddLODStage(const Array<MeshDescriptor>& descriptor);
		
		RNAPI MeshLODStage *LODStage(int index);
		RNAPI machine_uint LODStages() const { return _LODStages.Count(); }
		
		RNAPI void UpdateMesh();
		
		static Mesh *CubeMesh(const Vector3& size);
		static Mesh *CubeMesh(const Vector3& size, const Color& color);
		
	private:
		Array<MeshLODStage *> _LODStages;
	};
}

#endif /* __RAYNE_MESH_H__ */
