//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"

namespace RN
{
	MeshLODStage::MeshLODStage(const Array<MeshDescriptor>& descriptor)
	{
		_descriptor = Array<MeshDescriptor>(descriptor);
		_meshSize = 0;
		_meshData = 0;
		
		_vbo = _ivbo = -1;
		
		for(int i=0; i<_descriptor.Count(); i++)
		{
			size_t size = _descriptor[i].elements * SizeForMeshFeatureType(_descriptor[i].type);
			
			if(_descriptor[i].feature != kMeshFeatureIndices)
				_meshSize += size;
			
			_descriptor[i]._pointer = (uint8 *)malloc(size);
			_descriptor[i]._offset = 0;
		}
	}
	
	MeshLODStage::~MeshLODStage()
	{
		if(_vbo != -1)
			glDeleteBuffers(1, &_vbo);
		
		if(_ivbo != -1)
			glDeleteBuffers(1, &_ivbo);
		
		for(int i=0; i<_descriptor.Count(); i++)
		{
			if(_descriptor[i]._pointer)
				free(_descriptor[i]._pointer);
		}
		
		if(_meshData)
			free(_meshData);
	}
	
	void MeshLODStage::GenerateMesh()
	{
		if(!_meshData)
			_meshData = malloc(_meshSize);
		
		uint8 *bytes = (uint8 *)_meshData;
		
		for(int i=0; i<_meshSize;)
		{
			for(int j=0; j<_descriptor.Count(); j++)
			{
				if(_descriptor[i].feature == kMeshFeatureIndices)
				{
					_indices = _descriptor[i]._pointer;
					_indicesSize = _descriptor[i].elements * sizeof(uint16);
					continue;
				}
				
				switch(_descriptor[i].type)
				{
					case kMeshFeatureTypeVector2:
					{
						const size_t size = 2 * sizeof(float);
						
						memcpy(&bytes[i], &_descriptor[i]._pointer[_descriptor[i]._offset], size);
						
						_descriptor[i]._offset += size;
						i += size;
						break;
					}
						
					case kMeshFeatureTypeVector3:
					{
						const size_t size = 3 * sizeof(float);
						
						memcpy(&bytes[i], &_descriptor[i]._pointer[_descriptor[i]._offset], size);
						
						_descriptor[i]._offset += size;
						i += size;
						break;
					}
						
					case kMeshFeatureTypeVector4:
					{
						const size_t size = 4 * sizeof(float);
						
						memcpy(&bytes[i], &_descriptor[i]._pointer[_descriptor[i]._offset], size);
						
						_descriptor[i]._offset += size;
						i += size;
						break;
					}
						
					case kMeshFeatureTypeUint16:
					{
						const size_t size = sizeof(uint16);
						
						memcpy(&bytes[i], &_descriptor[i]._pointer[_descriptor[i]._offset], size);
						
						_descriptor[i]._offset += size;
						i += size;
						break;
					}
						
					default:
						break;
				}
			}
		}
		
		
		// Generate the buffers
		if(_vbo == -1)
			glGenBuffers(1, &_vbo);
			
		if(_ivbo == -1)
			glGenBuffers(1, &_ivbo);
		
		
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, _meshSize, _meshData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ivbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, _indices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	
	size_t MeshLODStage::SizeForMeshFeatureType(MeshFeatureType type)
	{
		switch(type)
		{
			case kMeshFeatureTypeVector2:
				return 2 * sizeof(float);
				break;
				
			case kMeshFeatureTypeVector3:
				return 3 * sizeof(float);
				break;
				
			case kMeshFeatureTypeVector4:
				return 4 * sizeof(float);
				
			case kMeshFeatureTypeUint16:
				return sizeof(uint16);
				break;
			
			default:
				return 0;
		}
	}
	
	
	Mesh::Mesh()
	{
	}
	
	Mesh::~Mesh()
	{
		for(int i=0; i<_LODStages.Count(); i++)
		{
			delete _LODStages[i];
		}
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
