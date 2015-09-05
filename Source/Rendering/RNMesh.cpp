//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(Mesh, Object)

	struct PrimitiveTypeEntry
	{
		size_t size;
		size_t alignment;
	};

	RN_CONSTEXPR PrimitiveTypeEntry __PrimitiveTypeTable[] = {

		{sizeof(uint8),      alignof(uint8)},
		{sizeof(uint16),     alignof(uint16)},
		{sizeof(uint32),     alignof(uint32)},

		{sizeof(int8),       alignof(int8)},
		{sizeof(int16),      alignof(int16)},
		{sizeof(int32),      alignof(int32)},

		{sizeof(float),      alignof(float)},

		{sizeof(Vector2),    alignof(Vector2)},
		{sizeof(Vector3),    alignof(Vector3)},
		{sizeof(Vector4),    alignof(Vector4)},

		{sizeof(Matrix),     alignof(Matrix)},
		{sizeof(Quaternion), alignof(Quaternion)},
		{sizeof(Color),      alignof(Color)}

	};

	Mesh::Mesh(const std::initializer_list<VertexAttribute> &attributes, size_t verticesCount, size_t indicesCount) :
		_vertexAttributes(attributes),
		_vertexBuffer(nullptr),
		_indicesBuffer(nullptr),
		_verticesCount(verticesCount),
		_indicesCount(indicesCount),
		_drawMode(DrawMode::Triangle),
		_descriptor(attributes)
	{
		ParseAttributes();
	}

	Mesh::~Mesh()
	{
		SafeRelease(_vertexBuffer);
		SafeRelease(_indicesBuffer);
	}

	void Mesh::ParseAttributes()
	{
		bool hasIndices = false;

		size_t offset = 0;

		for(VertexAttribute &attribute : _vertexAttributes)
		{
			const PrimitiveTypeEntry &entry = __PrimitiveTypeTable[static_cast<size_t>(attribute._type)];

			attribute._size = entry.size;

			switch(attribute._feature)
			{
				case VertexAttribute::Feature::Indices:
				{
					hasIndices = true;
					attribute._offset = 0;

					_indicesSize = entry.size * _indicesCount;
					break;
				}

				default:
				{
					offset += offset % entry.alignment;

					attribute._offset = offset;

					offset += entry.size;

					break;
				}
			}
		}

		_stride = offset;
		_verticesSize = _stride * _verticesCount;

		// Sanity checks before doing anything with the data
		if(hasIndices && _indicesSize == 0)
			throw InconsistencyException("Mesh created with indices, but zero indices count");
		if(_verticesSize == 0)
			throw InconsistencyException("Mesh created with a zero vertex size");
		if(! hasIndices && _indicesCount > 0)
			throw InconsistencyException("Mesh created without indices descriptor and non-zero indices count");

		_vertexBuffer = Renderer::GetActiveRenderer()->CreateBufferWithLength(_verticesSize,
																			  GPUResource::UsageOptions::ReadWrite);

		if(hasIndices)
			_indicesBuffer = Renderer::GetActiveRenderer()->CreateBufferWithLength(_indicesSize,
																				   GPUResource::UsageOptions::ReadWrite);
	}

	void Mesh::SetDrawMode(DrawMode mode)
	{
		_drawMode = mode;
	}

	void Mesh::SetElementData(VertexAttribute::Feature feature, const void *tdata)
	{
		if(feature == VertexAttribute::Feature::Indices)
		{
			const uint8 *data = static_cast<const uint8 *>(tdata);
			uint8 *destination = static_cast<uint8 *>(_indicesBuffer->GetBuffer());

			std::copy(data, data + _indicesSize, destination);
			_indicesBuffer->InvalidateRange(Range(0, _indicesSize));
		}
		else
		{
			for(auto &attribute : _vertexAttributes)
			{
				if(attribute._feature == feature)
				{
					uint8 *vertices = static_cast<uint8 *>(_vertexBuffer->GetBuffer());
					const uint8 *data = static_cast<const uint8 *>(tdata);

					uint8 *buffer = vertices + attribute._offset;

					for(size_t i = 0; i < _verticesCount; i ++)
					{
						std::copy(data, data + attribute._size, buffer);

						buffer += _stride;
						data += attribute._size;
					}

					_vertexBuffer->InvalidateRange(Range(0, _verticesSize));
					break;
				}
			}
		}
	}

	void Mesh::SetElementData(const String *name, const void *tdata)
	{
		for(auto &attribute : _vertexAttributes)
		{
			if(attribute._name && attribute._name->IsEqual(name))
			{
				uint8 *vertices = static_cast<uint8 *>(_vertexBuffer->GetBuffer());
				const uint8 *data = static_cast<const uint8 *>(tdata);

				uint8 *buffer = vertices + attribute._offset;

				for(size_t i = 0; i < _verticesCount; i ++)
				{
					std::copy(data, data + attribute._size, buffer);

					buffer += _stride;
					data += attribute._size;
				}

				_vertexBuffer->InvalidateRange(Range(0, _verticesSize));
				break;
			}
		}
	}

	const Mesh::VertexAttribute *Mesh::GetAttribute(VertexAttribute::Feature feature) const
	{
		for(auto &attribute : _vertexAttributes)
		{
			if(attribute._feature == feature)
				return &attribute;
		}

		return nullptr;
	}

	const Mesh::VertexAttribute *Mesh::GetAttribute(const String *name) const
	{
		for(auto &attribute : _vertexAttributes)
		{
			if(attribute._name && attribute._name->IsEqual(name))
				return &attribute;
		}

		return nullptr;
	}

	// ---------------------
	// MARK: -
	// MARK: Chunk
	// ---------------------

	Mesh::Chunk::Chunk(Mesh *mesh, bool triangles) :
		_mesh(mesh),
		_triangles(triangles),
		_dirtyVertices(false),
		_dirtyIndices(false),
		_vertexData(nullptr),
		_indexData(nullptr),
		_indicesDescriptor(nullptr)
	{
		if(triangles)
		{
			_indicesDescriptor = _mesh->GetAttribute(VertexAttribute::Feature::Indices);
			GetIndexData();
		}
	}

	Mesh::Chunk::~Chunk()
	{
		CommitChanges();
	}

	void Mesh::Chunk::CommitChanges()
	{
		if(_dirtyVertices)
		{
			_mesh->_vertexBuffer->InvalidateRange(Range(0, _mesh->_verticesSize));
			_dirtyVertices = false;
		}

		if(_dirtyIndices)
		{
			_mesh->_indicesBuffer->InvalidateRange(Range(0, _mesh->_indicesSize));
			_dirtyIndices = false;
		}
	}

	// ---------------------
	// MARK: -
	// MARK: Convenient constructors
	// ---------------------

	Mesh *Mesh::WithCubeMesh(const Vector3 &size, const Color &color)
	{
		Mesh *mesh = new Mesh({VertexAttribute(VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
							   VertexAttribute(VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
							   VertexAttribute(VertexAttribute::Feature::Color0, PrimitiveType::Color),
							   VertexAttribute(VertexAttribute::Feature::Indices, PrimitiveType::Uint16)}, 24, 36);

		Chunk chunk = mesh->GetChunk();

		ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Vertices);

		*vertices ++ = Vector3(- size.x, size.y, size.z);
		*vertices ++ = Vector3(size.x, size.y, size.z);
		*vertices ++ = Vector3(size.x, - size.y, size.z);
		*vertices ++ = Vector3(- size.x, - size.y, size.z);

		*vertices ++ = Vector3(- size.x, size.y, - size.z);
		*vertices ++ = Vector3(size.x, size.y, - size.z);
		*vertices ++ = Vector3(size.x, - size.y, - size.z);
		*vertices ++ = Vector3(- size.x, - size.y, - size.z);

		*vertices ++ = Vector3(- size.x, size.y, - size.z);
		*vertices ++ = Vector3(- size.x, size.y, size.z);
		*vertices ++ = Vector3(- size.x, - size.y, size.z);
		*vertices ++ = Vector3(- size.x, - size.y, - size.z);

		*vertices ++ = Vector3(size.x, size.y, - size.z);
		*vertices ++ = Vector3(size.x, size.y, size.z);
		*vertices ++ = Vector3(size.x, - size.y, size.z);
		*vertices ++ = Vector3(size.x, - size.y, - size.z);

		*vertices ++ = Vector3(- size.x, size.y, size.z);
		*vertices ++ = Vector3(size.x, size.y, size.z);
		*vertices ++ = Vector3(size.x, size.y, - size.z);
		*vertices ++ = Vector3(- size.x, size.y, - size.z);

		*vertices ++ = Vector3(- size.x, - size.y, - size.z);
		*vertices ++ = Vector3(size.x, - size.y, - size.z);
		*vertices ++ = Vector3(size.x, - size.y, size.z);
		*vertices ++ = Vector3(- size.x, - size.y, size.z);

		ElementIterator<Vector3> normals = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Normals);

		*normals ++ = Vector3(0.0f, 0.0f, 1.0f);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0f);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0f);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0f);

		*normals ++ = Vector3(0.0f, 0.0f, -1.0f);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0f);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0f);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0f);

		*normals ++ = Vector3(-1.0f, 0.0f, 0.0f);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0f);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0f);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0f);

		*normals ++ = Vector3(1.0f, 0.0f, 0.0f);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0f);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0f);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0f);

		*normals ++ = Vector3(0.0f, 1.0f, 0.0f);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0f);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0f);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0f);

		*normals ++ = Vector3(0.0f, -1.0f, 0.0f);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0f);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0f);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0f);

		ElementIterator<Color> colors = chunk.GetIterator<Color>(VertexAttribute::Feature::Color0);

		for(size_t i = 0; i < 24; i ++)
			*colors ++ = color;

		ElementIterator<uint16> indices = chunk.GetIterator<uint16>(VertexAttribute::Feature::Indices);

		*indices ++ = 0;
		*indices ++ = 3;
		*indices ++ = 1;
		*indices ++ = 2;
		*indices ++ = 1;
		*indices ++ = 3;

		*indices ++ = 5;
		*indices ++ = 7;
		*indices ++ = 4;
		*indices ++ = 7;
		*indices ++ = 5;
		*indices ++ = 6;

		*indices ++ = 8;
		*indices ++ = 11;
		*indices ++ = 9;
		*indices ++ = 10;
		*indices ++ = 9;
		*indices ++ = 11;

		*indices ++ = 13;
		*indices ++ = 15;
		*indices ++ = 12;
		*indices ++ = 15;
		*indices ++ = 13;
		*indices ++ = 14;

		*indices ++ = 17;
		*indices ++ = 19;
		*indices ++ = 16;
		*indices ++ = 19;
		*indices ++ = 17;
		*indices ++ = 18;

		*indices ++ = 21;
		*indices ++ = 23;
		*indices ++ = 20;
		*indices ++ = 23;
		*indices ++ = 21;
		*indices ++ = 22;

		return mesh->Autorelease();
	}
}
