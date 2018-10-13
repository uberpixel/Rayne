//
//  RNODEShape.cpp
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNODEShape.h"

namespace RN
{
	RNDefineMeta(ODEShape, Object)
	RNDefineMeta(ODESphereShape, ODEShape)
	RNDefineMeta(ODEBoxShape, ODEShape)
	RNDefineMeta(ODEStaticPlaneShape, ODEShape)
	RNDefineMeta(ODETriangleMeshShape, ODEShape)
//	RNDefineMeta(ODEConvexHullShape, ODEShape)
	RNDefineMeta(ODECompoundShape, ODEShape)
		
	ODEShape::ODEShape()
	{
		
	}
	
	ODEShape::~ODEShape()
	{

	}
		
		
		
	ODESphereShape::ODESphereShape(float radius)
	{
		_radius = radius;
	}
		
	ODESphereShape *ODESphereShape::WithRadius(float radius)
	{
		ODESphereShape *shape = new ODESphereShape(radius);
		return shape->Autorelease();
	}

		
	ODEBoxShape::ODEBoxShape(const Vector3 &halfExtents)
	{
		_halfExtents = halfExtents;
	}
		
	ODEBoxShape *ODEBoxShape::WithHalfExtents(const Vector3 &halfExtents)
	{
		ODEBoxShape *shape = new ODEBoxShape(halfExtents);
		return shape->Autorelease();
	}
		
		
	ODEStaticPlaneShape::ODEStaticPlaneShape(const Vector3 &normal, float constant)
	{
		_plane = RN::Vector4(normal, constant);
	}
		
	ODEStaticPlaneShape *ODEStaticPlaneShape::WithNormal(const Vector3 &normal, float constant)
	{
		ODEStaticPlaneShape *shape = new ODEStaticPlaneShape(normal, constant);
		return shape->Autorelease();
	}
		
		
	ODETriangleMeshShape::ODETriangleMeshShape(Model *model, Vector3 scale)
	{
		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for(size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			AddMesh(mesh, scale);
		}
	}
		
	ODETriangleMeshShape::ODETriangleMeshShape(Mesh *mesh, Vector3 scale)
	{
		AddMesh(mesh, scale);
	}
		
	ODETriangleMeshShape::ODETriangleMeshShape(const Array *meshes, Vector3 scale)
	{
		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			AddMesh(mesh, scale);
		});
	}
		
	ODETriangleMeshShape::~ODETriangleMeshShape()
	{
		
	}
		
	ODETriangleMeshShape *ODETriangleMeshShape::WithModel(Model *model, Vector3 scale)
	{
		ODETriangleMeshShape *shape = new ODETriangleMeshShape(model, scale);
		return shape->Autorelease();
	}
		
	void ODETriangleMeshShape::AddMesh(Mesh *mesh, Vector3 scale)
	{
		//TODO: Use btTriangleIndexVertexArray which reuses existing indexed vertex data and should be a lot faster to create
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		const Mesh::VertexAttribute *indexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices);
		RN_ASSERT(vertexAttribute && vertexAttribute->GetType() == PrimitiveType::Vector3, "Mesh needs to have vertices of Vector3!");
		RN_ASSERT(indexAttribute, "Mesh needs indices!");

/*		physx::PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = mesh->GetVerticesCount();
		meshDesc.points.stride = mesh->GetStride();
		meshDesc.points.data = mesh->GetCPUVertexBuffer();

		meshDesc.triangles.count = mesh->GetIndicesCount() / 3;
		meshDesc.triangles.stride = indexAttribute->GetSize() * 3;
		meshDesc.triangles.data = mesh->GetCPUIndicesBuffer();
		if(indexAttribute->GetSize() == 2)
			meshDesc.flags = physx::PxMeshFlag::e16_BIT_INDICES;*/

		_vertices.reserve(_vertices.size() + mesh->GetVerticesCount());
		_indices.reserve(_indices.size() + mesh->GetIndicesCount());
		_normals.reserve(_normals.size() + mesh->GetIndicesCount() / 3);

		Mesh::Chunk chunk = mesh->GetChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		uint32 indexOffset = _vertices.size();
		for(size_t i = 0; i < mesh->GetVerticesCount(); i++)
		{
			if(i > 0) vertexIterator++;
			const Vector3 &vertex = *vertexIterator;
			_vertices.push_back(vertex * scale);
		}

		if(indexAttribute->GetSize() == 2)
		{
			for(size_t i = 0; i < mesh->GetIndicesCount(); i++)
			{
				uint32 index = indexOffset + static_cast<uint32>(static_cast<uint16*>(mesh->GetCPUIndicesBuffer())[i]);
				_indices.push_back(index);
			}
		}
		else if(indexAttribute->GetSize() == 4)
		{
			for(size_t i = 0; i < mesh->GetIndicesCount(); i++)
			{
				uint32 index = indexOffset + static_cast<uint32*>(mesh->GetCPUIndicesBuffer())[i];
				_indices.push_back(index);
			}
		}
		

		Mesh::Chunk trichunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> normalsIterator = trichunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Normals);

		for(size_t i = 0; i < mesh->GetIndicesCount()/3; i++)
		{
			if(i > 0) normalsIterator++;
			const Vector3 &normal1 = *(normalsIterator++);
			const Vector3 &normal2 = *(normalsIterator++);
			const Vector3 &normal3 = *normalsIterator;

			Vector3 faceNormal = normal1 + normal2 + normal3;
			faceNormal.Normalize();
			_normals.push_back(faceNormal);
		}
	}


/*	ODEConvexHullShape::ODEConvexHullShape(Mesh *mesh)
	{
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if (!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}

		Mesh::Chunk chunk = mesh->GetChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		std::vector<float> vertices;
		vertices.reserve(mesh->GetVerticesCount() * 3);
		for(size_t i = 0; i < mesh->GetVerticesCount(); i++)
		{
			if(i > 0)
			{
				vertexIterator++;
			}
			const Vector3 &vertex = *vertexIterator;
			vertices.push_back(vertex.x);
			vertices.push_back(vertex.y);
			vertices.push_back(vertex.z);
		}

/*		btConvexHullComputer *convexHullComputer = new btConvexHullComputer();
		btScalar actualMargin = convexHullComputer->compute(&vertices[0], 3 * sizeof(float), mesh->GetVerticesCount(), margin, 0.001f);
		
		btConvexHullShape *shape = new btConvexHullShape(convexHullComputer->vertices[0].m_floats, convexHullComputer->vertices.size());
		shape->setMargin(actualMargin);
		shape->recalcLocalAabb();

		delete convexHullComputer;
		_shape = shape;*/
/*	}

	ODEConvexHullShape *ODEConvexHullShape::WithMesh(Mesh *mesh)
	{
		ODEConvexHullShape *shape = new ODEConvexHullShape(mesh);
		return shape->Autorelease();
	}
	*/

	
	ODECompoundShape::ODECompoundShape()
	{

	}
		
	ODECompoundShape::~ODECompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void ODECompoundShape::AddChild(ODEShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		_shapes.push_back(shape->Retain());
	}
}
