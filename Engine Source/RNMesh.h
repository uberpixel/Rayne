//
//  RNMesh.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
#include "RNEnum.h"

namespace RN
{
	struct MeshFeature : Enum<uint32>
	{
		MeshFeature()
		{}
		
		MeshFeature(int value) :
			Enum(value)
		{}
		
		enum
		{
			Vertices,
			Normals,
			Tangents,
			Color0,
			Color1,
			UVSet0,
			UVSet1,
			BoneWeights,
			BoneIndices,
			Indices,
			
			Custom = Indices + 1
		};
	};
	
	enum DescriptorFlags
	{
		DescriptorFlagSIMDAlignment
	};
	
	struct MeshDescriptor
	{
	friend class Mesh;
	public:
		RNAPI MeshDescriptor(MeshFeature feature, uint32 flags=0);
		
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
			
			ElementIterator(const ElementIterator &other) :
				_feature(other._feature),
				_ptr(other._ptr),
				_base(other._base)
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
			
			T &operator *()
			{
				__ChunkFriend::MarkDirty();
				return *_ptr;
			}
			
			const T &operator *() const
			{
				return *_ptr;
			}
			
			
			ElementIterator<T> &Seek(size_t index)
			{
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_base) + (__ChunkFriend::GetStride() * index)));
				return *this;
			}
			
			
			ElementIterator<T> operator +(size_t value) const
			{
				ElementIterator<T> result(*this);
				result.Advance(value);
				
				return result;
			}
			ElementIterator<T> &operator +=(size_t value)
			{
				Advance(value);
				return *this;
			}
			
			ElementIterator<T> operator -(size_t value) const
			{
				ElementIterator<T> result(*this);
				result.Decrease(value);
				
				return result;
			}
			ElementIterator<T> &operator -=(size_t value)
			{
				Decrease(value);
				return *this;
			}
			
			
			ElementIterator<T>& operator ++()
			{
				Advance(1);
				return *this;
			}
			ElementIterator<T> operator ++(int)
			{
				ElementIterator<T> result(*this);
				Advance(1);
				
				return result;
			}
			
			
			ElementIterator<T>& operator --()
			{
				Decrease(1);
				return *this;
			}
			ElementIterator<T> operator --(int)
			{
				ElementIterator<T> result(*this);
				Decrease(1);
				
				return result;
			}
			
		private:
			ElementIterator(MeshFeature feature, Chunk *chunk, T *ptr) :
				_feature(feature),
				_ptr(ptr),
				_base(ptr)
			{
				_chunk = chunk;
			}
			
			void Advance(size_t count)
			{
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_ptr) + (__ChunkFriend::GetStride() * count)));
			}
			void Decrease(size_t count)
			{
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_ptr) - (__ChunkFriend::GetStride() * count)));
			}
			
			
			MeshFeature _feature;
			T *_ptr;
			T *_base;
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
			ElementIterator<T> GetIteratorAtIndex(MeshFeature feature, size_t index)
			{
				size_t offset = _mesh->GetDescriptorForFeature(feature)->offset;
				uint8 *ptr = reinterpret_cast<uint8 *>(_begin) + offset;
				
				ElementIterator<T> result(feature, this, reinterpret_cast<T *>(ptr));
				result += index;
				
				return result;
			}
			
			template<class T>
			T *GetData()
			{
				_dirty = true;
				return reinterpret_cast<T *>(_begin);
			}
			
			RNAPI void SetData(const void *data);
			RNAPI void SetData(const void *data, MeshFeature feature);
			RNAPI void SetDataInRange(const void *data, const Range &range);
			
			RNAPI void CommitChanges();
			
		private:
			Chunk(Mesh *mesh, const Range &range, bool indices);
			
			Mesh *_mesh;
			Range _range;
			void *_begin;
			size_t _stride;
			bool _dirty;
			bool _indices;
		};
		
		enum class DrawMode : GLenum
		{
			Points = GL_POINTS,
			LineStrip = GL_LINE_STRIP,
			LineLoop  = GL_LINE_LOOP,
			Lines = GL_LINES,
			TriangleStrip = GL_TRIANGLE_STRIP,
			TriangleFan = GL_TRIANGLE_FAN,
			Triangles = GL_TRIANGLES,
			LineStripAdjacency = GL_LINE_STRIP_ADJACENCY,
			LinesAdjacency = GL_LINES_ADJACENCY,
			TriangleStripAdjacency = GL_TRIANGLE_STRIP_ADJACENCY,
			TrianglesAdjacency = GL_TRIANGLES_ADJACENCY,
			Patches = GL_PATCHES
		};
		
		enum class MeshUsage : GLenum
		{
			Static = GL_STATIC_DRAW,
			Dynamic = GL_DYNAMIC_DRAW,
			Stream = GL_STREAM_DRAW
		};
		
		
		RNAPI Mesh(const std::vector<MeshDescriptor>& descriptor, size_t verticesCount, size_t indicesCount);
		RNAPI Mesh(const std::vector<MeshDescriptor>& descriptor, size_t verticesCount, size_t indicesCount, const std::pair<const void *, const void *>& data);
		RNAPI Mesh(Deserializer *deserializer);
		RNAPI ~Mesh() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
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
		RNAPI Chunk GetChunkForRange(const Range &range);
		
		RNAPI Chunk GetIndicesChunk();
		RNAPI Chunk GetIndicesChunkForRange(const Range &range);
		
		RNAPI Chunk InsertChunk(size_t offset, size_t elements);
		RNAPI Chunk InsertIndicesChunk(size_t offset, size_t elements);
		
		RNAPI void DeleteChunk(size_t offset, size_t elements);
		RNAPI void DeleteIndicesChunk(size_t offset, size_t elements);
	
		
		RNAPI void SetDrawMode(DrawMode mode);
		RNAPI void SetVBOUsage(MeshUsage usage);
		RNAPI void SetIBOUsage(MeshUsage usage);
		
		RNAPI void SetElementData(MeshFeature feature, const void *data);
		RNAPI void SetVerticesCount(size_t count);
		RNAPI void SetIndicesCount(size_t count);
		
		RNAPI void CalculateBoundingVolumes();
		RNAPI void GenerateTangents();
		
		RNAPI bool SupportsFeature(MeshFeature feature) const;
		
		RNAPI Hit IntersectsRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode = Hit::HitMode::IgnoreNone);
		
		RNAPI GLuint GetVBO() { return _vbo; }
		RNAPI GLuint GetIBO() { return _ibo; }
		
		RNAPI size_t GetStride() const { return _stride; }
		RNAPI DrawMode GetDrawMode() const { return _mode; }
		
		RNAPI size_t GetVerticesCount() const { return _verticesCount; }
		RNAPI size_t GetIndicesCount() const { return _indicesCount; }
		
		RNAPI const AABB &GetBoundingBox() const { return _boundingBox; }
		RNAPI const Sphere &GetBoundingSphere() const { return _boundingSphere; }
		
		RNAPI static Mesh *PlaneMesh(const Vector3 &size = Vector3(1.0f), const Vector3 &rotation = Vector3(0.0f));
		RNAPI static Mesh *CubeMesh(const Vector3 &size);
		RNAPI static Mesh *CubeMesh(const Vector3 &size, const Color &color);
		RNAPI static Mesh *SphereMesh(float radius, size_t slices, size_t segments);
		
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
		
		MeshUsage _vboUsage;
		MeshUsage _iboUsage;
		DrawMode  _mode;
		
		AABB   _boundingBox;
		Sphere _boundingSphere;
		
		size_t _verticesSize;
		size_t _indicesSize;
		
		uint8 *_vertices;
		uint8 *_indices;
		
		std::vector<MeshDescriptor> _descriptors;
		std::set<MeshFeature> _features;
		
		RNDeclareMeta(Mesh)
	};
	
	RNObjectClass(Mesh)
}

#endif /* __RAYNE_MESH_H__ */
