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
#include "RNAABB.h"
#include "RNSphere.h"

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
	
	class Mesh;
	struct MeshDescriptor
	{
	friend class Mesh;
	public:
		MeshDescriptor();
		
		MeshFeature feature;
		
		int32 elementMember;
		size_t elementSize;
		size_t elementCount;
		
		size_t offset;
		
	private:
		uint8 *_pointer;
		size_t _size;
		uint32 _useCount;
		
		bool _available;
		bool _dirty;
	};
	
	class Mesh : public Object
	{
	public:
		RNAPI Mesh(const Array<MeshDescriptor>& descriptor);
		RNAPI Mesh(const Array<MeshDescriptor>& descriptor, const void *data);
		RNAPI virtual ~Mesh();
		
		template <typename T>
		T *MutableData(MeshFeature feature)
		{
			return reinterpret_cast<T *>(FetchDataForFeature(feature));
		}
		
		template <typename T>
		const T *Data(MeshFeature feature)
		{
			return reinterpret_cast<const T *>(FetchConstDataForFeature(feature));
		}
		
		template <typename T>
		T *MeshData()
		{
			if(!_meshData)
				_meshData = static_cast<uint8 *>(Memory::AllocateSIMD(_meshSize));
			
			return reinterpret_cast<T *>(_meshData);
		}
		
		MeshDescriptor *Descriptor(MeshFeature feature)
		{
			return &_descriptor[(int32)feature];
		}
		
		RNAPI void UpdateMesh();
		
		RNAPI void SetMode(GLenum mode);
		RNAPI void SetVBOUsage(GLenum usage);
		RNAPI void SetIBOUsage(GLenum usage);
		
		RNAPI void CalculateBoundingBox();
		RNAPI void ReleaseData(MeshFeature feature);
		
		RNAPI bool SupportsFeature(MeshFeature feature);
		RNAPI size_t OffsetForFeature(MeshFeature feature);
		size_t Stride() const { return _stride; };
		
		GLuint VBO() const { return _vbo; }
		GLuint IBO() const { return _ibo; }
		GLenum Mode() const { return _mode; }
		
		const AABB& BoundingBox() const { return _boundingBox; }
		const Sphere& BoundingSphere() const { return _boundingSphere; }
		
		RNAPI static Mesh *PlaneMesh(const Vector3& size = Vector3(1.0, 1.0, 1.0), const Vector3& rotation = Vector3(0.0f, 0.0f, 0.0f));
		RNAPI static Mesh *CubeMesh(const Vector3& size);
		RNAPI static Mesh *CubeMesh(const Vector3& size, const Color& color);
		
	private:
		void Initialize(const Array<MeshDescriptor>& descriptor);
		void GenerateMesh();
		
		const void *FetchConstDataForFeature(MeshFeature feature);
		void *FetchDataForFeature(MeshFeature feature);
		
		struct
		{
			GLuint _vbo;
			GLuint _ibo;
		};
		
		size_t _stride;
		
		size_t _meshSize;
		size_t _indicesSize;
		
		GLenum _vboUsage;
		GLenum _iboUsage;
		GLenum _mode;
		
		AABB _boundingBox;
		Sphere _boundingSphere;
		
		uint8 *_meshData;
		uint8 *_indices;
		MeshDescriptor _descriptor[__kMaxMeshFeatures];
		
		RNDefineConstructorlessMeta(Mesh, Object)
	};
}

#endif /* __RAYNE_MESH_H__ */
