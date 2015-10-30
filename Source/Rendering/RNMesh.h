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
#include "../Math/RNAlgorithm.h"
#include "RNRendererTypes.h"
#include "RNGPUBuffer.h"

namespace RN
{
	class Mesh : public Object
	{
	public:
		struct VertexAttribute
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


			VertexAttribute(Feature feature, PrimitiveType type) :
				_feature(feature),
				_name(nullptr),
				_type(type)
			{}
			VertexAttribute(const String *name, PrimitiveType type) :
				_feature(Feature::Custom),
				_name(name->Copy()),
				_type(type)
			{}
			~VertexAttribute()
			{
				SafeRelease(_name);
			}

			bool operator ==(const VertexAttribute &other) const
			{
				return IsEqual(other);
			}
			bool operator !=(const VertexAttribute &other) const
			{
				return !IsEqual(other);
			}

			bool IsEqual(const VertexAttribute &other) const
			{
				return (_type == other._type && _feature == other._feature);
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

		class VertexDescriptor
		{
		public:
			VertexDescriptor(const std::initializer_list<VertexAttribute> &attributes) :
				_attributes(attributes),
				_names(nullptr),
				_featureSet(0)
			{
				for(auto &attribute : _attributes)
				{
					_featureSet |= (1 << static_cast<uint32>(attribute.GetFeature()));

					if(attribute.GetFeature() == VertexAttribute::Feature::Custom)
					{
						if(!_names)
							_names = new Array();

						_names->AddObject(attribute.GetName()->Copy());
					}
				}

				_hash = _featureSet;
				if(_names)
					HashCombine(_hash, _names->GetHash());
			}

			VertexDescriptor(const VertexDescriptor &other) :
				_attributes(other._attributes),
				_featureSet(other._featureSet),
				_hash(other._hash),
				_names(SafeRetain(other._names))
			{}

			VertexDescriptor &operator =(const VertexDescriptor &other)
			{
				SafeRelease(_names);

				_attributes = other._attributes;
				_featureSet = other._featureSet;
				_hash = other._hash;
				_names = SafeRetain(other._names);

				return *this;
			}

			~VertexDescriptor()
			{
				SafeRelease(_names);
			}


			bool operator== (VertexDescriptor &other) const
			{
				return IsEqual(other);
			}
			bool operator!= (VertexDescriptor &other) const
			{
				return IsEqual(other);
			}


			size_t GetHash() const
			{
				return _hash;
			}
			bool IsEqual(const VertexDescriptor &other) const
			{
				if(GetHash() != other.GetHash() || _attributes.size() != other._attributes.size())
					return false;

				size_t count = _attributes.size();
				for(size_t i = 0; i < count; i ++)
				{
					if(_attributes[i] != other._attributes[i])
						return false;
				}

				return true;
			}

		private:
			std::vector<VertexAttribute> _attributes;
			uint32 _featureSet;
			Array *_names;
			size_t _hash;
		};


		class Chunk;
		friend class Chunk;

		class __ChunkFriend
		{
		protected:
			size_t GetStride() const
			{
				return (_feature == VertexAttribute::Feature::Indices) ? 2 : _chunk->_mesh->GetStride();
			}

			void MarkDirty()
			{
				_feature == VertexAttribute::Feature::Indices ? (_chunk->_dirtyIndices = true) : (_chunk->_dirtyVertices = true);
			}

			size_t TranslateIndex(size_t index)
			{
				return _chunk->TranslateIndex(index);
			}

			VertexAttribute::Feature _feature;
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
			ElementIterator(VertexAttribute::Feature feature, Chunk *chunk, T *ptr, size_t index) :
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
			ElementIterator<T> GetIterator(VertexAttribute::Feature feature)
			{
				size_t offset = _mesh->GetAttribute(feature)->GetOffset();
				uint8 *ptr = reinterpret_cast<uint8 *>(feature == VertexAttribute::Feature::Indices ? GetIndexData() : GetVertexData()) + offset;

				return ElementIterator<T>(feature, this, reinterpret_cast<T *>(ptr), 0);
			}

			template<class T>
			ElementIterator<T> GetIteratorAtIndex(VertexAttribute::Feature feature, size_t index)
			{
				size_t offset = _mesh->GetAttribute(feature)->GetOffset();
				uint8 *ptr = reinterpret_cast<uint8 *>(feature == VertexAttribute::Feature::Indices ? GetIndexData() : GetVertexData()) + offset;

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

			const VertexAttribute *_indicesDescriptor;
			void *_vertexData;
			void *_indexData;

			Mesh *_mesh;
			bool _triangles;

			bool _dirtyVertices;
			bool _dirtyIndices;
		};


		RNAPI Mesh(const std::initializer_list<VertexAttribute> &descriptors, size_t verticesCount, size_t indicesCount);
		RNAPI ~Mesh() override;

		RNAPI static Mesh *WithColoredCube(const Vector3 &size, const Color &color);
		RNAPI static Mesh *WithTexturedCube(const Vector3 &size);

		RNAPI static Mesh *WithSphereMesh(float radius, size_t slices, size_t segments);

		RNAPI void SetDrawMode(DrawMode mode);
		RNAPI void SetElementData(VertexAttribute::Feature feature, const void *data);
		RNAPI void SetElementData(const String *name, const void *data);

		RNAPI const VertexAttribute *GetAttribute(VertexAttribute::Feature feature) const;
		RNAPI const VertexAttribute *GetAttribute(const String *name) const;

		Chunk GetChunk() { return Chunk(this, false); }
		Chunk GetTrianglesChunk() { return Chunk(this, true); }

		size_t GetStride() const { return _stride; }
		size_t GetVerticesCount() const { return _verticesCount; }
		size_t GetIndicesCount() const { return _indicesCount; }

		const std::vector<VertexAttribute> &GetVertexAttributes() const { return _vertexAttributes; }
		const VertexDescriptor &GetVertexDescriptor() const { return _descriptor; }

		GPUBuffer *GetVertexBuffer() const { return _vertexBuffer; }
		GPUBuffer *GetIndicesBuffer() const { return _indicesBuffer; }

	private:
		void ParseAttributes();

		GPUBuffer *_vertexBuffer;
		GPUBuffer *_indicesBuffer;

		size_t _stride;
		size_t _verticesCount;
		size_t _indicesCount;

		size_t _verticesSize;
		size_t _indicesSize;

		DrawMode _drawMode;

		std::vector<VertexAttribute> _vertexAttributes;
		VertexDescriptor _descriptor;

		RNDeclareMeta(Mesh)
	};
}


#endif /* __RAYNE_MESH_H_ */
