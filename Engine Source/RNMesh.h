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
		
		size_t elementMember;
		size_t elementSize;
		
		size_t offset;
		
	private:
		size_t _size;
		size_t _alignment;
	};
	
	class Mesh : public Object
	{
	public:
		friend class Chunk;
		class Chunk;
		
		class __ChunkFriend
		{
		protected:
			size_t GetStride() const
			{
				return _chunk->_stride;
			}
			
			void MarkDirty()
			{
				_chunk->_dirty = true;
			}
			
			Chunk *_chunk;
		};
		
		template<class T>
		class ElementIterator : public __ChunkFriend
		{
		public:
			friend class Chunk;
			
			ElementIterator(const ElementIterator& other) :
				_feature(other._feature),
				_ptr(other._ptr)
			{
				_chunk = other._chunk;
			}
			
			
			T *operator ->()
			{
				MarkDirty();
				return _ptr;
			}
			
			const T *operator ->() const
			{
				return _ptr;
			}
			
			T& operator *()
			{
				__ChunkFriend::MarkDirty();
				return *_ptr;
			}
			
			const T& operator *() const
			{
				return *_ptr;
			}
			
			
			ElementIterator<T>& operator ++()
			{
				Advance();
				return *this;
			}
			
			ElementIterator<T> operator ++(int)
			{
				ElementIterator<T> result(*this);
				Advance();
				
				return result;
			}
			
		private:
			ElementIterator(MeshFeature feature, Chunk *chunk, T *ptr) :
				_feature(feature),
				_ptr(ptr)
			{
				_chunk = chunk;
			}
			
			void Advance()
			{
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_ptr) + __ChunkFriend::GetStride()));
			}
			
			MeshFeature _feature;
			T *_ptr;
		};
		
		class Chunk
		{
		public:
			friend class Mesh;
			friend class __ChunkFriend;
			
			template<class T>
			ElementIterator<T> GetIterator(MeshFeature feature)
			{
				size_t offset = _mesh->GetDescriptorForFeature(feature)->offset;
				uint8 *ptr = reinterpret_cast<uint8 *>(_begin) + offset;
				
				return ElementIterator<T>(feature, this, reinterpret_cast<T *>(ptr));
			}
			
			template<class T>
			T *GetData()
			{
				_dirty = true;
				return reinterpret_cast<T *>(_begin);
			}
			
			void SetData(const void *data);
			void SetData(const void *data, MeshFeature feature);
			void SetDataInRange(const void *data, const Range& range);
			
			void CommitChanges();
			
		private:
			Chunk(Mesh *mesh, const Range& range, bool indices);
			
			Mesh *_mesh;
			Range _range;
			void *_begin;
			size_t _stride;
			bool _dirty;
			bool _indices;
		};
	
		
		RNAPI Mesh(const std::vector<MeshDescriptor>& descriptor, size_t verticesCount, size_t indicesCount);
		RNAPI Mesh(const std::vector<MeshDescriptor>& descriptor, size_t verticesCount, size_t indicesCount, const std::pair<const void *, const void *>& data);
		RNAPI ~Mesh() override;
		
		RNAPI const MeshDescriptor *GetDescriptorForFeature(MeshFeature feature) const;
	
		template<typename T>
		const T *GetVerticesData() const
		{
			return reinterpret_cast<T *>(_vertices);
		}
		
		template<typename T>
		const T *GetIndicesData() const
		{
			return reinterpret_cast<T *>(_indices);
		}
		
		RNAPI Chunk GetChunk();
		RNAPI Chunk GetChunkForRange(const Range& range);
		
		RNAPI Chunk GetIndicesChunk();
		RNAPI Chunk GetIndicesChunkForRange(const Range& range);
		
		RNAPI Chunk InsertChunk(size_t offset, size_t elements);
		RNAPI Chunk InsertIndicesChunk(size_t offset, size_t elements);
		
		RNAPI void DeleteChunk(size_t offset, size_t elements);
		RNAPI void DeleteIndicesChunk(size_t offset, size_t elements);
	
		
		RNAPI void SetMode(GLenum mode);
		RNAPI void SetVBOUsage(GLenum usage);
		RNAPI void SetIBOUsage(GLenum usage);
		
		RNAPI void SetElementData(MeshFeature feature, void *data);
		RNAPI void SetVerticesCount(size_t count);
		RNAPI void SetIndicesCount(size_t count);
		
		RNAPI void CalculateBoundingVolumes();
		
		RNAPI bool SupportsFeature(MeshFeature feature) const;
		
		RNAPI Hit IntersectsRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode = Hit::HitMode::IgnoreNone);
		
		GLuint GetVBO() { return _vbo; }
		GLuint GetIBO() { return _ibo; }
		
		size_t GetStride() const { return _stride; }
		GLenum GetMode() const { return _mode; }
		
		size_t GetVerticesCount() const { return _verticesCount; }
		size_t GetIndicesCount() const { return _indicesCount; }
		
		const AABB& GetBoundingBox() const { return _boundingBox; }
		const Sphere& GetBoundingSphere() const { return _boundingSphere; }
		
		RNAPI static Mesh *PlaneMesh(const Vector3& size = Vector3(1.0f), const Vector3& rotation = Vector3(0.0f));
		RNAPI static Mesh *CubeMesh(const Vector3& size);
		RNAPI static Mesh *CubeMesh(const Vector3& size, const Color& color);
		
	private:
		void Initialize(const std::vector<MeshDescriptor>& descriptors);
		void AllocateBuffer(const std::pair<const void *, const void *>& data);
		void PushData(bool vertices, bool indices);
		
		Hit IntersectsRay3DWithIndices(const MeshDescriptor *positionDescriptor, const MeshDescriptor *indicesDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode);
		Hit IntersectsRay2DWithIndices(const MeshDescriptor *positionDescriptor, const MeshDescriptor *indicesDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode);
		Hit IntersectsRay3DWithoutIndices(const MeshDescriptor *positionDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode);
		Hit IntersectsRay2DWithoutIndices(const MeshDescriptor *positionDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode);
		
		float RayTriangleIntersection(const Vector3 &pos, const Vector3 &dir, const Vector3 &vert1, const Vector3 &vert2, const Vector3 &vert3, Hit::HitMode mode);
		
		struct
		{
			GLuint _vbo;
			GLuint _ibo;
		};
		
		size_t _stride;
		size_t _verticesCount;
		size_t _indicesCount;
		
		GLenum _vboUsage;
		GLenum _iboUsage;
		GLenum _mode;
		
		AABB   _boundingBox;
		Sphere _boundingSphere;
		
		size_t _verticesSize;
		size_t _indicesSize;
		
		uint8 *_vertices;
		uint8 *_indices;
		
		std::vector<MeshDescriptor> _descriptors;
		std::unordered_set<MeshFeature> _features;
		
		RNDefineMeta(Mesh, Object)
	};
}

#endif /* __RAYNE_MESH_H__ */
