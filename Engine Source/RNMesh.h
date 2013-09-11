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
#include "RNHit.h"

namespace RN
{
	enum
	{
		kMeshFeatureVertices,
		kMeshFeatureNormals,
		kMeshFeatureTangents,
		kMeshFeatureColor0,
		kMeshFeatureColor1,
		kMeshFeatureUVSet0,
		kMeshFeatureUVSet1,
		kMeshFeatureBoneWeights,
		kMeshFeatureBoneIndices,
		kMeshFeatureIndices,
		
		kMeshFeatureCustom = kMeshFeatureIndices + 1
	};
	
	enum DescriptorFlags
	{
		DescriptorFlagSIMDAlignment
	};
	
	typedef uint32 MeshFeature;
	
	struct MeshDescriptor
	{
	friend class Mesh;
	public:
		MeshDescriptor(MeshFeature feature, uint32 flags=0);
		
		MeshFeature feature;
		uint32 flags;
		
		int32 elementMember;
		size_t elementSize;
		size_t elementCount;
		
		size_t offset;
		
	private:
		size_t _size;
		size_t _alignment;
		
		uint8 *_pointer;
		size_t _useCount;
	};
	
	
	
	class Mesh : public Object
	{
	public:
		RNAPI Mesh(const std::vector<MeshDescriptor>& descriptor);
		RNAPI Mesh(const std::vector<MeshDescriptor>& descriptor, const void *data);
		RNAPI ~Mesh() override;
		
		RNAPI void AddDescriptor(const std::vector<MeshDescriptor>& descriptor);
		RNAPI void RemoveDescriptor(MeshFeature feature);
		RNAPI MeshDescriptor *GetDescriptor(MeshFeature feature);
		
		RNAPI bool CanMergeMesh(Mesh *mesh);
		RNAPI void MergeMesh(Mesh *mesh);
		
		RNAPI void UpdateMesh(bool force=false);
							
		template<typename T>
		T *GetElement(MeshFeature feature, size_t index)
		{
			return reinterpret_cast<T *>(FetchElement(feature, index));
		}
		
		template<typename T>
		T *GetElement(MeshFeature feature)
		{
			return reinterpret_cast<T *>(CopyElement(feature));
		}
		
		template<typename T>
		T *GetMeshData()
		{
			_dirty = true;
			return reinterpret_cast<T *>(_meshData);
		}
		
		template<typename T>
		T *GetIndicesData()
		{
			_dirtyIndices = true;
			return reinterpret_cast<T *>(_indices);
		}
		
		RNAPI void SetMode(GLenum mode);
		RNAPI void SetVBOUsage(GLenum usage);
		RNAPI void SetIBOUsage(GLenum usage);
		RNAPI void SetElement(MeshFeature feature, void *data);
		
		RNAPI void ReleaseElement(MeshFeature feature);
		RNAPI void CalculateBoundingBox();
		
		RNAPI bool SupportsFeature(MeshFeature feature);
		RNAPI size_t OffsetForFeature(MeshFeature feature);
		
		RNAPI Hit IntersectsRay(const Vector3 &position, const Vector3 &direction);
		
		size_t GetStride() const { return _stride; };
		
		GLuint GetVBO();
		GLuint GetIBO();
		
		GLenum GetMode() const { return _mode; }
		
		const AABB& GetBoundingBox() const { return _boundingBox; }
		const Sphere& GetBoundingSphere() const { return _boundingSphere; }
		
		RNAPI static Mesh *PlaneMesh(const Vector3& size = Vector3(1.0, 1.0, 1.0), const Vector3& rotation = Vector3(0.0f, 0.0f, 0.0f));
		RNAPI static Mesh *CubeMesh(const Vector3& size);
		RNAPI static Mesh *CubeMesh(const Vector3& size, const Color& color);
		
	private:
		void Initialize();
		void AllocateStorage();
		void RecalculateInternalData();
		
		Hit IntersectsRay3DWithIndices(MeshDescriptor *positionDescriptor, MeshDescriptor *indicesDescriptor, const Vector3 &position, const Vector3 &direction);
		Hit IntersectsRay2DWithIndices(MeshDescriptor *positionDescriptor, MeshDescriptor *indicesDescriptor, const Vector3 &position, const Vector3 &direction);
		Hit IntersectsRay3DWithoutIndices(MeshDescriptor *positionDescriptor, const Vector3 &position, const Vector3 &direction);
		Hit IntersectsRay2DWithoutIndices(MeshDescriptor *positionDescriptor, const Vector3 &position, const Vector3 &direction);
		float RayTriangleIntersection(const Vector3 &pos, const Vector3 &dir, const Vector3 &vert1, const Vector3 &vert2, const Vector3 &vert3);
		
		void *FetchElement(MeshFeature feature, size_t index);
		void *CopyElement(MeshFeature feature);
		
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
		
		bool _dirty;
		bool _dirtyIndices;
		
		std::vector<MeshDescriptor> _descriptor;
		RNDefineMeta(Mesh, Object)
	};
}

#endif /* __RAYNE_MESH_H__ */
