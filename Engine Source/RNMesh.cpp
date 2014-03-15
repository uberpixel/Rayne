//
//  RNMesh.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMesh.h"
#include "RNKernel.h"
#include "RNDebug.h"
#include "RNRenderer.h"
#include "RNOpenGLQueue.h"

namespace RN
{
	RNDefineMeta(Mesh, Object)
	
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
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteBuffers(2, &_vbo);
		}, true);
	}
	
	void Mesh::PushData(bool vertices, bool indices)
	{
		if(vertices)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
				Renderer::GetSharedInstance()->BindVAO(0);
				
				gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);
				gl::BufferData(GL_ARRAY_BUFFER, _verticesSize, _vertices, static_cast<GLenum>(_vboUsage));
			});
		}
		
		if(indices)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
				Renderer::GetSharedInstance()->BindVAO(0);
				
				gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
				gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, _indices, static_cast<GLenum>(_iboUsage));
			});
		}
	}
	
	
	void Mesh::Initialize(const std::vector<MeshDescriptor>& descriptors)
	{
		_indicesSize   = 0;
		_verticesSize  = 0;
		
		_indices  = nullptr;
		_vertices = nullptr;
		
		_stride = 0;
		_mode   = DrawMode::Triangles;
		
		_vboUsage = MeshUsage::Static;
		_iboUsage = MeshUsage::Static;
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenBuffers(2, &_vbo);
		});
		
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
			if(descriptor.feature != MeshFeature::Indices)
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
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			Renderer::GetSharedInstance()->BindVAO(0);
			
			if(_verticesSize > 0)
			{
				gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);
				gl::BufferData(GL_ARRAY_BUFFER, _verticesSize, data.first, static_cast<GLenum>(_vboUsage));
			}
			
			if(_indicesCount > 0)
			{
				gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
				gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, _indicesSize, data.second, static_cast<GLenum>(_iboUsage));
			}
		}, true);
	}
		
	
	void Mesh::SetDrawMode(DrawMode mode)
	{
		_mode = mode;
	}
	
	void Mesh::SetVBOUsage(MeshUsage usage)
	{
		_vboUsage = usage;
	}
	
	void Mesh::SetIBOUsage(MeshUsage usage)
	{
		_iboUsage = usage;
	}
	
	
	void Mesh::SetElementData(MeshFeature feature, void *tdata)
	{
		if(feature == MeshFeature::Indices)
		{
			uint8 *data = static_cast<uint8 *>(tdata);
			std::copy(data, data + _indicesSize, _indices);
			
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
				gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
				gl::BufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _indicesSize, _indices);
			});
		}
		else
		{
			for(auto& descriptor : _descriptors)
			{
				if(descriptor.feature == feature)
				{
					uint8 *data   = static_cast<uint8 *>(tdata);
					uint8 *buffer = _vertices + descriptor.offset;
					
					for(size_t i = 0; i < _verticesCount; i ++)
					{
						std::copy(data, data + descriptor.elementSize, buffer);
														
						buffer += _stride;
						data   += descriptor.elementSize;
					}
					
					OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
						Renderer::GetSharedInstance()->BindVAO(0);
						
						gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);
						gl::BufferSubData(GL_ARRAY_BUFFER, 0, _verticesSize, _vertices);
					});
					
					break;
				}
			}
		}
	}
	
	void Mesh::SetVerticesCount(size_t count)
	{
		if(_verticesCount == count)
			return;
		
		uint8 *temp = static_cast<uint8 *>(Memory::AllocateSIMD(count * _stride));
		size_t copy = std::min(count * _stride, _verticesSize);
		
		std::copy(_vertices, _vertices + copy, temp);
		
		Memory::FreeSIMD(_vertices);
		
		_vertices = temp;
		_verticesCount = count;
		_verticesSize  = count * _stride;
		
		PushData(true, false);
	}
	
	void Mesh::SetIndicesCount(size_t count)
	{
		if(_indicesCount == count)
			return;
		
		const MeshDescriptor *descriptor = GetDescriptorForFeature(MeshFeature::Indices);
		
		uint8 *temp = static_cast<uint8 *>(Memory::Allocate(count * descriptor->elementSize));
		size_t copy = std::min(count * descriptor->elementSize, _indicesCount);
		
		std::copy(_indices, _indices + copy, temp);
		
		Memory::FreeSIMD(_indices);
		
		_indices = temp;
		_indicesCount = count;
		_indicesSize  = count * descriptor->elementSize;
		
		PushData(false, true);
	}
	
	void Mesh::CalculateBoundingVolumes()
	{
		Vector3 min = Vector3();
		Vector3 max = Vector3();
		
		const MeshDescriptor *descriptor = GetDescriptorForFeature(MeshFeature::Vertices);
		
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
			_stride = _mesh->GetDescriptorForFeature(MeshFeature::Indices)->elementSize;
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
			
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
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
			}, true);
			
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
	
	Mesh::Chunk Mesh::InsertChunk(size_t offset, size_t elements)
	{
		size_t size = elements * _stride;
		size_t offsetSize = offset * _stride;
		
		uint8 *temp = static_cast<uint8 *>(Memory::AllocateSIMD(_verticesSize + size));
		
		std::copy(_vertices, _vertices + offsetSize, temp);
		std::copy(_vertices + offset, _vertices + (_verticesSize - offset), temp + offsetSize + size);
		
		Memory::FreeSIMD(temp);
		
		_vertices = temp;
		_verticesCount += elements;
		_verticesSize  += size;
		
		PushData(true, false);
		return Chunk(this, Range(offset, elements), false);
	}
	
	void Mesh::DeleteChunk(size_t offset, size_t elements)
	{
		size_t size = elements * _stride;
		size_t offsetSize = offset * _stride;
		
		uint8 *temp = static_cast<uint8 *>(Memory::AllocateSIMD(_verticesSize - size));
		
		std::copy(_vertices, _vertices + offsetSize, temp);
		std::copy(_vertices + offset + size, _vertices + (_verticesSize - offset), temp + offset);
		
		Memory::FreeSIMD(_vertices);
		
		_vertices = temp;
		_verticesCount -= elements;
		_verticesSize  -= size;
		
		PushData(true, false);
	}
	
	
	Mesh::Chunk Mesh::InsertIndicesChunk(size_t offset, size_t elements)
	{
		const MeshDescriptor *descriptor = GetDescriptorForFeature(MeshFeature::Indices);
		
		size_t size = elements * descriptor->elementSize;
		size_t offsetSize = offset * descriptor->elementSize;
		
		uint8 *temp = static_cast<uint8 *>(Memory::Allocate(_indicesSize + size));
		
		std::copy(_indices, _indices + offsetSize, temp);
		std::copy(_indices + offset, _indices + (_indicesSize - offset), temp + offsetSize + size);
		
		Memory::Free(_indices);
		
		_indices = temp;
		_indicesCount += elements;
		_indicesSize  += size;
		
		
		PushData(false, true);
		return Chunk(this, Range(offset, elements), true);
	}
	
	void Mesh::DeleteIndicesChunk(size_t offset, size_t elements)
	{
		const MeshDescriptor *descriptor = GetDescriptorForFeature(MeshFeature::Indices);
		
		size_t size = elements * descriptor->elementSize;
		size_t offsetSize = offset * descriptor->elementSize;
		
		uint8 *temp = static_cast<uint8 *>(Memory::Allocate(_indicesSize - size));
		
		std::copy(_indices, _indices + offsetSize, temp);
		std::copy(_indices + offset + size, _indices + (_indicesSize - offset), temp + offset);
		
		Memory::Free(_indices);
		
		_indices = temp;
		_indicesCount -= elements;
		_indicesSize  -= size;
		
		PushData(false, true);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Postprocessing
	// ---------------------
	
	void Mesh::GenerateTangents()
	{
		RN_ASSERT(SupportsFeature(MeshFeature::Vertices), "Tangent generation needs vertex positions!");
		RN_ASSERT(SupportsFeature(MeshFeature::Normals), "Tangent generation needs vertex normals!");
		RN_ASSERT(SupportsFeature(MeshFeature::UVSet0), "Tangent generation needs first vertex uv set!");
		RN_ASSERT(SupportsFeature(MeshFeature::UVSet0), "Tangent generation needs vertex tangents to replace!");
		
		const MeshDescriptor *inddescriptor = GetDescriptorForFeature(MeshFeature::Indices);
		uint8 *indpointer = _indices;
		Chunk chunk = GetChunk();
		
		Vector3 *tan1 = new Vector3[_verticesCount * 2];
		Vector3 *tan2 = tan1 + _verticesCount;
		
		size_t indicescount = _indicesCount;
		if(!SupportsFeature(MeshFeature::Indices))
		{
			indicescount = _verticesCount;
		}
		
		for(size_t a = 0; a < indicescount; a += 3)
		{
			size_t i1, i2, i3;
			
			if(inddescriptor)
			{
				switch(inddescriptor->elementSize)
				{
					case 1:
					{
						i1 = (*indpointer ++);
						i2 = (*indpointer ++);
						i3 = (*indpointer ++);
						break;
					}
					
					case 2:
					{
						uint16 *index = reinterpret_cast<uint16 *>(indpointer + a * inddescriptor->elementSize);
						
						i1 = (*index ++);
						i2 = (*index ++);
						i3 = (*index ++);
						break;
					}
					
					case 4:
					{
						uint32 *index = reinterpret_cast<uint32 *>(indpointer + a * inddescriptor->elementSize);
						
						i1 = (*index ++);
						i2 = (*index ++);
						i3 = (*index ++);
						break;
					}
				}
			}
			else
			{
				i1 = a;
				i2 = a+1;
				i3 = a+2;
			}
			
			Vector3 v1 = *chunk.GetIteratorAtIndex<Vector3>(MeshFeature::Vertices, i1);
			Vector3 v2 = *chunk.GetIteratorAtIndex<Vector3>(MeshFeature::Vertices, i2);
			Vector3 v3 = *chunk.GetIteratorAtIndex<Vector3>(MeshFeature::Vertices, i3);
			
			Vector2 w1 = *chunk.GetIteratorAtIndex<Vector2>(MeshFeature::UVSet0, i1);
			Vector2 w2 = *chunk.GetIteratorAtIndex<Vector2>(MeshFeature::UVSet0, i2);
			Vector2 w3 = *chunk.GetIteratorAtIndex<Vector2>(MeshFeature::UVSet0, i3);
			
			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;
			
			float s1 = w2.x - w1.x;
			float s2 = w3.x - w1.x;
			float t1 = w2.y - w1.y;
			float t2 = w3.y - w1.y;
			
			float r = 1.0F / (s1 * t2 - s2 * t1);
			Vector3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
						  (t2 * z1 - t1 * z2) * r);
			Vector3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
						  (s1 * z2 - s2 * z1) * r);
			
			tan1[i1] += sdir;
			tan1[i2] += sdir;
			tan1[i3] += sdir;
			
			tan2[i1] += tdir;
			tan2[i2] += tdir;
			tan2[i3] += tdir;
		}
		
		for(size_t a = 0; a < _verticesCount; a++)
		{
			Vector3 n = *chunk.GetIteratorAtIndex<Vector3>(MeshFeature::Normals, a);
			const Vector3 &t = tan1[a];
			
			auto tangent = chunk.GetIteratorAtIndex<Vector4>(MeshFeature::Tangents, a);
			// Gram-Schmidt orthogonalize
			*tangent = Vector4((t - n * n.GetDotProduct(t)).GetNormalized());
			
			// Calculate handedness
			(*tangent).w = (n.GetCrossProduct(t).GetDotProduct(tan2[a]) < 0.0f) ? -1.0f : 1.0f;
		}
		
		delete[] tan1;
		
		chunk.CommitChanges();
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Intersection
	// ---------------------
	
	float Mesh::RayTriangleIntersection(const Vector3 &pos, const Vector3 &dir, const Vector3 &vert1, const Vector3 &vert2, const Vector3 &vert3, Hit::HitMode mode)
	{
		float u, v;
		Vector3 edge1, edge2, tvec, pvec, qvec;
		float det, inv_det;
		
		edge1 = vert2-vert1;
		edge2 = vert3-vert1;
		
		if(mode != Hit::HitMode::IgnoreNone)
		{
			float facing = edge1.GetCrossProduct(edge2).GetDotProduct(dir);
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
		
		pvec = dir.GetCrossProduct(edge2);
		det = pvec.GetDotProduct(edge1);
		
		if(det > -k::EpsilonFloat && det < k::EpsilonFloat)
			return -1.0f;
		
		inv_det = 1.0f/det;
		
		tvec = pos-vert1;
		u = tvec.GetDotProduct(pvec)*inv_det;
		
		if(u < 0.0f || u > 1.0f)
			return -1.0f;
		
		qvec = tvec.GetCrossProduct(edge1);
		v = dir.GetDotProduct(qvec)*inv_det;
		
		if(v < 0.0f || u+v > 1.0f)
			return -1.0f;
		
		//distance
		float t = edge2.GetDotProduct(qvec)*inv_det;
		return t;
	}
	
	Hit Mesh::IntersectsRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		const MeshDescriptor *posdescriptor = GetDescriptorForFeature(MeshFeature::Vertices);
		const MeshDescriptor *inddescriptor = GetDescriptorForFeature(MeshFeature::Indices);
		
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
		int trioffset = (_mode != DrawMode::TriangleStrip) ? 3 : 1;
		
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
		int trioffset = (_mode != DrawMode::Triangles) ? 3 : 1;

		for(size_t i = 0; i < _verticesCount - 2; i += trioffset)
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
		MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor texcoordDescriptor(MeshFeature::UVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		
		MeshDescriptor indicesDescriptor(MeshFeature::Indices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptors = { vertexDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptors, 4, 6);
		
		Matrix rotmat = Matrix::WithRotation(rotation);
		
		Chunk chunk = mesh->GetChunk();
		Chunk ichunk = mesh->GetIndicesChunk();
		
		ElementIterator<Vector3> vertices  = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
		ElementIterator<uint16> indices    = ichunk.GetIterator<uint16>(MeshFeature::Indices);
		
		*vertices ++ = rotmat * Vector3(-size.x, size.y, -size.z);
		*vertices ++ = rotmat * Vector3( size.x, size.y, -size.z);
		*vertices ++ = rotmat * Vector3( size.x, size.y, size.z);
		*vertices ++ = rotmat * Vector3(-size.x, size.y, size.z);
		
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
		MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor normalDescriptor(MeshFeature::Normals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		
		MeshDescriptor texcoordDescriptor(MeshFeature::UVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		
		MeshDescriptor indicesDescriptor(MeshFeature::Indices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, normalDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor, 24, 36);
		
		
		Chunk chunk = mesh->GetChunk();
		Chunk ichunk = mesh->GetIndicesChunk();
		
		ElementIterator<Vector3> vertices  = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
		ElementIterator<Vector3> normals   = chunk.GetIterator<Vector3>(MeshFeature::Normals);
		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
		ElementIterator<uint16> indices    = ichunk.GetIterator<uint16>(MeshFeature::Indices);
		
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
		MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor normalDescriptor(MeshFeature::Normals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
		
		MeshDescriptor colorDescriptor(MeshFeature::Color0);
		colorDescriptor.elementSize = sizeof(Color);
		colorDescriptor.elementMember = 4;
		
		MeshDescriptor texcoordDescriptor(MeshFeature::UVSet0);
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		
		MeshDescriptor indicesDescriptor(MeshFeature::Indices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, normalDescriptor, colorDescriptor, indicesDescriptor, texcoordDescriptor };
		Mesh *mesh = new Mesh(descriptor, 24, 36);

		Chunk chunk = mesh->GetChunk();
		Chunk ichunk = mesh->GetIndicesChunk();
		
		ElementIterator<Vector3> vertices  = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
		ElementIterator<Vector3> normals   = chunk.GetIterator<Vector3>(MeshFeature::Normals);
		ElementIterator<Color>   colors    = chunk.GetIterator<Color>(MeshFeature::Color0);
		ElementIterator<Vector2> texcoords = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
		ElementIterator<uint16> indices    = ichunk.GetIterator<uint16>(MeshFeature::Indices);
		
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
	
	Mesh *Mesh::SphereMesh(float radius, size_t slices, size_t segments)
	{
		struct Vertex
		{
			Vertex()
			{}
			
			Vertex(float tx, float ty, float tz)
			{
				x = tx;
				y = ty;
				z = tz;
			}
			
			float x, y, z;
		};
		
		std::vector<Vertex> vertices;
		std::vector<uint16> indices;
		
		for(size_t i = 0; i < segments; i ++)
		{
			for(size_t j = 0; j < slices; j ++)
			{
				float theta = float(i) / (segments - 1) * (k::Pi);
				float phi   = float(j) / (slices - 1)   * (k::Pi * 2);
				
				Vertex v;
				v.x = radius *  Math::Sin(theta) * Math::Cos(phi);
				v.y = radius * -Math::Sin(theta) * Math::Sin(phi);
				v.z = radius *  Math::Cos(theta);
				
				vertices.push_back(std::move(v));
			}
		}
		
		for(size_t i = 0; i < segments - 3; i ++)
		{
			for(size_t j = 0; j < slices - 1; j ++)
			{
				indices.push_back(i * slices + j);
				indices.push_back((i + 1) * slices + j + 1);
				indices.push_back(i * slices + j + 1);
				
				indices.push_back(i * slices + j);
				indices.push_back((i + 1) * slices + j);
				indices.push_back((i + 1) * slices + j + 1);
			}
		}
		
		for(size_t i = 0; i < slices - 1; i ++)
		{
			indices.push_back((segments - 2) * slices);
			indices.push_back(i);
			indices.push_back(i + 2);
			
			indices.push_back((segments - 2) * slices + 1);
			indices.push_back((segments - 3) * slices + i + 1);
			indices.push_back((segments - 3) * slices + i);
		}
		
		
		
		MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		
		MeshDescriptor normalDescriptor(MeshFeature::Normals);
		normalDescriptor.elementSize = sizeof(Vector3);
		normalDescriptor.elementMember = 3;
							  
		MeshDescriptor indicesDescriptor(MeshFeature::Indices);
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		
		std::vector<MeshDescriptor> descriptor = { vertexDescriptor, indicesDescriptor };
		Mesh *mesh = new Mesh(descriptor, vertices.size(), indices.size());
		
		Chunk chunk = mesh->GetChunk();
		chunk.SetData(vertices.data());
		chunk.CommitChanges();
		
		Chunk ichunk = mesh->GetIndicesChunk();
		ichunk.SetData(indices.data());
		ichunk.CommitChanges();
							  
		return mesh->Autorelease();
	}
}
