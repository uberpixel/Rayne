//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"
#include "RNKernel.h"

namespace RN
{
	RNDeclareMeta(Mesh)
	
	MeshLODStage::MeshLODStage(const Array<MeshDescriptor>& descriptor)
	{
		_meshSize = 0;
		_meshData = 0;
		
		_indicesSize = 0;
		_indices     = 0;
		
		_instancing._data = 0;
		_instancing._size = 0;
		_instancing._vbo = 0;
		
		glGenBuffers(2, &_vbo);
		RN_CHECKOPENGL();
		
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			_descriptor[i]._available = false;
			_descriptor[i]._pointer = 0;
			_descriptor[i].offset  = 0;
		}
		
		for(machine_uint i=0; i<descriptor.Count(); i++)
		{
			size_t size = descriptor[(int)i].elementSize * descriptor[(int)i].elementCount;
			int index = (int)descriptor[(int)i].feature;
			
			if(_descriptor[index]._available == false)
			{
				_descriptor[index] = descriptor[(int)i];
				
				if(_descriptor[index].feature != kMeshFeatureIndices)
					_meshSize += size;
				else
					_indicesSize += size;
				
				_descriptor[index].offset = 0;
				_descriptor[index]._pointer = 0;
				_descriptor[index]._available = true;
				_descriptor[index]._size = size;
			}
		}
		
		size_t offset = 0;
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			if(_descriptor[i]._available)
			{
				_descriptor[i].offset = offset;
				offset += _descriptor[i].elementSize;
			}
		}
	}
	
	MeshLODStage::MeshLODStage(const Array<MeshDescriptor>& descriptor, const void *data)
	{
		_meshSize = 0;
		_meshData = 0;
		
		_indicesSize = 0;
		_indices     = 0;
		
		_instancing._data = 0;
		_instancing._size = 0;
		_instancing._vbo = 0;
		
		glGenBuffers(2, &_vbo);
		RN_CHECKOPENGL();
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			_descriptor[i]._available = false;
			_descriptor[i]._pointer = 0;
			_descriptor[i].offset  = 0;
		}
		
		for(machine_uint i=0; i<descriptor.Count(); i++)
		{
			size_t size = descriptor[(int)i].elementSize * descriptor[(int)i].elementCount;
			int index   = (int)descriptor[(int)i].feature;
			
			if(_descriptor[index]._available == false)
			{
				_descriptor[index] = descriptor[(int)i];
				
				if(_descriptor[index].feature != kMeshFeatureIndices)
					_meshSize += size;
				else
					_indicesSize += size;
				
				_descriptor[index].offset = descriptor[(int)i].offset;
				_descriptor[index]._pointer = 0;
				_descriptor[index]._available = true;
				_descriptor[index]._size = size;
			}
		}
		
		_meshData = malloc(_meshSize);
		memcpy(_meshData, data, _meshSize);
	}
	
	MeshLODStage::~MeshLODStage()
	{
		glDeleteBuffers(2, &_vbo);
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			if(_descriptor[i]._pointer)
				free(_descriptor[i]._pointer);
		}
		
		if(_meshData)
			free(_meshData);
		
		if(_indices)
			free(_indices);
		
		if(_instancing._data)
			free(_instancing._data);
		
		if(_instancing._vbo)
			glDeleteBuffers(1, &_instancing._vbo);
	}
	
	void *MeshLODStage::FetchDataForFeature(MeshFeature feature)
	{
		int32 index = (int32)feature;
		
		if(!_descriptor[index]._available)
			return 0;
		
		if(_descriptor[index]._pointer)
			return _descriptor[index]._pointer;
		
		uint8 *data = (uint8 *)malloc(_descriptor[index]._size);
		uint8 *meshData = static_cast<uint8 *>(_meshData);
		
		if(meshData)
		{
			size_t offset = OffsetForFeature(feature);
			size_t size = _descriptor[feature].elementSize;
			
			uint8 *tdata = data;
			
			while(offset < _meshSize)
			{
				std::copy(meshData + offset, meshData + (offset + size), tdata);
				
				offset += _stride;
				tdata  += size;
			}
		}
		
		_descriptor[index]._pointer = data;
		return data;
	}
	
	void MeshLODStage::GenerateMesh()
	{
		if(!_meshData)
			_meshData = malloc(_meshSize);
		
		if(!_indices)
			_indices = malloc(_indicesSize);
		
		uint8 *bytes = static_cast<uint8 *>(_meshData);
		uint8 *bytesEnd = bytes + _meshSize;
		
		uint8 *buffer[kMeshFeatureIndices];
		
		_stride = 0;
		
		for(int i=0; i<kMeshFeatureIndices; i++)
		{
			buffer[i] = 0;
			
			if(_descriptor[i]._available)
			{
				if(_descriptor[i]._pointer)
					buffer[i] = _descriptor[i]._pointer;
				
				_stride += _descriptor[i].elementSize;
			}
		}
		
		while(bytes < bytesEnd)
		{
			for(int i=0; i<kMeshFeatureIndices; i++)
			{
				if(_descriptor[i]._available)
				{
					if(_descriptor[i]._pointer)
						std::copy(buffer[i], buffer[i] +_descriptor[i].elementSize, bytes);
					
					bytes     += _descriptor[i].elementSize;
					buffer[i] += _descriptor[i].elementSize;
				}
			}
		}
		
		if(_descriptor[kMeshFeatureIndices]._available)
		{
			uint8 *indices = Data<uint8>(kMeshFeatureIndices);
			std::copy(indices, indices + _indicesSize, static_cast<uint8 *>(_indices));
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, _meshSize, _meshData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		RN_CHECKOPENGL();
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, _indices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		RN_CHECKOPENGL();
		
		glFlush();
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			if(_descriptor[i]._available && _descriptor[i]._pointer)
			{
				free(_descriptor[i]._pointer);
				_descriptor[i]._pointer = 0;
			}
		}
	}

	bool MeshLODStage::SupportsFeature(MeshFeature feature)
	{
		return _descriptor[(int32)feature]._available;
	}
	
	size_t MeshLODStage::OffsetForFeature(MeshFeature feature)
	{
		return _descriptor[(int32)feature].offset;
	}
	
	bool MeshLODStage::InstancingData(size_t size, GLuint *outVBO, void **outData)
	{
		RN_ASSERT0(outVBO);
		RN_ASSERT0(outData);
		
		bool didResize = false;
		
		if(_instancing._vbo == 0)
		{
			glGenBuffers(1, &_instancing._vbo);
			RN_CHECKOPENGL();
		}

		if(_instancing._data == 0)
		{
			void *temp = malloc(size);
			if(!temp)
				throw ErrorException(0, 0, 0);
			
			_instancing._data = temp;
			_instancing._size = size;
			
			didResize = true;
		}
		
		if(size > _instancing._size)
		{
			void *temp = realloc(_instancing._data, size);
			if(!temp)
				throw ErrorException(0, 0, 0);
				
			_instancing._data = temp;
			_instancing._size = size;
			
			didResize = true;
		}
		
		*outVBO  = _instancing._vbo;
		*outData = _instancing._data;
		
		return didResize;
	}

	
	
	
	Mesh::Mesh()
	{
	}
	
	Mesh::~Mesh()
	{
		for(machine_uint i=0; i<_LODStages.Count(); i++)
			delete _LODStages[(int)i];
	}
	
	MeshLODStage *Mesh::AddLODStage(const Array<MeshDescriptor>& descriptor)
	{
		MeshLODStage *stage = new MeshLODStage(descriptor);
		_LODStages.AddObject(stage);
		
		return stage;
	}
	
	MeshLODStage *Mesh::AddLODStage(const Array<MeshDescriptor>& descriptor, const void *data)
	{
		MeshLODStage *stage = new MeshLODStage(descriptor, data);
		_LODStages.AddObject(stage);
		
		return stage;
	}
	
	MeshLODStage *Mesh::LODStage(int index)
	{
		return _LODStages[index];
	}
	
	void Mesh::UpdateMesh()
	{
		for(machine_uint i=0; i<_LODStages.Count(); i++)
		{
			_LODStages[(int)i]->GenerateMesh();
		}
	}
	
	Mesh *Mesh::PlaneMesh(const Vector3& size, const Vector3& rotation)
	{
		Mesh *mesh = new Mesh();
		
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
		
		
		MeshLODStage *stage = mesh->AddLODStage(descriptors);
		
		Vector3 *vertices  = stage->Data<Vector3>(kMeshFeatureVertices);
		Vector2 *texcoords = stage->Data<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = stage->Data<uint16>(kMeshFeatureIndices);
		
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
		
		mesh->UpdateMesh();
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size)
	{
		Mesh *mesh = new Mesh();
		
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
		
		
		MeshLODStage *stage = mesh->AddLODStage(descriptors);
		
		Vector3 *vertices  = stage->Data<Vector3>(kMeshFeatureVertices);
		Vector3 *normals   = stage->Data<Vector3>(kMeshFeatureNormals);
		Vector2 *texcoords = stage->Data<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = stage->Data<uint16>(kMeshFeatureIndices);
		
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
		
		mesh->UpdateMesh();
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size, const Color& color)
	{
		Mesh *mesh = new Mesh();
		
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
		
		
		MeshLODStage *stage = mesh->AddLODStage(descriptors);
		
		Vector3 *vertices  = stage->Data<Vector3>(kMeshFeatureVertices);
		Vector3 *normals   = stage->Data<Vector3>(kMeshFeatureNormals);
		Color *colors      = stage->Data<Color>(kMeshFeatureColor0);
		Vector2 *texcoords = stage->Data<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = stage->Data<uint16>(kMeshFeatureIndices);
		
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
		
		mesh->UpdateMesh();
		return mesh;
	}
}
