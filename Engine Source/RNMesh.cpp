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
	
	MeshDescriptor::MeshDescriptor(MeshFeature tfeature, uint32 tflags)
	{
		feature = tfeature;
		flags   = tflags;
		
		elementMember = 0;
		elementSize   = 0;
		elementCount  = 0;
		
		offset = -1;
		
		_size = 0;
		_alignment = (flags & DescriptorFlagSIMDAlignment) ? 16 : 0;
		
		_pointer = 0;
		_useCount = 0;
	}

	
	
	Mesh::Mesh(const std::vector<MeshDescriptor>& descriptor)
	{
		Initialize();
		AddDescriptor(descriptor);
	}
	
	Mesh::Mesh(const std::vector<MeshDescriptor>& descriptor, const void *data)
	{
		Initialize();
		AddDescriptor(descriptor);
		
		uint8 *mdata = MeshData<uint8>();
		uint8 *source = reinterpret_cast<uint8 *>(const_cast<void *>(data));
		
		std::copy(source, source + _meshSize, mdata);
		_dirty = true;
	}
	
	Mesh::~Mesh()
	{
		glDeleteBuffers(2, &_vbo);
		
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			if(i->_pointer)
				Memory::FreeSIMD(i->_pointer);
		}
		
		if(_meshData)
			Memory::FreeSIMD(_meshData);
		
		if(_indices)
			Memory::FreeSIMD(_indices);
	}
	
	
	void Mesh::Initialize()
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
	}
	
	
	
	void Mesh::AddDescriptor(const std::vector<MeshDescriptor>& descriptor)
	{
		if(descriptor.size() > 0)
		{
			_descriptor.insert(_descriptor.end(), descriptor.begin(), descriptor.end());
			RecalculateInternalData();
		}
	}
	
	void Mesh::RemoveDescriptor(MeshFeature feature)
	{
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			if(i->feature == feature)
			{
				_descriptor.erase(i);
				
				RecalculateInternalData();
				return;
			}
		}
	}
	
	void Mesh::RecalculateInternalData()
	{
		size_t offset = 0;
		
		_stride = 0;
		
		_meshSize    = 0;
		_indicesSize = 0;
		
		_dirty = _dirtyIndices = true;
		
		if(_meshData)
		{
			Memory::FreeSIMD(_meshData);
			_meshSize = 0;
		}
		
		if(_indices)
		{
			Memory::FreeSIMD(_indices);
			_indices = 0;
		}
		
		
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			i->_size = i->elementSize * i->elementCount;
			
			if(i->feature != kMeshFeatureIndices)
			{
				i->offset = offset;
				offset += i->elementSize;
				
				_meshSize += i->_size;
				_stride += i->elementSize;
			}
			else
			{
				_indicesSize += i->_size;
				i->offset = 0;
			}
		}
		
		AllocateStorage();
	}
	
	void Mesh::AllocateStorage()
	{
		if(!_meshData && _meshSize > 0)
			_meshData = static_cast<uint8 *>(Memory::AllocateSIMD(_meshSize));
		
		if(!_indices && _indicesSize > 0)
			_indices = static_cast<uint8 *>(Memory::AllocateSIMD(_indicesSize));
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
	
	void Mesh::SetElement(MeshFeature feature, void *tdata)
	{
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			if(i->feature == feature)
			{
				if(feature != kMeshFeatureIndices)
				{
					uint8 *data = static_cast<uint8 *>(tdata);
					uint8 *buffer = _meshData + i->offset;
					
					for(size_t j=0; j<i->elementCount; j++)
					{
						std::copy(data, data + i->elementSize, buffer);
						
						buffer += _stride;
						data += i->elementSize;
					}
					
					_dirty = true;
				}
				else
				{
					uint8 *data = static_cast<uint8 *>(tdata);
					std::copy(data, data + _indicesSize, _indices);
					
					_dirtyIndices = true;
				}
				
				if(i->_pointer)
				{
					uint8 *data = static_cast<uint8 *>(tdata);
					std::copy(data, data + i->_size, i->_pointer);
				}
				
				break;
			}
		}
	}
	
	
	
	void *Mesh::FetchElement(MeshFeature feature, size_t index)
	{
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			if(i->feature == feature)
			{
				if(feature != kMeshFeatureIndices)
				{
					_dirty = true;
					
					uint8 *data = _meshData + i->offset;
					return data + (index * _stride);
				}
				else
				{
					_dirtyIndices = true;
					return _indices + (index * i->elementSize);
				}
			}
		}
		
		return 0;
	}
	
	void *Mesh::CopyElement(MeshFeature feature)
	{		
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			if(i->feature == feature)
			{
				if(!i->_pointer)
				{
					if(feature != kMeshFeatureIndices)
					{
						uint8 *data = static_cast<uint8 *>(Memory::AllocateSIMD(i->_size));
						uint8 *buffer = _meshData + i->offset;
						
						i->_pointer = data;
						
						for(size_t j=0; j<i->elementCount; j++)
						{
							std::copy(buffer, buffer + i->elementSize, data);
							
							buffer += _stride;
							data += i->elementSize;
						}
					}
					else
					{
						i->_pointer = static_cast<uint8 *>(Memory::AllocateSIMD(_indicesSize));
						std::copy(_indices, _indices + _indicesSize, i->_pointer);
					}
				}
				
				i->_useCount ++;
				return i->_pointer;
			}
		}
		
		return 0;
	}
	
	
	void Mesh::ReleaseElement(MeshFeature feature)
	{
		for(auto i=_descriptor.begin(); i!=_descriptor.end(); i++)
		{
			if(i->feature == feature)
			{
				if((-- i->_useCount) == 0)
				{
					void *data = i->_pointer;
					i->_pointer = 0;
					
					SetElement(feature, data);
					Memory::FreeSIMD(data);
				}
				
				break;
			}
		}
	}
	
	void Mesh::CalculateBoundingBox()
	{
		Vector3 min = Vector3();
		Vector3 max = Vector3();
		
		bool wasDirty = _dirty;
		
		MeshDescriptor *descriptor = Descriptor(kMeshFeatureVertices);
		uint8 *pointer = _meshData + descriptor->offset;
		
		Vector3 *vertex = reinterpret_cast<Vector3 *>(pointer);
		if(vertex)
		{
			max = min = *vertex;
			
			for(size_t i=1; i<descriptor->elementCount; i++)
			{
				pointer += _stride;
				vertex = reinterpret_cast<Vector3 *>(pointer);
				
				min.x = MIN(vertex->x, min.x);
				min.y = MIN(vertex->y, min.y);
				min.z = MIN(vertex->z, min.z);
				
				max.x = MAX(vertex->x, max.x);
				max.y = MAX(vertex->y, max.y);
				max.z = MAX(vertex->z, max.z);
			}
		}
		
		_dirty = wasDirty;
		
		_boundingBox = AABB(min, max);
		_boundingSphere = Sphere(_boundingBox);
	}
	
	
	void Mesh::UpdateMesh(bool force)
	{
		if((_dirtyIndices || force) && _indices)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, _indices, _iboUsage);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			
			_dirtyIndices = false;
		}
		
		if((_dirty || force) && _meshData)
		{
			glBindBuffer(GL_ARRAY_BUFFER, _vbo);
			glBufferData(GL_ARRAY_BUFFER, _meshSize, _meshData, _vboUsage);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			
			_dirty = false;
			CalculateBoundingBox();
		}
		
		glFlush();
	}
	
	
	
	GLuint Mesh::VBO()
	{
		UpdateMesh();
		return _vbo;
	}
	
	GLuint Mesh::IBO()
	{
		UpdateMesh();
		return _ibo;
	}

	
	
	bool Mesh::SupportsFeature(MeshFeature feature)
	{
		for(size_t i=0; i<_descriptor.size(); i++)
		{
			if(_descriptor[i].feature == feature)
				return true;
		}
		
		return false;
	}
	
	size_t Mesh::OffsetForFeature(MeshFeature feature)
	{
		for(size_t i=0; i<_descriptor.size(); i++)
		{
			if(_descriptor[i].feature == feature)
				return _descriptor[i].offset;
		}
		
		return -1;
	}
	
	MeshDescriptor *Mesh::Descriptor(MeshFeature feature)
	{
		for(size_t i=0; i<_descriptor.size(); i++)
		{
			if(_descriptor[i].feature == feature)
				return &_descriptor[i];
		}
		
		return 0;
	}
	
	
	
	
	Mesh *Mesh::PlaneMesh(const Vector3& size, const Vector3& rotation)
	{
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 4;
		
		MeshDescriptor texcoordDescriptor(kMeshFeatureUVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 4;
		
		MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 6;

		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor);
		
		Vector3 *vertices  = mesh->Element<Vector3>(kMeshFeatureVertices);
		Vector2 *texcoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = mesh->Element<uint16>(kMeshFeatureIndices);
		
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
		
		mesh->ReleaseElement(kMeshFeatureVertices);
		mesh->ReleaseElement(kMeshFeatureUVSet0);
		mesh->ReleaseElement(kMeshFeatureIndices);
		mesh->UpdateMesh();
		
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size)
	{
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 24;
		
		MeshDescriptor normalDescriptor(kMeshFeatureNormals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		normalDescriptor.elementCount  = 24;
		
		MeshDescriptor texcoordDescriptor(kMeshFeatureUVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 24;
		
		MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 36;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, normalDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor);
		
		Vector3 *vertices  = mesh->Element<Vector3>(kMeshFeatureVertices);
		Vector3 *normals   = mesh->Element<Vector3>(kMeshFeatureNormals);
		Vector2 *texcoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = mesh->Element<uint16>(kMeshFeatureIndices);
		
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
		
		mesh->ReleaseElement(kMeshFeatureVertices);
		mesh->ReleaseElement(kMeshFeatureUVSet0);
		mesh->ReleaseElement(kMeshFeatureNormals);
		mesh->ReleaseElement(kMeshFeatureIndices);
		mesh->UpdateMesh();
		
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size, const Color& color)
	{
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 24;
		
		MeshDescriptor normalDescriptor(kMeshFeatureNormals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		normalDescriptor.elementCount  = 24;
		
		MeshDescriptor colorDescriptor(kMeshFeatureColor0);
		colorDescriptor.elementSize = sizeof(Color);
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementCount  = 24;
		
		MeshDescriptor texcoordDescriptor(kMeshFeatureUVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 24;
		
		MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 36;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, normalDescriptor, colorDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor);
		
		Vector3 *vertices  = mesh->Element<Vector3>(kMeshFeatureVertices);
		Vector3 *normals   = mesh->Element<Vector3>(kMeshFeatureNormals);
		Color *colors      = mesh->Element<Color>(kMeshFeatureColor0);
		Vector2 *texcoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = mesh->Element<uint16>(kMeshFeatureIndices);
		
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
		
		mesh->ReleaseElement(kMeshFeatureVertices);
		mesh->ReleaseElement(kMeshFeatureUVSet0);
		mesh->ReleaseElement(kMeshFeatureNormals);
		mesh->ReleaseElement(kMeshFeatureColor0);
		mesh->ReleaseElement(kMeshFeatureIndices);
		mesh->UpdateMesh();
		
		return mesh;
	}
}
