//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"
#include "RNKernel.h"
#include "RNDebug.h"
#include "RNRenderer.h"

namespace RN
{
	RNDeclareMeta(Mesh)
	
	// ---------------------
	// MARK: -
	// MARK: MeshDescriptor
	// ---------------------
	
	MeshDescriptor::MeshDescriptor(MeshFeature tfeature, uint32 tflags)
	{
		feature = tfeature;
		flags   = tflags;
		
		elementMember = 0;
		elementSize   = 0;
		
		offset = -1;
		
		_size      = 0;
		_alignment = (flags & DescriptorFlagSIMDAlignment) ? 16 : 8;
	}

	// ---------------------
	// MARK: -
	// MARK: Mesh
	// ---------------------
	
	Mesh::Mesh(const std::vector<MeshDescriptor>& descriptors, size_t verticesCount, size_t indicesCount) :
		_verticesCount(verticesCount),
		_indicesCount(indicesCount)
	{
		Initialize(descriptors);
		AllocateBuffer(std::make_pair(nullptr, nullptr));
	}
	
	Mesh::Mesh(const std::vector<MeshDescriptor>& descriptors, size_t verticesCount, size_t indicesCount, const std::pair<const void *, const void *>& data) :
		_verticesCount(verticesCount),
		_indicesCount(indicesCount)
	{
		Initialize(descriptors);
		AllocateBuffer(data);
		
		if(_verticesCount > 0 && data.first)
			std::copy(reinterpret_cast<const uint8 *>(data.first), reinterpret_cast<const uint8 *>(data.first) + _verticesSize, _vertices);
		
		if(_indicesCount > 0 && data.second)
			std::copy(reinterpret_cast<const uint8 *>(data.second), reinterpret_cast<const uint8 *>(data.second) + _indicesSize, _indices);
	}
	
	Mesh::~Mesh()
	{
		Renderer::GetSharedInstance()->RelinquishMesh(this);
		
		if(_vertices)
			Memory::FreeSIMD(_vertices);
		
		if(_indices)
			Memory::Free(_indices);
		
		gl::DeleteBuffers(2, &_vbo);
	}
	
	
	
	void Mesh::Initialize(const std::vector<MeshDescriptor>& descriptors)
	{
		_indicesSize  = 0;
		_verticesSize = 0;
		
		_indices  = nullptr;
		_vertices = nullptr;
		
		_stride = 0;
		_mode   = GL_TRIANGLES;
		
		_vboUsage = GL_STATIC_DRAW;
		_iboUsage = GL_STATIC_DRAW;
		
		gl::GenBuffers(2, &_vbo);
		RN_CHECKOPENGL();
		
		for(auto& descriptor : descriptors)
		{
			if(!SupportsFeature(descriptor.feature))
			{
				_descriptors.push_back(descriptor);
				_features.insert(descriptor.feature);
			}
		}
		
		
		size_t offset = 0;
		
		for(auto& descriptor : _descriptors)
		{
			if(descriptor.feature != kMeshFeatureIndices)
			{
				descriptor._size  = descriptor.elementSize * _verticesCount;
				descriptor.offset = offset;
				
				offset += descriptor.elementSize;
				
				_verticesSize += descriptor._size;
				_stride += descriptor.elementSize;
			}
			else
			{
				descriptor._size  = descriptor.elementSize * _indicesCount;
				descriptor.offset = 0;
				
				_indicesSize += descriptor._size;
			}
		}
		
		if(_verticesSize > 0)
			_vertices = static_cast<uint8 *>(Memory::AllocateSIMD(_verticesSize));
		
		if(_indicesSize > 0)
			_indices = static_cast<uint8 *>(Memory::Allocate(_indicesSize));
	}
	
	void Mesh::AllocateBuffer(const std::pair<const void *, const void *>& data)
	{
		Renderer::GetSharedInstance()->BindVAO(0);
		
		if(_verticesSize > 0)
		{
			gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);
			gl::BufferData(GL_ARRAY_BUFFER, _verticesSize, data.first, _vboUsage);
		}
		
		if(_indicesCount > 0)
		{
			gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
			gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, data.second, _iboUsage);
		}
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
	
	
	void Mesh::SetElementData(MeshFeature feature, void *tdata)
	{
		for(auto& descriptor : _descriptors)
		{
			if(descriptor.feature == feature)
			{
				Renderer::GetSharedInstance()->BindVAO(0);
				
				switch(feature)
				{
					case kMeshFeatureIndices:
					{
						uint8 *data   = static_cast<uint8 *>(tdata);
						uint8 *buffer = _vertices + descriptor.offset;
						
						GLintptr offset = static_cast<GLintptr>(descriptor.offset);
						
						gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);
						
						for(size_t i = 0; i < _verticesCount; i ++)
						{
							std::copy(data, data + descriptor.elementSize, buffer);
							gl::BufferSubData(GL_ARRAY_BUFFER, offset + (i * _stride), descriptor.elementSize, data);
							
							buffer += _stride;
							data   += descriptor.elementSize;
						}
						
						break;
					}
						
					default:
					{
						uint8 *data = static_cast<uint8 *>(tdata);
						std::copy(data, data + _indicesSize, _indices);
						
						gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
						gl::BufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _indicesSize, data);
						break;
					}
				}
				
				break;
			}
		}
	}
	
	void Mesh::CalculateBoundingVolumes()
	{
		Vector3 min = Vector3();
		Vector3 max = Vector3();
		
		const MeshDescriptor *descriptor = GetDescriptorForFeature(kMeshFeatureVertices);
		
		bool is3D = (descriptor->elementMember == 3);
		uint8 *pointer = _vertices + descriptor->offset;
		
		Vector3 *vertex = reinterpret_cast<Vector3 *>(pointer);
		if(vertex)
		{
			max = min = *vertex;
			
			if(!is3D)
				max.z = min.z = 0.0f;
			
			for(size_t i = 1; i < _verticesCount; i ++)
			{
				pointer += _stride;
				vertex = reinterpret_cast<Vector3 *>(pointer);
				
				min.x = std::min(vertex->x, min.x);
				min.y = std::min(vertex->y, min.y);
				
				max.x = std::max(vertex->x, max.x);
				max.y = std::max(vertex->y, max.y);
				
				if(is3D)
				{
					min.z = std::min(vertex->z, min.z);
					max.z = std::max(vertex->z, max.z);
				}
			}
		}
		
		_boundingBox = AABB(min, max);
		_boundingSphere = Sphere(_boundingBox);
	}
	
	bool Mesh::SupportsFeature(MeshFeature feature) const
	{
		return (_features.find(feature) != _features.end());
	}
	
	const MeshDescriptor *Mesh::GetDescriptorForFeature(MeshFeature feature) const
	{
		for(auto& descriptor : _descriptors)
		{
			if(descriptor.feature == feature)
				return &descriptor;
		}
		
		return nullptr;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Chunk
	// ---------------------
	
	Mesh::Chunk::Chunk(Mesh *mesh, const Range& range, bool indices) :
		_mesh(mesh),
		_range(range),
		_indices(indices),
		_dirty(false)
	{
		if(_indices)
		{
			_stride = _mesh->GetDescriptorForFeature(kMeshFeatureIndices)->elementSize;
			_begin  = _mesh->_indices + (range.origin * _stride);
		}
		else
		{
			_stride = _mesh->GetStride();
			_begin  = _mesh->_vertices + (range.origin * _stride);
		}
	}
	
	void Mesh::Chunk::SetData(const void *data)
	{
		std::memcpy(_begin, data, _range.length * _stride);
		_dirty = true;
	}
	
	void Mesh::Chunk::SetData(const void *data, MeshFeature feature)
	{
		const MeshDescriptor *descriptor = _mesh->GetDescriptorForFeature(feature);
		
		uint8 *temp = reinterpret_cast<uint8 *>(_begin) + descriptor->offset;
		const uint8 *tdata = reinterpret_cast<const uint8 *>(data);
		
		size_t size = descriptor->elementSize;
		
		for(size_t i = 0; i < _range.length; i ++)
		{
			std::copy(tdata, tdata + size, temp);
			
			tdata += size;
			temp  += _stride;
		}
		
		_dirty = true;
	}
	
	void Mesh::Chunk::SetDataInRange(const void *data, const Range& range)
	{
		uint8 *temp = reinterpret_cast<uint8 *>(_begin);
		temp += range.origin * _stride;
		
		std::memcpy(temp, data, range.length * _stride);
		_dirty = true;
	}
	
	void Mesh::Chunk::CommitChanges()
	{
		if(_dirty)
		{
			size_t offset = _range.origin * _stride;
			size_t length = _range.length * _stride;
			
			Renderer::GetSharedInstance()->BindVAO(0);
			
			if(_indices)
			{
				gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh->_ibo);
				gl::BufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, length, _begin);
			}
			else
			{
				gl::BindBuffer(GL_ARRAY_BUFFER, _mesh->_vbo);
				gl::BufferSubData(GL_ARRAY_BUFFER, offset, length, _begin);
			}
			
			_dirty = false;
		}
	}
	
	Mesh::Chunk Mesh::GetChunk()
	{
		return GetChunkForRange(Range(0, _verticesCount));
	}
	
	Mesh::Chunk Mesh::GetChunkForRange(const Range& range)
	{
		return Chunk(this, range, false);
	}
	
	Mesh::Chunk Mesh::GetIndicesChunk()
	{
		return GetIndicesChunkForRange(Range(0, _indicesCount));
	}
	
	Mesh::Chunk Mesh::GetIndicesChunkForRange(const Range& range)
	{
		return Chunk(this, range, true);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Intersection
	// ---------------------
	
	
	/*bool Mesh::CanMergeMesh(Mesh *mesh)
	{
		if(_descriptor.size() != mesh->_descriptor.size() || _stride != mesh->_stride || _mode != mesh->_mode)
			return false;
		
		for(size_t i=0; i!=_descriptor.size(); i++)
		{
			const MeshDescriptor& dA = _descriptor[i];
			const MeshDescriptor& dB = mesh->_descriptor[i];
			
			if(dA.feature != dB.feature || dA.offset != dB.offset)
				return false;
			
			if(dA.elementMember != dB.elementMember || dA.elementSize != dB.elementSize)
				return false;
		
			if(dA._useCount || dB._useCount)
				return false;
		}
		
		return true;
	}
	
	void Mesh::MergeMesh(Mesh *mesh)
	{
		if(!CanMergeMesh(mesh))
			throw Exception(Exception::Type::InconsistencyException, "The meshes cannot be merged!");
		
		if(_meshData)
		{
			uint8 *tdata = static_cast<uint8 *>(Memory::AllocateSIMD(_meshSize + mesh->_meshSize));
			std::copy(_meshData, _meshData + _meshSize, tdata);
			std::copy(mesh->_meshData, mesh->_meshData + mesh->_meshSize, tdata + _meshSize);
			
			Memory::FreeSIMD(_meshData);
			_meshData = tdata;
			_meshSize += mesh->_meshSize;
			
			_dirty = true;
		}
		
		if(_indices)
		{
			uint8 *tdata = static_cast<uint8 *>(Memory::AllocateSIMD(_indicesSize + mesh->_indicesSize));
			std::copy(_indices, _indices + _indicesSize, tdata);
			std::copy(mesh->_indices, mesh->_indices + mesh->_indicesSize, tdata + _indicesSize);
			
			MeshDescriptor *verticesDescriptor = GetDescriptor(kMeshFeatureVertices);
			MeshDescriptor *indicesDescriptor  = GetDescriptor(kMeshFeatureIndices);
			
			switch(indicesDescriptor->elementSize)
			{
				case 2:
				{
					size_t count = mesh->_indicesSize / 2;
					uint16 *indices = reinterpret_cast<uint16 *>(tdata) + indicesDescriptor->elementCount;
					
					for(size_t i=0; i<count; i++)
					{
						*indices += verticesDescriptor->elementCount;
						indices ++;
					}
					
					break;
				}
					
				case 4:
				{
					size_t count = mesh->_indicesSize / 4;
					uint32 *indices = reinterpret_cast<uint32 *>(tdata) + indicesDescriptor->elementCount;
					
					for(size_t i=0; i<count; i++)
					{
						*indices += verticesDescriptor->elementCount;
						indices ++;
					}
					
					break;
				}
					
				default:
					throw Exception(Exception::Type::InconsistencyException, "The indices element size is not supported (uint16 and uint32 support only)");
			}
			
			Memory::FreeSIMD(_indices);
			_indices = tdata;
			_indicesSize += mesh->_indicesSize;
			
			_dirtyIndices = true;
		}
		
		for(size_t i=0; i!=_descriptor.size(); i++)
		{
			MeshDescriptor& dA = _descriptor[i];
			MeshDescriptor& dB = mesh->_descriptor[i];
			
			dA.elementCount += dB.elementCount;
		}
	}*/
	
	

	
	float Mesh::RayTriangleIntersection(const Vector3 &pos, const Vector3 &dir, const Vector3 &vert1, const Vector3 &vert2, const Vector3 &vert3, Hit::HitMode mode)
	{
		float u, v;
		Vector3 edge1, edge2, tvec, pvec, qvec;
		float det, inv_det;
		
		edge1 = vert2-vert1;
		edge2 = vert3-vert1;
		
		if(mode != Hit::HitMode::IgnoreNone)
		{
			float facing = edge1.Cross(edge2).Dot(dir);
			if(mode == Hit::HitMode::IgnoreBackfaces)
			{
				if(facing > 0.0f)
					return -1.0f;
			}
			else
			{
				if(facing < 0.0f)
					return -1.0f;
			}
		}
		
		pvec = dir.Cross(edge2);
		det = pvec.Dot(edge1);
		
		if(det > -k::EpsilonFloat && det < k::EpsilonFloat)
			return -1.0f;
		
		inv_det = 1.0f/det;
		
		tvec = pos-vert1;
		u = tvec.Dot(pvec)*inv_det;
		
		if(u < 0.0f || u > 1.0f)
			return -1.0f;
		
		qvec = tvec.Cross(edge1);
		v = dir.Dot(qvec)*inv_det;
		
		if(v < 0.0f || u+v > 1.0f)
			return -1.0f;
		
		//distance
		float t = edge2.Dot(qvec)*inv_det;
		return t;
	}
	
	Hit Mesh::IntersectsRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		const MeshDescriptor *posdescriptor = GetDescriptorForFeature(kMeshFeatureVertices);
		const MeshDescriptor *inddescriptor = GetDescriptorForFeature(kMeshFeatureIndices);
		
		switch(posdescriptor->elementMember)
		{
			case 2:
				if(inddescriptor)
					return IntersectsRay2DWithIndices(posdescriptor, inddescriptor, position, direction, mode);
				else
					return IntersectsRay2DWithoutIndices(posdescriptor, position, direction, mode);
				
			case 3:
				if(inddescriptor)
					return IntersectsRay3DWithIndices(posdescriptor, inddescriptor, position, direction, mode);
				else
					return IntersectsRay3DWithoutIndices(posdescriptor, position, direction, mode);
		}
		
		Hit hit;
		return hit;
	}
	
	Hit Mesh::IntersectsRay3DWithIndices(const MeshDescriptor *positionDescriptor, const MeshDescriptor *indicesDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		uint8 *pospointer = _vertices + positionDescriptor->offset;
		uint8 *indpointer = _indices  + indicesDescriptor->offset;
		
		for(size_t i = 0; i < _indicesCount; i += 3)
		{
			Vector3 *vertex1;
			Vector3 *vertex2;
			Vector3 *vertex3;
			
			switch(indicesDescriptor->elementSize)
			{
				case 1:
					vertex1 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*indpointer ++));
					vertex2 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*indpointer ++));
					vertex3 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*indpointer ++));
					break;
					
				case 2:
				{
					uint16 *index = reinterpret_cast<uint16 *>(indpointer + i * indicesDescriptor->elementSize);
					
					vertex1 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*index ++));
					vertex2 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*index ++));
					vertex3 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*index ++));
					break;
				}
					
				case 4:
				{
					uint32 *index = reinterpret_cast<uint32 *>(indpointer + i * indicesDescriptor->elementSize);
					
					vertex1 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*index ++));
					vertex2 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*index ++));
					vertex3 = reinterpret_cast<Vector3 *>(pospointer + _stride * (*index ++));
					break;
				}
			}
			
			float result = RayTriangleIntersection(position, direction, *vertex1, *vertex2, *vertex3, mode);
			
			if(result >= 0.0f)
			{
				if(hit.distance < 0.0f)
				{
					hit.distance = result;
				}
				
				if(result < hit.distance)
				{
					hit.distance = result;
				}
			}
		}
		
		hit.position = position+direction*hit.distance;
		
		return hit;
	}

	
	Hit Mesh::IntersectsRay2DWithIndices(const MeshDescriptor *positionDescriptor, const MeshDescriptor *indicesDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		uint8 *pospointer = _vertices + positionDescriptor->offset;
		uint8 *indpointer = _indices + indicesDescriptor->offset;
		
		for(size_t i = 0; i < _indicesCount; i += 3)
		{
			Vector2 *vertex1;
			Vector2 *vertex2;
			Vector2 *vertex3;
			
			switch(indicesDescriptor->elementSize)
			{
				case 1:
					vertex1 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*indpointer ++));
					vertex2 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*indpointer ++));
					vertex3 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*indpointer ++));
					break;
					
				case 2:
				{
					uint16 *index = reinterpret_cast<uint16 *>(indpointer + i * indicesDescriptor->elementSize);
					
					vertex1 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*index ++));
					vertex2 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*index ++));
					vertex3 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*index ++));
					break;
				}
					
				case 4:
				{
					uint32 *index = reinterpret_cast<uint32 *>(indpointer + i * indicesDescriptor->elementSize);
					
					vertex1 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*index ++));
					vertex2 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*index ++));
					vertex3 = reinterpret_cast<Vector2 *>(pospointer + _stride * (*index ++));
					break;
				}
			}
			
			float result = RayTriangleIntersection(position, direction, Vector3(*vertex1, 0.0f), Vector3(*vertex2, 0.0f), Vector3(*vertex3, 0.0f), mode);
			
			if(result >= 0.0f)
			{
				if(hit.distance < 0.0f)
				{
					hit.distance = result;
				}
				
				if(result < hit.distance)
				{
					hit.distance = result;
				}
			}
		}
		
		hit.position = position+direction*hit.distance;
		
		return hit;
	}
	
	Hit Mesh::IntersectsRay3DWithoutIndices(const MeshDescriptor *positionDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		uint8 *pospointer = _vertices + positionDescriptor->offset;
		int trioffset = (GetMode() != GL_TRIANGLE_STRIP) ? 3 : 1;
		
		for(size_t i = 0; i < _indicesCount - 2; i += trioffset)
		{
			Vector3 *vertex1;
			Vector3 *vertex2;
			Vector3 *vertex3;
			
			vertex1 = reinterpret_cast<Vector3 *>(pospointer + _stride * (i+2*(i%2)));
			vertex2 = reinterpret_cast<Vector3 *>(pospointer + _stride * (i+1));
			vertex3 = reinterpret_cast<Vector3 *>(pospointer + _stride * (i+2*((i+1)%2)));
			
			float result = RayTriangleIntersection(position, direction, *vertex1, *vertex2, *vertex3, mode);
			
			if(result >= 0.0f)
			{
				if(hit.distance < 0.0f)
				{
					hit.distance = result;
				}
				
				if(result < hit.distance)
				{
					hit.distance = result;
				}
			}
		}
		
		hit.position = position+direction*hit.distance;
		
		return hit;
	}
	
	
	Hit Mesh::IntersectsRay2DWithoutIndices(const MeshDescriptor *positionDescriptor, const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		uint8 *pospointer = _vertices + positionDescriptor->offset;
		int trioffset = (GetMode() != GL_TRIANGLE_STRIP) ? 3 : 1;

		for(size_t i = 0; i < _indicesCount - 2; i += trioffset)
		{
			Vector2 *vertex1;
			Vector2 *vertex2;
			Vector2 *vertex3;
			
			vertex1 = reinterpret_cast<Vector2 *>(pospointer + _stride * (i+2*(i%2)));
			vertex2 = reinterpret_cast<Vector2 *>(pospointer + _stride * (i+1));
			vertex3 = reinterpret_cast<Vector2 *>(pospointer + _stride * (i+2*((i+1)%2)));
			
			float result = RayTriangleIntersection(position, direction, Vector3(*vertex1, 0.0f), Vector3(*vertex2, 0.0f), Vector3(*vertex3, 0.0f), mode);
			
			if(result >= 0.0f)
			{
				if(hit.distance < 0.0f)
				{
					hit.distance = result;
				}
				
				if(result < hit.distance)
				{
					hit.distance = result;
				}
			}
		}
		
		hit.position = position+direction*hit.distance;
		
		return hit;
	}

	
	
	
	Mesh *Mesh::PlaneMesh(const Vector3& size, const Vector3& rotation)
	{
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor texcoordDescriptor(kMeshFeatureUVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		
		MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptors = { vertexDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptors, 4, 6);
		
		Matrix rotmat;
		rotmat.MakeRotate(rotation);
		
		Chunk chunk = mesh->GetChunk();
		Chunk ichunk = mesh->GetIndicesChunk();
		
		ElementIterator<Vector3> vertices  = chunk.GetIterator<Vector3>(kMeshFeatureVertices);
		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(kMeshFeatureUVSet0);
		ElementIterator<uint16> indices    = ichunk.GetIterator<uint16>(kMeshFeatureIndices);
		
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
		
		chunk.CommitChanges();
		ichunk.CommitChanges();
		
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size)
	{
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor normalDescriptor(kMeshFeatureNormals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		
		MeshDescriptor texcoordDescriptor(kMeshFeatureUVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		
		MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, normalDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor, 24, 36);
		
		
		Chunk chunk = mesh->GetChunk();
		Chunk ichunk = mesh->GetIndicesChunk();
		
		ElementIterator<Vector3> vertices  = chunk.GetIterator<Vector3>(kMeshFeatureVertices);
		ElementIterator<Vector3> normals   = chunk.GetIterator<Vector3>(kMeshFeatureNormals);
		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(kMeshFeatureUVSet0);
		ElementIterator<uint16> indices    = ichunk.GetIterator<uint16>(kMeshFeatureIndices);
		
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
		
		chunk.CommitChanges();
		ichunk.CommitChanges();
		
		return mesh;
	}
	
	Mesh *Mesh::CubeMesh(const Vector3& size, const Color& color)
	{
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor normalDescriptor(kMeshFeatureNormals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		
		MeshDescriptor colorDescriptor(kMeshFeatureColor0);
		colorDescriptor.elementSize = sizeof(Color);
		colorDescriptor.elementMember = 4;
		
		MeshDescriptor texcoordDescriptor(kMeshFeatureUVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		
		MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, normalDescriptor, colorDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor, 24, 36);

		Chunk chunk = mesh->GetChunk();
		Chunk ichunk = mesh->GetIndicesChunk();
		
		ElementIterator<Vector3> vertices  = chunk.GetIterator<Vector3>(kMeshFeatureVertices);
		ElementIterator<Vector3> normals   = chunk.GetIterator<Vector3>(kMeshFeatureNormals);
		ElementIterator<Color>   colors    = chunk.GetIterator<Color>(kMeshFeatureColor0);
		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(kMeshFeatureUVSet0);
		ElementIterator<uint16> indices    = ichunk.GetIterator<uint16>(kMeshFeatureIndices);
		
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
		
		chunk.CommitChanges();
		ichunk.CommitChanges();
		
		return mesh;
	}
}
