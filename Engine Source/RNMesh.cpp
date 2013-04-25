//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"
#include "RNKernel.h"
#include "RNMemory.h"

namespace RN
{
	RNDeclareMeta(Mesh)
	
	MeshDescriptor::MeshDescriptor()
	{
		elementMember = 0;
		elementSize = 0;
		elementCount = 0;
		offset = -1;
		
		_pointer = 0;
		_size = 0;
		_useCount = 0;
		_available = false;
		_dirty = false;
	}
	
	Mesh::Mesh(const Array<MeshDescriptor>& descriptor)
	{
		Initialize(descriptor);
	}
	
	Mesh::Mesh(const Array<MeshDescriptor>& descriptor, const void *data)
	{
		Initialize(descriptor);
		
		_meshData = MeshData<uint8>();
		
		const uint8 *meshData = static_cast<const uint8 *>(data);
		std::copy(meshData, meshData + _meshSize, _meshData);
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			if(_descriptor[i]._available)
				_descriptor[i]._dirty = true;
		}
	}
	
	void Mesh::Initialize(const Array<MeshDescriptor>& descriptor)
	{
		_meshSize = 0;
		_meshData = 0;
		
		_indicesSize = 0;
		_indices     = 0;
		
		_stride = 0;
		_mode   = GL_TRIANGLES;
		
		_vboUsage = GL_STATIC_DRAW;
		_iboUsage = GL_STATIC_DRAW;
		
		glGenBuffers(2, &_vbo);
		RN_CHECKOPENGL();
		
		size_t offset = 0;
		
		for(int i=0; i<descriptor.Count(); i++)
		{
			size_t size = descriptor[i].elementSize * descriptor[i].elementCount;
			int index   = (int)descriptor[i].feature;
			
			if(!_descriptor[index]._available)
			{
				_descriptor[index] = descriptor[i];
				_descriptor[index]._available = true;
				_descriptor[index]._size = size;
				
				if(_descriptor[index].feature != kMeshFeatureIndices)
				{
					_meshSize += size;
					_stride   += descriptor[i].elementSize;
					
					if(_descriptor[index].offset == -1)
					{
						_descriptor[index].offset = offset;
						offset += _descriptor[index].elementSize;
					}
				}
				else
				{
					_indicesSize += size;
				}
			}
		}
	}
	
	Mesh::~Mesh()
	{
		glDeleteBuffers(2, &_vbo);
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			delete [] _descriptor[i]._pointer;
		}
		
		if(_meshData)
			Memory::FreeSIMD(_meshData);
		
		if(_indices)
			Memory::FreeSIMD(_indices);
	}
	
	void Mesh::SetMode(GLenum mode)
	{
		_mode = mode;
	}
	
	void Mesh::SetVBOUsage(GLenum usage)
	{
		_vboUsage = usage;
	}
	
	void Mesh::SetIBOUsage(GLenum usage)
	{
		_iboUsage = usage;
	}
	
	const void *Mesh::FetchConstDataForFeature(MeshFeature feature)
	{
		int32 index = (int32)feature;
		if(!_descriptor[index]._available)
			return 0;
		
		if(_descriptor[index]._pointer)
			return _descriptor[index]._pointer;
		
		uint8 *data = static_cast<uint8 *>(Memory::AllocateSIMD(_descriptor[index]._size));
		
		if(feature == kMeshFeatureIndices)
		{
			uint8 *indicesData = static_cast<uint8 *>(_indices);
			
			if(indicesData)
				std::copy(indicesData, indicesData + _indicesSize, data);
		}
		else
		{
			uint8 *meshData = static_cast<uint8 *>(_meshData);
			
			if(meshData)
			{
				size_t offset = OffsetForFeature(feature);
				size_t size   = _descriptor[feature].elementSize;
				
				uint8 *tdata = data;
				
				while(offset < _meshSize)
				{
					std::copy(meshData + offset, meshData + (offset + size), tdata);
					
					offset += _stride;
					tdata  += size;
				}
			}
		}
		
		_descriptor[index]._pointer = data;
		_descriptor[index]._dirty = true;
		_descriptor[index]._useCount ++;
		
		return (const void *)data;
	}
	
	void *Mesh::FetchDataForFeature(MeshFeature feature)
	{
		int32 index = (int32)feature;
		
		if(!_descriptor[index]._available)
			return 0;
		
		_descriptor[index]._dirty = true;
		return (void *)FetchConstDataForFeature(feature);
	}
	
	void Mesh::ReleaseData(MeshFeature feature)
	{
		int32 index = (int32)feature;
		if((--_descriptor[index]._useCount) == 0)
		{
			bool regenerateMesh = true;
			bool hasDirtyMesh = false;
			
			for(int i=0; i<__kMaxMeshFeatures; i++)
			{
				if(_descriptor[i]._useCount > 0)
				{
					regenerateMesh = false;
					break;
				}
				
				if(_descriptor[i]._dirty)
					hasDirtyMesh = true;
			}
			
			if(regenerateMesh && hasDirtyMesh)
			{
				GenerateMesh();
				
				for(int i=0; i<__kMaxMeshFeatures; i++)
				{
					if(i == kMeshFeatureVertices)
						continue;
					
					if(_descriptor[i]._pointer)
					{
						Memory::FreeSIMD(_descriptor[i]._pointer);
						
						_descriptor[i]._pointer = 0;
						_descriptor[i]._dirty = false;
					}
				}
			}
		}
	}
	
	void Mesh::CalculateBoundingBox()
	{
		Vector3 min = Vector3();
		Vector3 max = Vector3();
		
		bool wasDirty = _descriptor[kMeshFeatureVertices]._dirty;
		
		const Vector3 *vertices = Data<Vector3>(kMeshFeatureVertices);
		if(vertices)
		{
			max = min = *vertices;
			
			size_t count = _descriptor[kMeshFeatureVertices].elementCount;
			for(size_t i=1; i<count; i++)
			{
				const Vector3 *vertex = vertices + i;
				
				min.x = MIN(vertex->x, min.x);
				min.y = MIN(vertex->y, min.y);
				min.z = MIN(vertex->z, min.z);
				
				max.x = MAX(vertex->x, max.x);
				max.y = MAX(vertex->y, max.y);
				max.z = MAX(vertex->z, max.z);
			}
		}
		
		_boundingBox = AABB(min, max);
		_boundingSphere = Sphere(_boundingBox);
		
		_descriptor[kMeshFeatureVertices]._useCount --;
		_descriptor[kMeshFeatureVertices]._dirty = wasDirty;
	}
	
	void Mesh::UpdateMesh()
	{
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, _meshSize, 0, _vboUsage);
		glBufferData(GL_ARRAY_BUFFER, _meshSize, _meshData, _vboUsage);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glFlush();
	}
	
	void Mesh::GenerateMesh()
	{
		if(!_meshData)
			_meshData = static_cast<uint8 *>(Memory::AllocateSIMD(_meshSize));
		
		if(!_indices && _indicesSize > 0)
			_indices = static_cast<uint8 *>(Memory::AllocateSIMD(_indicesSize));
		
		bool meshChanged = false;
		bool indicesChanged = false;
		
		uint8 *bytes = static_cast<uint8 *>(_meshData);
		uint8 *bytesEnd = bytes + _meshSize;
		
		uint8 *buffer[kMeshFeatureIndices];
		
		for(int i=0; i<kMeshFeatureIndices; i++)
			buffer[i] = _descriptor[i]._pointer;
		
		// Fill the mesh buffer
		while(bytes < bytesEnd)
		{
			for(int i=0; i<kMeshFeatureIndices; i++)
			{
				if(_descriptor[i]._available)
				{
					if(_descriptor[i]._dirty)
					{
						if(_descriptor[i]._pointer)
						{
							std::copy(buffer[i], buffer[i] + _descriptor[i].elementSize, bytes);
							buffer[i] += _descriptor[i].elementSize;
						}
						
						meshChanged = true;
					}
					
					bytes += _descriptor[i].elementSize;
				}
			}
		}
		
		// Fill the indices buffer
		if(_descriptor[kMeshFeatureIndices]._available && _descriptor[kMeshFeatureIndices]._dirty)
		{
			uint8 *indices = _descriptor[kMeshFeatureIndices]._pointer;
			std::copy(indices, indices + _indicesSize, static_cast<uint8 *>(_indices));
			
			indicesChanged = true;
		}
		
		if(meshChanged)
		{
			glBindBuffer(GL_ARRAY_BUFFER, _vbo);
			glBufferData(GL_ARRAY_BUFFER, _meshSize, 0, _vboUsage);
			glBufferData(GL_ARRAY_BUFFER, _meshSize, _meshData, _vboUsage);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			RN_CHECKOPENGL();
		}
		
		if(indicesChanged)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, 0, _iboUsage);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, _indices, _iboUsage);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			RN_CHECKOPENGL();
		}
		
		glFlush();
		CalculateBoundingBox();
	}

	bool Mesh::SupportsFeature(MeshFeature feature)
	{
		return _descriptor[(int32)feature]._available;
	}
	
	size_t Mesh::OffsetForFeature(MeshFeature feature)
	{
		return _descriptor[(int32)feature].offset;
	}
	
	
	
	Mesh *Mesh::PlaneMesh(const Vector3& size, const Vector3& rotation)
	{
		MeshDescriptor vertexDescriptor;
		vertexDescriptor.feature = kMeshFeatureVertices;
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 4;
		
		MeshDescriptor texcoordDescriptor;
		texcoordDescriptor.feature = kMeshFeatureUVSet0;
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 4;
		
		MeshDescriptor indicesDescriptor;
		indicesDescriptor.feature = kMeshFeatureIndices;
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 6;
		
		Array<MeshDescriptor> descriptors;
		descriptors.AddObject(vertexDescriptor);
		descriptors.AddObject(indicesDescriptor);
		descriptors.AddObject(texcoordDescriptor);
		
		
		Mesh *mesh = new Mesh(descriptors);
		
		Vector3 *vertices  = mesh->MutableData<Vector3>(kMeshFeatureVertices);
		Vector2 *texcoords = mesh->MutableData<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = mesh->MutableData<uint16>(kMeshFeatureIndices);
		
		Matrix rotmat;
		rotmat.MakeRotate(rotation);
		
		*vertices ++ = rotmat.Transform(Vector3(-size.x, size.y, -size.z));
		*vertices ++ = rotmat.Transform(Vector3( size.x, size.y, -size.z));
		*vertices ++ = rotmat.Transform(Vector3( size.x, size.y, size.z));
		*vertices ++ = rotmat.Transform(Vector3(-size.x, size.y, size.z));
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*indices ++ = 0;
		*indices ++ = 3;
		*indices ++ = 1;
		*indices ++ = 2;
		*indices ++ = 1;
		*indices ++ = 3;
		
		mesh->ReleaseData(kMeshFeatureVertices);
		mesh->ReleaseData(kMeshFeatureUVSet0);
		mesh->ReleaseData(kMeshFeatureIndices);
		
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size)
	{
		MeshDescriptor vertexDescriptor;
		vertexDescriptor.feature = kMeshFeatureVertices;
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 24;
		
		MeshDescriptor normalDescriptor;
		normalDescriptor.feature = kMeshFeatureNormals;
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		normalDescriptor.elementCount  = 24;
		
		MeshDescriptor texcoordDescriptor;
		texcoordDescriptor.feature = kMeshFeatureUVSet0;
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 24;
		
		MeshDescriptor indicesDescriptor;
		indicesDescriptor.feature = kMeshFeatureIndices;
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 36;
		
		Array<MeshDescriptor> descriptors;
		descriptors.AddObject(vertexDescriptor);
		descriptors.AddObject(normalDescriptor);
		descriptors.AddObject(indicesDescriptor);
		descriptors.AddObject(texcoordDescriptor);
		
		
		Mesh *mesh = new Mesh(descriptors);
		
		Vector3 *vertices  = mesh->MutableData<Vector3>(kMeshFeatureVertices);
		Vector3 *normals   = mesh->MutableData<Vector3>(kMeshFeatureNormals);
		Vector2 *texcoords = mesh->MutableData<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = mesh->MutableData<uint16>(kMeshFeatureIndices);
		
		*vertices ++ = Vector3(-size.x,  size.y, size.z);
		*vertices ++ = Vector3( size.x,  size.y, size.z);
		*vertices ++ = Vector3( size.x, -size.y, size.z);
		*vertices ++ = Vector3(-size.x, -size.y, size.z);
		
		*vertices ++ = Vector3(-size.x,  size.y, -size.z);
		*vertices ++ = Vector3( size.x,  size.y, -size.z);
		*vertices ++ = Vector3( size.x, -size.y, -size.z);
		*vertices ++ = Vector3(-size.x, -size.y, -size.z);
		
		*vertices ++ = Vector3(-size.x,  size.y, -size.z);
		*vertices ++ = Vector3(-size.x,  size.y,  size.z);
		*vertices ++ = Vector3(-size.x, -size.y,  size.z);
		*vertices ++ = Vector3(-size.x, -size.y, -size.z);
		
		*vertices ++ = Vector3(size.x,  size.y, -size.z);
		*vertices ++ = Vector3(size.x,  size.y,  size.z);
		*vertices ++ = Vector3(size.x, -size.y,  size.z);
		*vertices ++ = Vector3(size.x, -size.y, -size.z);
		
		*vertices ++ = Vector3(-size.x, size.y,  size.z);
		*vertices ++ = Vector3( size.x, size.y,  size.z);
		*vertices ++ = Vector3( size.x, size.y, -size.z);
		*vertices ++ = Vector3(-size.x, size.y, -size.z);
		
		*vertices ++ = Vector3(-size.x, -size.y, -size.z);
		*vertices ++ = Vector3( size.x, -size.y, -size.z);
		*vertices ++ = Vector3( size.x, -size.y,  size.z);
		*vertices ++ = Vector3(-size.x, -size.y,  size.z);
		
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);		
		
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
		
		mesh->ReleaseData(kMeshFeatureVertices);
		mesh->ReleaseData(kMeshFeatureUVSet0);
		mesh->ReleaseData(kMeshFeatureNormals);
		mesh->ReleaseData(kMeshFeatureIndices);
		
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size, const Color& color)
	{
		MeshDescriptor vertexDescriptor;
		vertexDescriptor.feature = kMeshFeatureVertices;
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 24;
		
		MeshDescriptor normalDescriptor;
		normalDescriptor.feature = kMeshFeatureNormals;
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		normalDescriptor.elementCount  = 24;
		
		MeshDescriptor colorDescriptor;
		colorDescriptor.feature = kMeshFeatureColor0;
		colorDescriptor.elementSize = sizeof(Color);
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementCount  = 24;
		
		MeshDescriptor texcoordDescriptor;
		texcoordDescriptor.feature = kMeshFeatureUVSet0;
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 24;
		
		MeshDescriptor indicesDescriptor;
		indicesDescriptor.feature = kMeshFeatureIndices;
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 36;
		
		Array<MeshDescriptor> descriptors;
		descriptors.AddObject(vertexDescriptor);
		descriptors.AddObject(normalDescriptor);
		descriptors.AddObject(colorDescriptor);
		descriptors.AddObject(indicesDescriptor);
		descriptors.AddObject(texcoordDescriptor);
		
		
		Mesh *mesh = new Mesh(descriptors);
		
		Vector3 *vertices  = mesh->MutableData<Vector3>(kMeshFeatureVertices);
		Vector3 *normals   = mesh->MutableData<Vector3>(kMeshFeatureNormals);
		Color *colors      = mesh->MutableData<Color>(kMeshFeatureColor0);
		Vector2 *texcoords = mesh->MutableData<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = mesh->MutableData<uint16>(kMeshFeatureIndices);
		
		*vertices ++ = Vector3(-size.x,  size.y, size.z);
		*vertices ++ = Vector3( size.x,  size.y, size.z);
		*vertices ++ = Vector3( size.x, -size.y, size.z);
		*vertices ++ = Vector3(-size.x, -size.y, size.z);
		
		*vertices ++ = Vector3(-size.x,  size.y, -size.z);
		*vertices ++ = Vector3( size.x,  size.y, -size.z);
		*vertices ++ = Vector3( size.x, -size.y, -size.z);
		*vertices ++ = Vector3(-size.x, -size.y, -size.z);
		
		*vertices ++ = Vector3(-size.x,  size.y, -size.z);
		*vertices ++ = Vector3(-size.x,  size.y,  size.z);
		*vertices ++ = Vector3(-size.x, -size.y,  size.z);
		*vertices ++ = Vector3(-size.x, -size.y, -size.z);
		
		*vertices ++ = Vector3(size.x,  size.y, -size.z);
		*vertices ++ = Vector3(size.x,  size.y,  size.z);
		*vertices ++ = Vector3(size.x, -size.y,  size.z);
		*vertices ++ = Vector3(size.x, -size.y, -size.z);
		
		*vertices ++ = Vector3(-size.x, size.y,  size.z);
		*vertices ++ = Vector3( size.x, size.y,  size.z);
		*vertices ++ = Vector3( size.x, size.y, -size.z);
		*vertices ++ = Vector3(-size.x, size.y, -size.z);
		
		*vertices ++ = Vector3(-size.x, -size.y, -size.z);
		*vertices ++ = Vector3( size.x, -size.y, -size.z);
		*vertices ++ = Vector3( size.x, -size.y,  size.z);
		*vertices ++ = Vector3(-size.x, -size.y,  size.z);
		
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
		
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		*normals ++ = Vector3(0.0f, 0.0f, 1.0);
		
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		*normals ++ = Vector3(0.0f, 0.0f, -1.0);
		
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(-1.0f, 0.0f, 0.0);
		
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		*normals ++ = Vector3(1.0f, 0.0f, 0.0);
		
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		*normals ++ = Vector3(0.0f, 1.0f, 0.0);
		
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		*normals ++ = Vector3(0.0f, -1.0f, 0.0);
		
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		*colors ++ = color;
		
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
		
		mesh->ReleaseData(kMeshFeatureVertices);
		mesh->ReleaseData(kMeshFeatureUVSet0);
		mesh->ReleaseData(kMeshFeatureNormals);
		mesh->ReleaseData(kMeshFeatureColor0);
		mesh->ReleaseData(kMeshFeatureIndices);
		
		return mesh;
	}
}
