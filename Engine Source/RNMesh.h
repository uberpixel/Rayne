//
//  RNMesh.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MESH_H__
#define __RAYNE_MESH_H__

#include "RNBase.h"
#include "RNObject.h"
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
		kMeshFeatureBoneWeights = 7,
		kMeshFeatureBoneIndices = 8,
		kMeshFeatureIndices = 9,
		
		__kMaxMeshFeatures = 10
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
		
		size_t offset;
		
	private:
		uint8 *_pointer;
		size_t _size;
		
		bool _available;
	};
	
	class MeshLODStage
	{
	public:
		RNAPI MeshLODStage(const Array<MeshDescriptor>& descriptor);
		RNAPI MeshLODStage(const Array<MeshDescriptor>& descriptor, const void *data);
		RNAPI ~MeshLODStage();
		
		template <typename T>
		T *Data(MeshFeature feature)
		{
			return static_cast<T *>(FetchDataForFeature(feature));
		}
		
		MeshDescriptor *Descriptor(MeshFeature feature)
		{
			return &_descriptor[(int32)feature];
		}
		
		RNAPI void GenerateMesh();
		
		RNAPI bool SupportsFeature(MeshFeature feature);
		RNAPI size_t OffsetForFeature(MeshFeature feature);
		size_t Stride() const { return _stride; };
		
		GLuint VBO() const { return _vbo; }
		GLuint IBO() const { return _ibo; }
		
		RNAPI bool InstancingData(size_t size, GLuint *outVBO, void **outData);
		
	private:
		struct
		{
			GLuint _vbo;
			GLuint _ibo;
		};
		
		struct
		{
			GLuint _vbo;
			size_t _size;
			void *_data;
		} _instancing;
		
		void *FetchDataForFeature(MeshFeature feature);
		
		size_t _stride;
		
		size_t _meshSize;
		size_t _indicesSize;
		
		void *_meshData;
		void *_indices;
		MeshDescriptor _descriptor[__kMaxMeshFeatures];
	};
	
	class Mesh : public Object
	{
	public:
		RNAPI Mesh();
		RNAPI virtual ~Mesh();
		
		RNAPI MeshLODStage *AddLODStage(const Array<MeshDescriptor>& descriptor);
		RNAPI MeshLODStage *AddLODStage(const Array<MeshDescriptor>& descriptor, const void *data);
		
		RNAPI MeshLODStage *LODStage(int index);
		RNAPI machine_uint LODStages() const { return _LODStages.Count(); }
		
		RNAPI void UpdateMesh();
		
		RNAPI static Mesh *PlaneMesh(const Vector3& size = Vector3(1.0, 1.0, 1.0), const Vector3& rotation = Vector3(0.0f, 0.0f, 0.0f));
		RNAPI static Mesh *CubeMesh(const Vector3& size);
		RNAPI static Mesh *CubeMesh(const Vector3& size, const Color& color);
		
	private:
		Array<MeshLODStage *> _LODStages;
		
		RNDefineMeta(Mesh, Object)
	};
}

#endif /* __RAYNE_MESH_H__ */
