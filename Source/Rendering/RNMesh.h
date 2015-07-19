//
//  RNMesh.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_MESH_H_
#define __RAYNE_MESH_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "RNRendererTypes.h"
#include "RNGPUBuffer.h"

namespace RN
{
	class Mesh : public Object
	{
	public:
		struct VertexDescriptor
		{
		public:
			friend class Mesh;

			enum class Feature
			{
				Vertices,
				Normals,
				Tangents,
				Color0,
				Color1,
				UVCoords0,
				UVCoords1,
				Indices,

				Custom
			};


			VertexDescriptor(Feature feature, PrimitiveType type) :
				_feature(feature),
				_name(nullptr),
				_type(type)
			{}
			VertexDescriptor(const String *name, PrimitiveType type) :
				_feature(Feature::Custom),
				_name(name->Copy()),
				_type(type)
			{}
			~VertexDescriptor()
			{
				SafeRelease(_name);
			}

			PrimitiveType GetType() const { return _type; }
			Feature GetFeature() const { return _feature; }
			const String *GetName() const { return _name; }

			size_t GetOffset() const { return _offset; }
			size_t GetSize() const { return _size; }

		private:

			PrimitiveType _type;

			Feature _feature;
			String *_name;

			size_t _offset;
			size_t _size;
		};

		class Chunk;
		friend class Chunk;

		class __ChunkFriend
		{
		protected:
			size_t GetStride() const
			{
				return (_feature == VertexDescriptor::Feature::Indices) ? 0 : _chunk->_mesh->GetStride();
			}

			void MarkDirty()
			{
				_feature == VertexDescriptor::Feature::Indices ? (_chunk->_dirtyIndices = true) : (_chunk->_dirtyVertices = true);
			}

			size_t TranslateIndex(size_t index)
			{
				return _chunk->TranslateIndex(index);
			}

			VertexDescriptor::Feature _feature;
			Chunk *_chunk;
		};

		template<class T>
		class ElementIterator : public __ChunkFriend
		{
		public:
			friend class Chunk;

			ElementIterator(const ElementIterator &other) :
				_ptr(other._ptr),
				_base(other._base)
			{
				_feature = other._feature;
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


			ElementIterator<T> &operator ++()
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


			ElementIterator<T> &operator --()
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
			ElementIterator(VertexDescriptor::Feature feature, Chunk *chunk, T *ptr, size_t index) :
				_ptr(ptr),
				_base(ptr),
				_index(index)
			{
				_chunk = chunk;
				_feature = feature;
			}

			void Advance(size_t count)
			{
				_index += count;
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_base) + (__ChunkFriend::GetStride() * __ChunkFriend::TranslateIndex(_index))));
			}
			void Decrease(size_t count)
			{
				_index -= count;
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_base) + (__ChunkFriend::GetStride() * __ChunkFriend::TranslateIndex(_index))));
			}

			T *_ptr;
			T *_base;
			size_t _index;
		};

		class Chunk
		{
		public:
			friend class Mesh;
			friend class __ChunkFriend;

			RNAPI ~Chunk();

			template<class T>
			ElementIterator<T> GetIterator(VertexDescriptor::Feature feature)
			{
				size_t offset = _mesh->GetDescriptor(feature)->GetOffset();
				uint8 *ptr = reinterpret_cast<uint8 *>(feature == VertexDescriptor::Feature::Indices ? GetIndexData() : GetVertexData()) + offset;

				return ElementIterator<T>(feature, this, reinterpret_cast<T *>(ptr), 0);
			}

			template<class T>
			ElementIterator<T> GetIteratorAtIndex(VertexDescriptor::Feature feature, size_t index)
			{
				size_t offset = _mesh->GetDescriptor(feature)->GetOffset();
				uint8 *ptr = reinterpret_cast<uint8 *>(feature == VertexDescriptor::Feature::Indices ? GetIndexData() : GetVertexData()) + offset;

				ElementIterator<T> result(feature, this, reinterpret_cast<T *>(ptr), index);
				return result;
			}

			RNAPI void CommitChanges();

		private:
			Chunk(Mesh *mesh, bool triangles);

			size_t TranslateIndex(size_t index)
			{
				if(!_triangles || !_indicesDescriptor)
					return index;

				switch(_indicesDescriptor->GetType())
				{
					case PrimitiveType::Uint8:
						return static_cast<uint8 *>(_indexData)[index];
					case PrimitiveType::Uint16:
						return static_cast<uint16 *>(_indexData)[index];
					case PrimitiveType::Uint32:
						return static_cast<uint32 *>(_indexData)[index];

					default:
						throw InconsistencyException("Invalid state while translating triangle index!");
				}
			}

			void *GetVertexData()
			{
				if(!_vertexData)
					_vertexData = _mesh->_vertexBuffer->GetBuffer();

				return _vertexData;
			}
			void *GetIndexData()
			{
				if(!_indexData)
					_indexData = _mesh->_indicesBuffer->GetBuffer();

				return _indexData;
			}

			const VertexDescriptor *_indicesDescriptor;
			void *_vertexData;
			void *_indexData;

			Mesh *_mesh;
			bool _triangles;

			bool _dirtyVertices;
			bool _dirtyIndices;
		};


		RNAPI Mesh(const std::initializer_list<VertexDescriptor> &descriptors, size_t verticesCount, size_t indicesCount);
		RNAPI ~Mesh() override;

		RNAPI static Mesh *WithCubeMesh(const Vector3 &size, const Color &color);

		RNAPI void SetDrawMode(DrawMode mode);
		RNAPI void SetElementData(VertexDescriptor::Feature feature, const void *data);
		RNAPI void SetElementData(const String *name, const void *data);

		RNAPI const VertexDescriptor *GetDescriptor(VertexDescriptor::Feature feature) const;
		RNAPI const VertexDescriptor *GetDescriptor(const String *name) const;

		Chunk GetChunk() { return Chunk(this, false); }
		Chunk GetTrianglesChunk() { return Chunk(this, true); }

		size_t GetStride() const { return _stride; }
		size_t GetVerticesCount() const { return _verticesCount; }
		size_t GetIndicesCount() const { return _indicesCount; }

	private:
		void PerformDescriptorSetup();

		GPUBuffer *_vertexBuffer;
		GPUBuffer *_indicesBuffer;

		size_t _stride;
		size_t _verticesCount;
		size_t _indicesCount;

		size_t _verticesSize;
		size_t _indicesSize;

		DrawMode  _drawMode;

		std::vector<VertexDescriptor> _vertexDescriptors;

		RNDeclareMeta(Mesh)
	};
}


#endif /* __RAYNE_MESH_H_ */
