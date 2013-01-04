//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"
#include "RNKernel.h"

namespace RN
{
	MeshLODStage::MeshLODStage(const Array<MeshDescriptor>& descriptor)
	{
		_meshSize = 0;
		_meshData = 0;
		
		_indicesSize = 0;
		_indices     = 0;
		
		glGenBuffers(2, &_vbo);
		Kernel::CheckOpenGLError("glGenBuffers");
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			_descriptor[i]._available = false;
			_descriptor[i]._pointer = 0;
			_descriptor[i]._offset  = 0;
		}
		
		for(int i=0; i<descriptor.Count(); i++)
		{
			size_t size = descriptor[i].elementSize * descriptor[i].elementCount;
			int index = (int)descriptor[i].feature;
			
			if(_descriptor[index]._available == false)
			{
				_descriptor[index] = descriptor[i];
				
				if(_descriptor[index].feature != kMeshFeatureIndices)
					_meshSize += size;
				else
					_indicesSize += size;
				
				_descriptor[index]._pointer = (uint8 *)malloc(size);
				_descriptor[index]._offset = 0;
				_descriptor[index]._available = true;
			}
		}
		
		size_t offset = 0;
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			if(_descriptor[i]._available)
			{
				_descriptor[i]._offset = offset;
				offset += _descriptor[i].elementSize;
				
			}
		}
	}
	
	MeshLODStage::~MeshLODStage()
	{
		if(_vbo)
			glDeleteBuffers(1, &_vbo);
		
		if(_ibo)
			glDeleteBuffers(1, &_ibo);
		
		for(int i=0; i<__kMaxMeshFeatures; i++)
		{
			if(_descriptor[i]._pointer)
				free(_descriptor[i]._pointer);
		}
		
		if(_meshData)
			free(_meshData);
		
		if(_indices)
			free(_indices);
	}
	
	void MeshLODStage::GenerateMesh()
	{
		if(!_meshData)
			_meshData = malloc(_meshSize);
		
		if(!_indices)
			_indices = malloc(_indicesSize);
		
		uint8 *bytes = (uint8 *)_meshData;
		uint8 *bytesEnd = bytes + _meshSize;
		
		uint8 *buffer[kMeshFeatureIndices];
		
		_stride = 0;
		
		for(int i=0; i<kMeshFeatureIndices; i++)
		{
			buffer[i] = 0;
			
			if(_descriptor[i]._available)
			{
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
					memcpy(bytes, buffer[i], _descriptor[i].elementSize);
					
					bytes     += _descriptor[i].elementSize;
					buffer[i] += _descriptor[i].elementSize;
				}
			}
		}
		
		if(_descriptor[kMeshFeatureIndices]._available)
			memcpy(_indices, _descriptor[kMeshFeatureIndices]._pointer, _indicesSize);
		
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, _meshSize, _meshData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		Kernel::CheckOpenGLError("glBufferData");
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, _indices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		Kernel::CheckOpenGLError("glBufferData");
	}

	bool MeshLODStage::SupportsFeature(MeshFeature feature)
	{
		return _descriptor[(int32)feature]._available;
	}
	
	size_t MeshLODStage::OffsetForFeature(MeshFeature feature)
	{
		return _descriptor[(int32)feature]._offset;
	}
	

	
	
	
	Mesh::Mesh() :
		RenderingResource("Mesh")
	{
	}
	
	Mesh::~Mesh()
	{
		for(int i=0; i<_LODStages.Count(); i++)
			delete _LODStages[i];
	}
	
	MeshLODStage *Mesh::AddLODStage(const Array<MeshDescriptor>& descriptor)
	{
		MeshLODStage *stage = new MeshLODStage(descriptor);
		_LODStages.AddObject(stage);
		
		return stage;
	}
	
	MeshLODStage *Mesh::LODStage(int index)
	{
		return _LODStages[index];
	}
	
	void Mesh::UpdateMesh()
	{
		for(int i=0; i<_LODStages.Count(); i++)
		{
			_LODStages[i]->GenerateMesh();
		}
	}
}
