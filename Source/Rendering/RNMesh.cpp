//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Assets/RNAssetManager.h"
#include "RNMesh.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(Mesh, Asset)

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
		Mesh(std::vector<VertexAttribute>(attributes), verticesCount, indicesCount)
	{}

	Mesh::Mesh(const std::vector<VertexAttribute> &attributes, size_t verticesCount, size_t indicesCount) :
		_vertexBuffer(nullptr),
		_indicesBuffer(nullptr),
		_vertexBufferCPU(nullptr),
		_indicesBufferCPU(nullptr),
		_verticesCount(verticesCount),
		_indicesCount(indicesCount),
		_drawMode(DrawMode::Triangle),
		_vertexAttributes(attributes),
		_descriptor(attributes),
		_changeCounter(0)
	{
		_boundingBox = AABB(Vector3(0.0f), Vector3(0.0f));
		_boundingSphere = Sphere(_boundingBox);

		ParseAttributes();
	}

	Mesh::~Mesh()
	{
		SafeRelease(_vertexBuffer);
		SafeRelease(_indicesBuffer);
		
		if(_vertexBufferCPU)
			free(_vertexBufferCPU);
		if(_indicesBufferCPU)
			free(_indicesBufferCPU);
	}

	void Mesh::ParseAttributes()
	{
		bool hasIndices = false;

		size_t offset = 0;
		size_t initialAlignment = kRNNotFound;

		Renderer *renderer = Renderer::GetActiveRenderer();

		for(VertexAttribute &attribute : _vertexAttributes)
		{
			attribute._size = renderer->GetSizeForType(attribute._type);
			attribute._typeSize = __PrimitiveTypeTable[static_cast<size_t>(attribute._type)].size;

			switch(attribute._feature)
			{
				case VertexAttribute::Feature::Indices:
				{
					hasIndices = true;
					attribute._offset = 0;

					_indicesSize = attribute._size * _indicesCount;
					break;
				}

				default:
				{
					size_t alignment = offset % renderer->GetAlignmentForType(attribute._type);

					if(initialAlignment == kRNNotFound)
						initialAlignment = renderer->GetAlignmentForType(attribute._type);

					attribute._offset = offset + alignment;
					offset += attribute._size + alignment;

					break;
				}
			}
		}

		_stride = offset + (offset % initialAlignment);
		_verticesSize = _stride * _verticesCount;

		// Sanity checks before doing anything with the data
		if(hasIndices && _indicesSize == 0)
			throw InconsistencyException("Mesh created with indices, but zero indices count");
		if(_verticesSize == 0)
			throw InconsistencyException("Mesh created with a zero vertex size");
		if(! hasIndices && _indicesCount > 0)
			throw InconsistencyException("Mesh created without indices descriptor and non-zero indices count");

		_vertexBuffer = renderer->CreateBufferWithLength(_verticesSize, GPUResource::UsageOptions::Vertex, GPUResource::AccessOptions::ReadWrite);
		_vertexBufferCPU = malloc(_verticesSize);

		if(hasIndices)
		{
			_indicesBuffer = renderer->CreateBufferWithLength(_indicesSize, GPUResource::UsageOptions::Index, GPUResource::AccessOptions::ReadWrite);
			_indicesBufferCPU = malloc(_indicesSize);
		}
	}

	void Mesh::BeginChanges()
	{
		if((_changeCounter ++) == 0)
		{
			_changedVertices = false;
			_changedIndices = false;
		}
	}

	void Mesh::EndChanges()
	{
		if((-- _changeCounter) == 0)
		{
			if(_changedIndices)
				SubmitIndices(Range(0, _indicesSize));

			if(_changedVertices)
				SubmitVertices(Range(0, _verticesSize));
		}
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
			uint8 *destination = static_cast<uint8 *>(_indicesBufferCPU);

			std::copy(data, data + _indicesSize, destination);

			if(_changeCounter)
				_changedIndices = true;
			else
				SubmitIndices(Range(0, _indicesSize));
		}
		else
		{
			for(auto &attribute : _vertexAttributes)
			{
				if(attribute._feature == feature)
				{
					uint8 *vertices = static_cast<uint8 *>(_vertexBufferCPU);
					const uint8 *data = static_cast<const uint8 *>(tdata);

					uint8 *buffer = vertices + attribute._offset;

					for(size_t i = 0; i < _verticesCount; i ++)
					{
						std::copy(data, data + attribute._typeSize, buffer);

						buffer += _stride;
						data += attribute._typeSize;
					}

					if(_changeCounter)
						_changedVertices = true;
					else
						SubmitVertices(Range(0, _verticesSize));

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
				uint8 *vertices = static_cast<uint8 *>(_vertexBufferCPU);
				const uint8 *data = static_cast<const uint8 *>(tdata);

				uint8 *buffer = vertices + attribute._offset;

				for(size_t i = 0; i < _verticesCount; i ++)
				{
					std::copy(data, data + attribute._typeSize, buffer);

					buffer += _stride;
					data += attribute._typeSize;
				}

				if(_changeCounter)
					_changedVertices = true;
				else
					SubmitVertices(Range(0, _verticesSize));

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

	void Mesh::CalculateBoundingVolumes()
	{
		Vector3 min = Vector3();
		Vector3 max = Vector3();

		const VertexAttribute *attribute = GetAttribute(VertexAttribute::Feature::Vertices);

		if(!attribute)
		{
			_boundingBox = AABB(Vector3(0.0f), Vector3(0.0f));
			_boundingSphere = Sphere(_boundingBox);

			return;
		}

		Chunk chunk = GetChunk();

		switch(attribute->GetType())
		{
			case PrimitiveType::Vector2:
			{
				ElementIterator<Vector2> iterator = chunk.GetIterator<Vector2>(VertexAttribute::Feature::Vertices);

				for(size_t i = 0; i < _verticesCount; i ++)
				{
					const Vector2 &vertex = *(iterator++);

					min.x = std::min(vertex.x, min.x);
					min.y = std::min(vertex.y, min.y);

					max.x = std::max(vertex.x, max.x);
					max.y = std::max(vertex.y, max.y);
				}

				break;
			}

			case PrimitiveType::Vector3:
			{
				ElementIterator<Vector3> iterator = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Vertices);

				for(size_t i = 0; i < _verticesCount; i ++)
				{
					const Vector3 &vertex = *(iterator++);

					min.x = std::min(vertex.x, min.x);
					min.y = std::min(vertex.y, min.y);

					max.x = std::max(vertex.x, max.x);
					max.y = std::max(vertex.y, max.y);

					min.z = std::min(vertex.z, min.z);
					max.z = std::max(vertex.z, max.z);
				}

				break;
			}

			default:
				_boundingBox = AABB(Vector3(0.0f), Vector3(0.0f));
				_boundingSphere = Sphere(_boundingBox);
				break;
		}

		_boundingBox = AABB(min, max);
		_boundingSphere = Sphere(_boundingBox);
	}

	// ---------------------
	// MARK: -
	// MARK: Chunk
	// ---------------------

	Mesh::Chunk::Chunk(Mesh *mesh, bool triangles) :
		_indicesDescriptor(nullptr),
		_vertexData(nullptr),
		_indexData(nullptr),
		_mesh(mesh),
		_triangles(triangles)
	{
		if(triangles)
		{
			_indicesDescriptor = _mesh->GetAttribute(VertexAttribute::Feature::Indices);
			GetIndexData();
		}
	}

	Mesh::Chunk::~Chunk()
	{

	}

	// ---------------------
	// MARK: -
	// MARK: Convenient constructors
	// ---------------------

	Mesh *Mesh::WithName(const String *name)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Mesh>(name, nullptr);
	}

	Mesh *Mesh::WithColoredCube(const Vector3 &size, const Color &color)
	{
		Mesh *mesh = new Mesh({VertexAttribute(VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
							   VertexAttribute(VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
							   VertexAttribute(VertexAttribute::Feature::Color0, PrimitiveType::Color),
							   VertexAttribute(VertexAttribute::Feature::Indices, PrimitiveType::Uint16)}, 24, 36);

		mesh->BeginChanges();
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

		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		mesh->_changedVertices = true;
		mesh->_changedIndices = true;
		mesh->EndChanges();

		return mesh->Autorelease();
	}

	Mesh *Mesh::WithTexturedCube(const Vector3 &size)
	{
		Mesh *mesh = new Mesh({VertexAttribute(VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
							   VertexAttribute(VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
							   VertexAttribute(VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2),
							   VertexAttribute(VertexAttribute::Feature::Indices, PrimitiveType::Uint16)}, 24, 36);

		mesh->BeginChanges();
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

		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(VertexAttribute::Feature::UVCoords0);

		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);

		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);

		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);

		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);

		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);

		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);

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

		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		mesh->_changedVertices = true;
		mesh->_changedIndices = true;
		mesh->EndChanges();

		return mesh->Autorelease();
	}


	Mesh *Mesh::WithTexturedPlane(const Quaternion &rotation, const Vector3 &position, const Vector2 &size)
	{
		Mesh *mesh = new Mesh({ VertexAttribute(VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
			VertexAttribute(VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
			VertexAttribute(VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2),
			VertexAttribute(VertexAttribute::Feature::Indices, PrimitiveType::Uint16) }, 4, 6);

		mesh->BeginChanges();
		Chunk chunk = mesh->GetChunk();

		ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Vertices);

		*vertices++ = position + rotation.GetRotatedVector(Vector3(-size.x, 0.0f, size.y));
		*vertices++ = position + rotation.GetRotatedVector(Vector3(size.x, 0.0f, size.y));
		*vertices++ = position + rotation.GetRotatedVector(Vector3(size.x, 0.0f, -size.y));
		*vertices++ = position + rotation.GetRotatedVector(Vector3(-size.x, 0.0f, -size.y));

		ElementIterator<Vector3> normals = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Normals);

		*normals++ = Vector3(0.0f, 1.0f, 0.0f);
		*normals++ = Vector3(0.0f, 1.0f, 0.0f);
		*normals++ = Vector3(0.0f, 1.0f, 0.0f);
		*normals++ = Vector3(0.0f, 1.0f, 0.0f);

		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(VertexAttribute::Feature::UVCoords0);

		*texcoords++ = Vector2(0.0f, 1.0f);
		*texcoords++ = Vector2(1.0f, 1.0f);
		*texcoords++ = Vector2(1.0f, 0.0f);
		*texcoords++ = Vector2(0.0f, 0.0f);

		ElementIterator<uint16> indices = chunk.GetIterator<uint16>(VertexAttribute::Feature::Indices);

		*indices++ = 0;
		*indices++ = 1;
		*indices++ = 3;
		*indices++ = 2;
		*indices++ = 3;
		*indices++ = 1;

		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		mesh->_changedVertices = true;
		mesh->_changedIndices = true;
		mesh->EndChanges();

		return mesh->Autorelease();
	}

	Mesh *Mesh::WithTexturedDome(float radius, size_t slices, size_t rings)
	{
		if(rings % 2 == 0)
		{
			rings += 1;
		}
		slices += 1;
		Mesh *mesh = new Mesh({ VertexAttribute(VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
			VertexAttribute(VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
			VertexAttribute(VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2),
			VertexAttribute(VertexAttribute::Feature::Indices, PrimitiveType::Uint16) }, rings*slices, (rings - 1)*(slices - 1) * 6);

		mesh->BeginChanges();
		Chunk chunk = mesh->GetChunk();

		ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Vertices);
		ElementIterator<Vector3> normals = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Normals);
		ElementIterator<Vector2> uvs = chunk.GetIterator<Vector2>(VertexAttribute::Feature::UVCoords0);
		ElementIterator<uint16> indices = chunk.GetIterator<uint16>(VertexAttribute::Feature::Indices);

		for(size_t ring = 0; ring < rings; ring++)
		{
			for(size_t slice = 0; slice < slices; slice++)
			{
				//float y = 2.0f * segment / segments - 1.0f;
				float y = -cos(k::Pi * ring / (rings - 1));
				float r = sqrt(1.0f - y*y);
				float x = r * sin(2.0f * k::Pi * slice / (slices - 1));
				float z = r * cos(2.0f * k::Pi * slice / (slices - 1));

				Vector3 position(x, y, z);
				*vertices++ = position * radius;
				*normals++ = position.GetNormalized();
				*uvs++ = Vector2(static_cast<float>(slice) / static_cast<float>(slices - 1), 1.0f-fabs(position.y));
			}
		}

		for(size_t i = 0; i < rings - 1; i++)
		{
			for(size_t j = 0; j < slices-1; j++)
			{
				*indices++ = i * slices + j;
				*indices++ = (i + 1) * slices + j + 1;
				*indices++ = i * slices + j + 1;

				*indices++ = i * slices + j;
				*indices++ = (i + 1) * slices + j;
				*indices++ = (i + 1) * slices + j + 1;
			}
		}

		//This would add empty faces for the douplicated vertical vertices, needs (slices) NOT (slices - 1) in the index count calculation
/*		for (size_t i = 0; i < rings - 1; i++)
		{
			*indices++ = i * slices + slices - 1;
			*indices++ = (i + 1) * slices + 0;
			*indices++ = i * slices + 0;

			*indices++ = i * slices + slices - 1;
			*indices++ = (i + 1) * slices + slices - 1;
			*indices++ = (i + 1) * slices + 0;
		}*/

		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		mesh->_changedVertices = true;
		mesh->_changedIndices = true;
		mesh->EndChanges();

		return mesh->Autorelease();
	}


	Mesh *Mesh::WithSphereMesh(float radius, size_t slices, size_t rings)
	{
		Mesh *mesh = new Mesh({ VertexAttribute(VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
			VertexAttribute(VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
			VertexAttribute(VertexAttribute::Feature::Color0, PrimitiveType::Vector4),
			VertexAttribute(VertexAttribute::Feature::Indices, PrimitiveType::Uint16) }, rings*slices, (rings - 1)*(slices) * 6);

		mesh->BeginChanges();
		Chunk chunk = mesh->GetChunk();

		ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Vertices);
		ElementIterator<Vector3> normals = chunk.GetIterator<Vector3>(VertexAttribute::Feature::Normals);
		ElementIterator<Vector4> colors = chunk.GetIterator<Vector4>(VertexAttribute::Feature::Color0);
		ElementIterator<uint16> indices = chunk.GetIterator<uint16>(VertexAttribute::Feature::Indices);

		for(size_t ring = 0; ring < rings; ring++)
		{
			for(size_t slice = 0; slice < slices; slice++)
			{
				//float y = 2.0f * segment / segments - 1.0f;
				float y = -cos(k::Pi * ring / (rings - 1));
				float r = sqrt(1.0f - y*y);
				float x = r * sin(2.0f * k::Pi * slice / (slices));
				float z = r * cos(2.0f * k::Pi * slice / (slices));

				Vector3 position(x, y, z);
				*vertices++ = position * radius;
				*normals++ = position.GetNormalized();
				*colors++ = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		for(size_t i = 0; i < rings - 1; i++)
		{
			for(size_t j = 0; j < slices - 1; j++)
			{
				*indices++ = i * slices + j;
				*indices++ = (i + 1) * slices + j + 1;
				*indices++ = i * slices + j + 1;

				*indices++ = i * slices + j;
				*indices++ = (i + 1) * slices + j;
				*indices++ = (i + 1) * slices + j + 1;
			}
		}

		for(size_t i = 0; i < rings - 1; i++)
		{
			*indices++ = i * slices + slices - 1;
			*indices++ = (i + 1) * slices + 0;
			*indices++ = i * slices + 0;

			*indices++ = i * slices + slices - 1;
			*indices++ = (i + 1) * slices + slices - 1;
			*indices++ = (i + 1) * slices + 0;
		}

		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		mesh->_changedVertices = true;
		mesh->_changedIndices = true;
		mesh->EndChanges();

		return mesh->Autorelease();
	}

	void Mesh::SubmitVertices(const Range &range)
	{
		if(!_vertexBufferCPU)
			return;

		//TODO: Don't copy full range if not needed.
		void *target = _vertexBuffer->GetBuffer();
		memcpy(target, _vertexBufferCPU, _verticesSize);
		_vertexBuffer->InvalidateRange(range);
	}

	void Mesh::SubmitIndices(const Range &range)
	{
		if(!_indicesBufferCPU)
			return;

		//TODO: Don't copy full range if not needed.
		void *target = _indicesBuffer->GetBuffer();
		memcpy(target, _indicesBufferCPU, _indicesSize);
		_indicesBuffer->InvalidateRange(range);
	}
}
