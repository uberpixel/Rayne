//
//  RNMesh.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_MESH_H_
#define __RAYNE_MESH_H_

#include "../Assets/RNAsset.h"
#include "../Base/RNBase.h"
#include "../Math/RNAABB.h"
#include "../Math/RNAlgorithm.h"
#include "../Math/RNSphere.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "RNGPUBuffer.h"
#include "RNRendererTypes.h"

namespace RN
{
	class Mesh : public Asset
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

				BoneWeights,
				BoneIndices,

				Custom
			};


			VertexAttribute(Feature feature, PrimitiveType type) :
				_type(type),
				_feature(feature),
				_name(nullptr),
				_offset(0),
				_size(0),
				_typeSize(0)
			{}
			VertexAttribute(const String *name, PrimitiveType type) :
				_type(type),
				_feature(Feature::Custom),
				_name(name->Copy()),
				_offset(0),
				_size(0),
				_typeSize(0)
			{}
			VertexAttribute(const VertexAttribute &other) :
				_type(other._type),
				_feature(other._feature),
				_name(SafeRetain(other._name)),
				_offset(other._offset),
				_size(other._size),
				_typeSize(other._typeSize)
			{}
			VertexAttribute &operator=(const VertexAttribute &other)
			{
				if(this == &other) return *this;

				SafeRelease(_name);

				_type = other._type;
				_feature = other._feature;
				_name = SafeRetain(other._name);
				_offset = other._offset;
				_size = other._size;
				_typeSize = other._typeSize;

				return *this;
			}
			~VertexAttribute()
			{
				SafeRelease(_name);
			}

			bool operator==(const VertexAttribute &other) const
			{
				return IsEqual(other);
			}
			bool operator!=(const VertexAttribute &other) const
			{
				return !IsEqual(other);
			}

			bool IsEqual(const VertexAttribute &other) const
			{
				if(_type != other._type || _feature != other._feature || _offset != other._offset || _size != other._size)
					return false;

				if(_name || other._name)
					return _name && other._name && _name->IsEqual(other._name);

				return true;
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
			size_t _typeSize;
		};

		class VertexDescriptor
		{
		public:
			VertexDescriptor() :
				_hash(0)
			{}

			VertexDescriptor(const std::vector<VertexAttribute> &attributes) :
				_attributes(attributes),
				_hash(attributes.size())
			{
				for(auto &attribute : _attributes)
				{
					HashCombine(_hash, static_cast<uint32>(attribute.GetFeature()));
					HashCombine(_hash, static_cast<uint32>(attribute.GetType()));
					HashCombine(_hash, attribute.GetOffset());
					HashCombine(_hash, attribute.GetSize());
					if(attribute.GetName()) HashCombine(_hash, attribute.GetName()->GetHash());
				}
			}

			bool operator==(const VertexDescriptor &other) const
			{
				return IsEqual(other);
			}
			bool operator!=(const VertexDescriptor &other) const
			{
				return !IsEqual(other);
			}


			size_t GetHash() const
			{
				return _hash;
			}
			const std::vector<VertexAttribute> &GetAttributes() const { return _attributes; }

			bool IsEqual(const VertexDescriptor &other) const
			{
				if(GetHash() != other.GetHash() || _attributes.size() != other._attributes.size())
					return false;

				size_t count = _attributes.size();
				for(size_t i = 0; i < count; i++)
				{
					if(_attributes[i] != other._attributes[i])
						return false;
				}

				return true;
			}

		private:
			std::vector<VertexAttribute> _attributes;
			size_t _hash;
		};


		class Chunk;
		friend class Chunk;

		class DrawSnapshot
		{
		public:
			DrawSnapshot() = default;
			RNAPI DrawSnapshot(const DrawSnapshot &snapshot);

			RNAPI DrawSnapshot &operator=(const DrawSnapshot &snapshot);

			RNAPI void Reset();

			size_t GetVerticesCount() const { return _verticesCount; }
			size_t GetIndicesCount() const { return _indicesCount; }
			size_t GetVertexPositionsSeparatedSize() const { return _vertexPositionsSeparatedSize; }
			size_t GetVertexPositionsSeparatedStride() const { return _vertexPositionsSeparatedStride; }
			size_t GetStride() const { return _stride; }
			const VertexDescriptor &GetVertexDescriptor() const { return _descriptor; }
			const std::vector<VertexAttribute> &GetVertexAttributes() const { return _descriptor.GetAttributes(); }
			size_t GetPipelineHash() const { return _pipelineHash; }
			DrawMode GetDrawMode() const { return _drawMode; }
			PrimitiveType GetIndexType() const { return _indexType; }

		private:
			friend class Mesh;

			VertexDescriptor _descriptor;
			StrongRef<GPUBuffer> _vertexBuffer;
			StrongRef<GPUBuffer> _indicesBuffer;

			size_t _vertexPositionsSeparatedSize = 0;
			size_t _vertexPositionsSeparatedStride = 0;
			size_t _stride = 0;
			size_t _verticesCount = 0;
			size_t _indicesCount = 0;
			size_t _pipelineHash = 0;

			DrawMode _drawMode = DrawMode::Triangle;
			PrimitiveType _indexType = PrimitiveType::Invalid;
		};

		class BufferSnapshot
		{
		public:
			BufferSnapshot() = default;

			void Reset()
			{
				_vertexBuffer = nullptr;
				_indicesBuffer = nullptr;
			}

			GPUBuffer *GetVertexBuffer() const { return _vertexBuffer; }
			GPUBuffer *GetIndicesBuffer() const { return _indicesBuffer; }

		private:
			friend class Mesh;

			GPUBuffer *_vertexBuffer = nullptr;
			GPUBuffer *_indicesBuffer = nullptr;
		};

		class __ChunkFriend
		{
		protected:
			size_t GetStride() const
			{
				if(_feature == VertexAttribute::Feature::Indices) return _chunk->_indicesDescriptor->GetSize();
				if(_feature == VertexAttribute::Feature::Vertices && _chunk->_mesh->GetVertexPositionsSeparatedSize() > 0) return _chunk->_mesh->GetVertexPositionsSeparatedStride();
				return _chunk->_mesh->GetStride();
			}

			size_t TranslateIndex(size_t index)
			{
				if(_feature == VertexAttribute::Feature::Indices)
					return index;

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
				_base(other._base),
				_index(other._index)
			{
				_feature = other._feature;
				_chunk = other._chunk;
			}


			T *operator->()
			{
				return _ptr;
			}

			const T *operator->() const
			{
				return _ptr;
			}

			T &operator*()
			{
				return *_ptr;
			}

			const T &operator*() const
			{
				return *_ptr;
			}


			ElementIterator<T> &Seek(size_t index)
			{
				_index = index;
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_base) + (__ChunkFriend::GetStride() * __ChunkFriend::TranslateIndex(_index))));
				return *this;
			}


			ElementIterator<T> operator+(size_t value) const
			{
				ElementIterator<T> result(*this);
				result.Advance(value);

				return result;
			}
			ElementIterator<T> &operator+=(size_t value)
			{
				Advance(value);
				return *this;
			}

			ElementIterator<T> operator-(size_t value) const
			{
				ElementIterator<T> result(*this);
				result.Decrease(value);

				return result;
			}
			ElementIterator<T> &operator-=(size_t value)
			{
				Decrease(value);
				return *this;
			}


			ElementIterator<T> &operator++()
			{
				Advance(1);
				return *this;
			}
			ElementIterator<T> operator++(int)
			{
				ElementIterator<T> result(*this);
				Advance(1);

				return result;
			}


			ElementIterator<T> &operator--()
			{
				Decrease(1);
				return *this;
			}
			ElementIterator<T> operator--(int)
			{
				ElementIterator<T> result(*this);
				Decrease(1);

				return result;
			}

		private:
			ElementIterator(VertexAttribute::Feature feature, Chunk *chunk, T *ptr, size_t index) :
				_ptr(nullptr),
				_base(ptr),
				_index(index)
			{
				_chunk = chunk;
				_feature = feature;
				_ptr = reinterpret_cast<T *>((reinterpret_cast<uint8 *>(_base) + (__ChunkFriend::GetStride() * __ChunkFriend::TranslateIndex(_index))));
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
				if(feature == VertexAttribute::Feature::Indices)
					_indicesDescriptor = _mesh->GetAttribute(VertexAttribute::Feature::Indices);
				else if(feature != VertexAttribute::Feature::Vertices)
					offset += _mesh->_vertexPositionsSeparatedSize;

				uint8 *ptr = reinterpret_cast<uint8 *>(feature == VertexAttribute::Feature::Indices ? GetIndexData() : GetVertexData()) + offset;
				return ElementIterator<T>(feature, this, reinterpret_cast<T *>(ptr), 0);
			}

			template<class T>
			ElementIterator<T> GetIteratorAtIndex(VertexAttribute::Feature feature, size_t index)
			{
				size_t offset = _mesh->GetAttribute(feature)->GetOffset();
				if(feature == VertexAttribute::Feature::Indices)
					_indicesDescriptor = _mesh->GetAttribute(VertexAttribute::Feature::Indices);
				else if(feature != VertexAttribute::Feature::Vertices)
					offset += _mesh->_vertexPositionsSeparatedSize;
				uint8 *ptr = reinterpret_cast<uint8 *>(feature == VertexAttribute::Feature::Indices ? GetIndexData() : GetVertexData()) + offset;

				ElementIterator<T> result(feature, this, reinterpret_cast<T *>(ptr), index);
				return result;
			}

		private:
			RNAPI Chunk(Mesh *mesh, bool triangles);

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
					_vertexData = _mesh->_skipCPUBuffer ? _mesh->_vertexBuffer->GetBuffer() : _mesh->_vertexBufferCPU;

				return _vertexData;
			}
			void *GetIndexData()
			{
				if(!_indexData)
					_indexData = _mesh->_skipCPUBuffer ? _mesh->_indicesBuffer->GetBuffer() : _mesh->_indicesBufferCPU;

				return _indexData;
			}

			const VertexAttribute *_indicesDescriptor;
			void *_vertexData;
			void *_indexData;

			Mesh *_mesh;
			bool _triangles;
		};


		RNAPI Mesh(const std::initializer_list<VertexAttribute> &attributes, size_t verticesCount, size_t indicesCount, bool streamed = false);
		RNAPI Mesh(const std::vector<VertexAttribute> &attributes, size_t verticesCount, size_t indicesCount, bool streamed = false);
		RNAPI ~Mesh() override;

		RNAPI static Mesh *WithName(const String *name);

		RNAPI static Mesh *WithColoredCube(const Vector3 &size, const Color &color);
		RNAPI static Mesh *WithTexturedCube(const Vector3 &size);

		RNAPI static Mesh *WithTexturedPlane(const Quaternion &rotation, const Vector3 &position = Vector3(0.0f, 0.0f, 0.0f), const Vector2 &size = Vector2(0.5f, 0.5f));
		RNAPI static Mesh *WithTexturedDome(float radius, size_t slices, size_t segments);

		RNAPI static Mesh *WithSphereMesh(float radius, size_t slices, size_t segments, Color color);
		RNAPI static Mesh *WithCylinderMesh(float radius, float height, size_t slices, Color color);

		RNAPI void BeginChanges(bool skipCPUBuffer = false);
		RNAPI void EndChanges();

		RNAPI void SetDrawMode(DrawMode mode);
		RNAPI void SetElementData(VertexAttribute::Feature feature, const void *data);
		RNAPI void SetElementData(const String *name, const void *data);

		RNAPI const VertexAttribute *GetAttribute(VertexAttribute::Feature feature) const;
		RNAPI const VertexAttribute *GetAttribute(const String *name) const;

		RNAPI void CalculateBoundingVolumes();
		uint64 GetPipelineVersion() const { return _pipelineVersion; }
		RNAPI void GetDrawSnapshot(DrawSnapshot &snapshot) const;
		// Submission-thread cache. Published snapshots remain immutable while frames use them.
		RNAPI const std::shared_ptr<const DrawSnapshot> &GetSharedDrawSnapshot() const;
		RNAPI void GetBufferSnapshot(BufferSnapshot &snapshot) const;

		//TODO: Having the two types is a bit confusing since they result in different iterator behaviour
		Chunk GetChunk() { return Chunk(this, false); }
		Chunk GetTrianglesChunk() { return Chunk(this, true); }

		const AABB &GetBoundingBox() const { return _boundingBox; }
		const Sphere &GetBoundingSphere() const { return _boundingSphere; }

		size_t GetStride() const { return _stride; }
		size_t GetVerticesCount() const { return _verticesCount; }
		size_t GetIndicesCount() const { return _indicesCount; }
		size_t GetVertexPositionsSeparatedSize() const { return _vertexPositionsSeparatedSize; }
		size_t GetVertexPositionsSeparatedStride() const { return _vertexPositionsSeparatedStride; }

		DrawMode GetDrawMode() const { return _drawMode; }

		const std::vector<VertexAttribute> &GetVertexAttributes() const { return _vertexAttributes; }
		const VertexDescriptor &GetVertexDescriptor() const { return _descriptor; }

		GPUBuffer *GetGPUVertexBuffer() const { return _vertexBuffer; }
		GPUBuffer *GetGPUIndicesBuffer() const { return _indicesBuffer; }

		void *GetCPUVertexBuffer() const { return _vertexBufferCPU; }
		void *GetCPUIndicesBuffer() const { return _indicesBufferCPU; }

		bool changedVertices;
		bool changedIndices;

	private:
		void MarkPipelineDirty()
		{
			_pipelineVersion += 1;
			_sharedDrawSnapshot.reset();
		}

		void ParseAttributes();
		void SubmitIndices(const Range &range);
		void SubmitVertices(const Range &range);

		//TODO: Find a nice way to combine cpu and gpu buffers with consistent interface, optional storage and transfer between them
		GPUBuffer *_vertexBuffer;
		GPUBuffer *_indicesBuffer;
		void *_vertexBufferCPU;
		void *_indicesBufferCPU;

		bool _isStreamed; //This makes the vertex and index buffers use multiple buffers, so that new data can be written to one while the GPU renders another, will also not be unmapped
		bool _skipCPUBuffer; //This in combination with isStreamed will make mesh updates write directly to the gpu buffer and leave the cpu buffer untouched. Makes the mesh changes unavailable to physics and such, but skips an additional copy step

		size_t _vertexPositionsSeparatedSize;
		size_t _vertexPositionsSeparatedStride;
		size_t _stride;
		size_t _verticesCount;
		size_t _indicesCount;

		size_t _verticesSize;
		size_t _indicesSize;

		DrawMode _drawMode;

		std::vector<VertexAttribute> _vertexAttributes;
		VertexDescriptor _descriptor;

		uint32 _changeCounter;

		uint64 _pipelineVersion;
		mutable std::shared_ptr<const DrawSnapshot> _sharedDrawSnapshot;

		AABB _boundingBox;
		Sphere _boundingSphere;

		__RNDeclareMetaInternal(Mesh)
	};
} // namespace RN


#endif /* __RAYNE_MESH_H_ */
