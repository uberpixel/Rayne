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
		_shape(nullptr), _material(nullptr)
	{}
		
	PhysXShape::PhysXShape(physx::PxShape *shape) :
		_shape(shape), _material(nullptr)
	{}
		
	PhysXShape::~PhysXShape()
	{
		if(_shape) _shape->release();
		SafeRelease(_material);
	}
		
		
		
	PhysXSphereShape::PhysXSphereShape(float radius, PhysXMaterial *material)
	{
		_material = material->Retain();
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
		_material = material->Retain();
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
		_material = material->Retain();
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
		_material = material->Retain();
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_shape = physics->createShape(physx::PxPlaneGeometry(), *material->GetPhysXMaterial());
	}
		
	PhysXStaticPlaneShape *PhysXStaticPlaneShape::WithMaterial(PhysXMaterial *material)
	{
		PhysXStaticPlaneShape *shape = new PhysXStaticPlaneShape(material);
		return shape->Autorelease();
	}
		
	PhysXTriangleMeshShape::PhysXTriangleMeshShape(Mesh *mesh, PhysXMaterial *material, Vector3 scale)
	{
		//TODO: Use btTriangleIndexVertexArray which reuses existing indexed vertex data and should be a lot faster to create
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		const Mesh::VertexAttribute *indexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices);
		RN_ASSERT(vertexAttribute && vertexAttribute->GetType() == PrimitiveType::Vector3, "Mesh needs to have vertices of Vector3!");
		RN_ASSERT(indexAttribute, "Mesh needs indices!");

		physx::PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = mesh->GetVerticesCount();
		meshDesc.points.stride = mesh->GetStride();
		meshDesc.points.data = mesh->GetCPUVertexBuffer();

		meshDesc.triangles.count = mesh->GetIndicesCount() / 3;
		meshDesc.triangles.stride = indexAttribute->GetSize()*3;
		meshDesc.triangles.data = mesh->GetCPUIndicesBuffer();
		if(indexAttribute->GetSize() == 2)
			meshDesc.flags = physx::PxMeshFlag::e16_BIT_INDICES;

		physx::PxDefaultMemoryOutputStream writeBuffer;
		physx::PxTriangleMeshCookingResult::Enum result;
		physx::PxCooking *cooking = PhysXWorld::GetSharedInstance()->GetPhysXCooking();
		bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
		RN_ASSERT(status, "Couldn't cook mesh!");

		physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());

		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		physx::PxTriangleMesh *triangleMesh = physics->createTriangleMesh(readBuffer);

		physx::PxShape* shape = physics->createShape(physx::PxTriangleMeshGeometry(triangleMesh), *material->GetPhysXMaterial());

		_material = material->Retain();
		_shape = shape;
	}
		
	PhysXTriangleMeshShape *PhysXTriangleMeshShape::WithMesh(Mesh *mesh, PhysXMaterial *material, Vector3 scale)
	{
		PhysXTriangleMeshShape *shape = new PhysXTriangleMeshShape(mesh, material, scale);
		return shape->Autorelease();
	}

	PhysXConvexHullShape::PhysXConvexHullShape(Mesh *mesh, PhysXMaterial *material)
	{
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		RN_ASSERT(vertexAttribute && vertexAttribute->GetType() == PrimitiveType::Vector3, "Mesh needs to have vertices of Vector3!");

		physx::PxConvexMeshDesc convexDesc;
		convexDesc.points.count = mesh->GetVerticesCount();
		convexDesc.points.stride = mesh->GetStride();
		convexDesc.points.data = mesh->GetCPUVertexBuffer();
		convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

		physx::PxDefaultMemoryOutputStream buf;
		physx::PxConvexMeshCookingResult::Enum result;
		physx::PxCooking *cooking = PhysXWorld::GetSharedInstance()->GetPhysXCooking();
		RN_ASSERT(cooking->cookConvexMesh(convexDesc, buf, &result), "Couldn't cook mesh!");
		physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());

		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		physx::PxConvexMesh* convexMesh = physics->createConvexMesh(input);

		physx::PxShape* shape = physics->createShape(physx::PxConvexMeshGeometry(convexMesh), *material->GetPhysXMaterial());

		_material = material->Retain();
		_shape = shape;
	}

	PhysXConvexHullShape *PhysXConvexHullShape::WithMesh(Mesh *mesh, PhysXMaterial *material)
	{
		PhysXConvexHullShape *shape = new PhysXConvexHullShape(mesh, material);
		return shape->Autorelease();
	}


	
	PhysXCompoundShape::PhysXCompoundShape()
	{

	}

	PhysXCompoundShape::PhysXCompoundShape(Model *model, PhysXMaterial *material, bool fromConcaveMesh)
	{
		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for (size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);

			if(fromConcaveMesh)
			{
				PhysXTriangleMeshShape *shape = PhysXTriangleMeshShape::WithMesh(mesh, material);
				AddChild(shape, Vector3(), Quaternion());
			}
			else
			{
				PhysXConvexHullShape *shape = PhysXConvexHullShape::WithMesh(mesh, material);
				AddChild(shape, Vector3(), Quaternion());
			}
		}
	}

	PhysXCompoundShape::PhysXCompoundShape(const Array *meshes, PhysXMaterial *material, bool fromConcaveMesh)
	{
		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			if(fromConcaveMesh)
			{
				PhysXTriangleMeshShape *shape = PhysXTriangleMeshShape::WithMesh(mesh, material);
				AddChild(shape, Vector3(), Quaternion());
			}
			else
			{
				PhysXConvexHullShape *shape = PhysXConvexHullShape::WithMesh(mesh, material);
				AddChild(shape, Vector3(), Quaternion());
			}
		});
	}
		
	PhysXCompoundShape::~PhysXCompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void PhysXCompoundShape::AddChild(PhysXShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		_shapes.push_back(shape->Retain());
		_positions.push_back(position);
		_rotations.push_back(rotation);
	}

	PhysXCompoundShape *PhysXCompoundShape::WithModel(Model *model, PhysXMaterial *material, bool fromConcaveMesh)
	{
		PhysXCompoundShape *shape = new PhysXCompoundShape(model, material, fromConcaveMesh);
		return shape->Autorelease();
	}
}
