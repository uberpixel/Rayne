//
//  RNPhysXShape.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXShape.h"
#include "PxPhysicsAPI.h"
#include "RNPhysXWorld.h"

namespace RN
{
	RNDefineMeta(PhysXShape, Object)
	RNDefineMeta(PhysXSphereShape, PhysXShape)
	RNDefineMeta(PhysXBoxShape, PhysXShape)
	RNDefineMeta(PhysXCapsuleShape, PhysXShape)
	RNDefineMeta(PhysXStaticPlaneShape, PhysXShape)
	RNDefineMeta(PhysXTriangleMeshShape, PhysXShape)
	RNDefineMeta(PhysXConvexHullShape, PhysXShape)
	RNDefineMeta(PhysXCompoundShape, PhysXShape)
		
	PhysXShape::PhysXShape() :
		_shape(nullptr)
	{}
		
	PhysXShape::PhysXShape(physx::PxShape *shape) :
		_shape(shape)
	{}
		
	PhysXShape::~PhysXShape()
	{
		_shape->release();
	}
		
		
		
	PhysXSphereShape::PhysXSphereShape(float radius, PhysXMaterial *material)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_shape = physics->createShape(physx::PxSphereGeometry(radius), *material->GetPhysXMaterial());
	}
		
	PhysXSphereShape *PhysXSphereShape::WithRadius(float radius, PhysXMaterial *material)
	{
		PhysXSphereShape *shape = new PhysXSphereShape(radius, material);
		return shape->Autorelease();
	}
		

		
	PhysXBoxShape::PhysXBoxShape(const Vector3 &halfExtents, PhysXMaterial *material)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_shape = physics->createShape(physx::PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z), *material->GetPhysXMaterial());
	}
		
	PhysXBoxShape *PhysXBoxShape::WithHalfExtents(const Vector3 &halfExtents, PhysXMaterial *material)
	{
		PhysXBoxShape *shape = new PhysXBoxShape(halfExtents, material);
		return shape->Autorelease();
	}
		
		
	PhysXCapsuleShape::PhysXCapsuleShape(float radius, float height, PhysXMaterial *material)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_shape = physics->createShape(physx::PxCapsuleGeometry(radius, height * 0.5f), *material->GetPhysXMaterial());
	}
		
	PhysXCapsuleShape *PhysXCapsuleShape::WithRadius(float radius, float height, PhysXMaterial *material)
	{
		PhysXCapsuleShape *shape = new PhysXCapsuleShape(radius, height, material);
		return shape->Autorelease();
	}
		
		
	PhysXStaticPlaneShape::PhysXStaticPlaneShape(PhysXMaterial *material)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_shape = physics->createShape(physx::PxPlaneGeometry(), *material->GetPhysXMaterial());
	}
		
	PhysXStaticPlaneShape *PhysXStaticPlaneShape::WithMaterial(PhysXMaterial *material)
	{
		PhysXStaticPlaneShape *shape = new PhysXStaticPlaneShape(material);
		return shape->Autorelease();
	}
		
		
	PhysXTriangleMeshShape::PhysXTriangleMeshShape(Model *model, Vector3 scale)
	{
		_triangleMesh = new btTriangleMesh();
		
		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for(size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			AddMesh(mesh, scale);
		}
			
		_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
	}
		
	PhysXTriangleMeshShape::PhysXTriangleMeshShape(Mesh *mesh, Vector3 scale)
	{
		_triangleMesh = new btTriangleMesh();
			
		AddMesh(mesh, scale);
			
		_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
	}
		
	PhysXTriangleMeshShape::PhysXTriangleMeshShape(const Array *meshes, Vector3 scale)
	{
		_triangleMesh = new btTriangleMesh();
			
		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
				
			AddMesh(mesh, scale);
				
		});
			
		_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
	}
		
	PhysXTriangleMeshShape::~PhysXTriangleMeshShape()
	{
		delete _triangleMesh;
	}
		
	PhysXTriangleMeshShape *PhysXTriangleMeshShape::WithModel(Model *model, Vector3 scale)
	{
		PhysXTriangleMeshShape *shape = new PhysXTriangleMeshShape(model, scale);
		return shape->Autorelease();
	}
		
	void PhysXTriangleMeshShape::AddMesh(Mesh *mesh, Vector3 scale)
	{
		//TODO: Use btTriangleIndexVertexArray which reuses existing indexed vertex data and should be a lot faster to create
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if(!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}

		Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		for(size_t i = 0; i < mesh->GetIndicesCount()/3; i++)
		{
			if(i > 0) vertexIterator++;
			const Vector3 &vertex1 = *(vertexIterator++) * scale;
			const Vector3 &vertex2 = *(vertexIterator++) * scale;
			const Vector3 &vertex3 = *(vertexIterator) * scale;
			_triangleMesh->addTriangle(btVector3(vertex1.x, vertex1.y, vertex1.z), btVector3(vertex2.x, vertex2.y, vertex2.z), btVector3(vertex3.x, vertex3.y, vertex3.z), false);
		}
	}

	PhysXConvexHullShape::PhysXConvexHullShape(Mesh *mesh, PhysXMaterial *material)
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

		btConvexHullComputer *convexHullComputer = new btConvexHullComputer();
		btScalar actualMargin = convexHullComputer->compute(&vertices[0], 3 * sizeof(float), mesh->GetVerticesCount(), margin, 0.001f);
		
		btConvexHullShape *shape = new btConvexHullShape(convexHullComputer->vertices[0].m_floats, convexHullComputer->vertices.size());
		shape->setMargin(actualMargin);
		shape->recalcLocalAabb();

		delete convexHullComputer;
		_shape = shape;
	}

	PhysXConvexHullShape *PhysXConvexHullShape::WithMesh(Mesh *mesh, PhysXMaterial *material)
	{
		PhysXConvexHullShape *shape = new PhysXConvexHullShape(mesh, margin);
		return shape->Autorelease();
	}


	
	PhysXCompoundShape::PhysXCompoundShape()
	{
		_shape = new btCompoundShape();
	}

	PhysXCompoundShape::PhysXCompoundShape(Model *model, PhysXMaterial *material)
	{
		_shape = new btCompoundShape();

		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for (size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			PhysXConvexHullShape *shape = PhysXConvexHullShape::WithMesh(mesh, margin);
			AddChild(shape, Vector3(), Quaternion());
		}
	}

	PhysXCompoundShape::PhysXCompoundShape(const Array *meshes, PhysXMaterial *material)
	{
		_shape = new btCompoundShape();

		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			PhysXConvexHullShape *shape = PhysXConvexHullShape::WithMesh(mesh, margin);
			AddChild(shape, Vector3(), Quaternion());
		});
	}
		
	PhysXCompoundShape::~PhysXCompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void PhysXCompoundShape::AddChild(BulletShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		btCompoundShape *compoundShape = static_cast<btCompoundShape *>(_shape);
			
		_shapes.push_back(shape->Retain());
		
		btTransform transform;
		transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
		transform.setOrigin(btVector3(position.x, position.y, position.z));
		compoundShape->addChildShape(transform, shape->GetBulletShape());
	}

	PhysXCompoundShape *PhysXCompoundShape::WithModel(Model *model, PhysXMaterial *material)
	{
		PhysXCompoundShape *shape = new PhysXCompoundShape(model, margin);
		return shape->Autorelease();
	}
}
