//
//  RNBulletShape.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletShape.h"
#include "btBulletDynamicsCommon.h"
#include <Math/RNVector.h>
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "LinearMath/btConvexHullComputer.h"

namespace RN
{
	RNDefineMeta(BulletShape, Object)
	RNDefineMeta(BulletSphereShape, BulletShape)
	RNDefineMeta(BulletMultiSphereShape, BulletShape)
	RNDefineMeta(BulletBoxShape, BulletShape)
	RNDefineMeta(BulletCylinderShape, BulletShape)
	RNDefineMeta(BulletCapsuleShape, BulletShape)
	RNDefineMeta(BulletStaticPlaneShape, BulletShape)
	RNDefineMeta(BulletTriangleMeshShape, BulletShape)
	RNDefineMeta(BulletConvexTriangleMeshShape, BulletShape)
	RNDefineMeta(BulletGImpactShape, BulletShape)
	RNDefineMeta(BulletConvexHullShape, BulletShape)
	RNDefineMeta(BulletCompoundShape, BulletShape)
		
	BulletShape::BulletShape() :
		_shape(nullptr)
	{}
		
	BulletShape::BulletShape(btCollisionShape *shape) :
		_shape(shape)
	{}
		
	BulletShape::~BulletShape()
	{
		delete _shape;
	}
		
		
		
	Vector3 BulletShape::CalculateLocalInertia(float mass)
	{
		btVector3 inertia;
		_shape->calculateLocalInertia(mass, inertia);
			
		return Vector3(inertia.x(), inertia.y(), inertia.z());
	}
		
	void BulletShape::SetScale(const Vector3 &scale)
	{
		_shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
	}
		
		
		
	BulletSphereShape::BulletSphereShape(float radius)
	{
		_shape = new btSphereShape(radius);
	}
		
	BulletSphereShape *BulletSphereShape::WithRadius(float radius)
	{
		BulletSphereShape *shape = new BulletSphereShape(radius);
		return shape->Autorelease();
	}
		
		
	BulletMultiSphereShape::BulletMultiSphereShape(const Vector3 *positions, float *radii, int count)
	{
		btVector3 *btPositions = new btVector3[count];
			
		for(int i = 0; i < count; i ++)
			btPositions[i] = btVector3(positions[i].x, positions[i].y, positions[i].z);
			
		_shape = new btMultiSphereShape(btPositions, radii, count);
		delete [] btPositions;
	}
		
	BulletMultiSphereShape *BulletMultiSphereShape::WithHeight(float height, float width)
	{
		Vector3 positions[2] = { Vector3(0.0, height * 0.5f - width, 0.0f), Vector3(0.0f, -height * 0.5f + width, 0.0f) };
		float radii[2] = { width, width };
			
		BulletMultiSphereShape *shape = new BulletMultiSphereShape(positions, radii, 2);
		return shape->Autorelease();
	}
		
		
	BulletBoxShape::BulletBoxShape(const Vector3 &halfExtents)
	{
		_shape = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
	}
		
	BulletBoxShape *BulletBoxShape::WithHalfExtents(const Vector3 &halfExtents)
	{
		BulletBoxShape *shape = new BulletBoxShape(halfExtents);
		return shape->Autorelease();
	}
		
		
	BulletCylinderShape::BulletCylinderShape(const Vector3 &halfExtents)
	{
		_shape = new btCylinderShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
	}
		
	BulletCylinderShape *BulletCylinderShape::WithHalfExtents(const Vector3 &halfExtents)
	{
		BulletCylinderShape *shape = new BulletCylinderShape(halfExtents);
		return shape->Autorelease();
	}
		
		
	BulletCapsuleShape::BulletCapsuleShape(float radius, float height)
	{
		_shape = new btCapsuleShape(radius, height);
	}
		
	BulletCapsuleShape *BulletCapsuleShape::WithRadius(float radius, float height)
	{
		BulletCapsuleShape *shape = new BulletCapsuleShape(radius, height);
		return shape->Autorelease();
	}
		
		
	BulletStaticPlaneShape::BulletStaticPlaneShape(const Vector3 &normal, float constant)
	{
		_shape = new btStaticPlaneShape(btVector3(normal.x, normal.y, normal.z), constant);
	}
		
	BulletStaticPlaneShape *BulletStaticPlaneShape::WithNormal(const Vector3 &normal, float constant)
	{
		BulletStaticPlaneShape *shape = new BulletStaticPlaneShape(normal, constant);
		return shape->Autorelease();
	}
		
		
	BulletTriangleMeshShape::BulletTriangleMeshShape(Model *model, Vector3 scale)
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
		
	BulletTriangleMeshShape::BulletTriangleMeshShape(Mesh *mesh, Vector3 scale)
	{
		_triangleMesh = new btTriangleMesh();
			
		AddMesh(mesh, scale);
			
		_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
	}
		
	BulletTriangleMeshShape::BulletTriangleMeshShape(const Array *meshes, Vector3 scale)
	{
		_triangleMesh = new btTriangleMesh();
			
		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
				
			AddMesh(mesh, scale);
				
		});
			
		_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
	}
		
	BulletTriangleMeshShape::~BulletTriangleMeshShape()
	{
		delete _triangleMesh;
	}
		
	BulletTriangleMeshShape *BulletTriangleMeshShape::WithModel(Model *model, Vector3 scale)
	{
		BulletTriangleMeshShape *shape = new BulletTriangleMeshShape(model, scale);
		return shape->Autorelease();
	}
		
	Vector3 BulletTriangleMeshShape::CalculateLocalInertia(float mass)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}
		
	void BulletTriangleMeshShape::AddMesh(Mesh *mesh, Vector3 scale)
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



	BulletConvexTriangleMeshShape::BulletConvexTriangleMeshShape(Mesh *mesh)
	{
		_triangleMesh = new btTriangleMesh();

		//TODO: Use btTriangleIndexVertexArray which reuses existing indexed vertex data and should be a lot faster to create
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if (!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}

		Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		for (size_t i = 0; i < mesh->GetIndicesCount() / 3; i++)
		{
			if (i > 0) vertexIterator++;
			const Vector3 &vertex1 = *(vertexIterator++);
			const Vector3 &vertex2 = *(vertexIterator++);
			const Vector3 &vertex3 = *(vertexIterator);
			_triangleMesh->addTriangle(btVector3(vertex1.x, vertex1.y, vertex1.z), btVector3(vertex2.x, vertex2.y, vertex2.z), btVector3(vertex3.x, vertex3.y, vertex3.z), false);
		}

		_shape = new btConvexTriangleMeshShape(_triangleMesh, true);
	}

	BulletConvexTriangleMeshShape::~BulletConvexTriangleMeshShape()
	{
		delete _triangleMesh;
	}

	BulletConvexTriangleMeshShape *BulletConvexTriangleMeshShape::WithMesh(Mesh *mesh)
	{
		BulletConvexTriangleMeshShape *shape = new BulletConvexTriangleMeshShape(mesh);
		return shape->Autorelease();
	}



	BulletGImpactShape::BulletGImpactShape(Mesh *mesh)
	{
		_triangleMesh = new btTriangleMesh();

		//TODO: Use btTriangleIndexVertexArray which reuses existing indexed vertex data and should be a lot faster to create
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if (!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}

		Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		for (size_t i = 0; i < mesh->GetIndicesCount() / 3; i++)
		{
			if (i > 0) vertexIterator++;
			const Vector3 &vertex1 = *(vertexIterator++);
			const Vector3 &vertex2 = *(vertexIterator++);
			const Vector3 &vertex3 = *(vertexIterator);
			_triangleMesh->addTriangle(btVector3(vertex1.x, vertex1.y, vertex1.z), btVector3(vertex2.x, vertex2.y, vertex2.z), btVector3(vertex3.x, vertex3.y, vertex3.z), false);
		}

		_shape = new btGImpactMeshShape(_triangleMesh);
		static_cast<btGImpactMeshShape*>(_shape)->updateBound();
	}

	BulletGImpactShape::~BulletGImpactShape()
	{
		delete _triangleMesh;
	}

	BulletGImpactShape *BulletGImpactShape::WithMesh(Mesh *mesh)
	{
		BulletGImpactShape *shape = new BulletGImpactShape(mesh);
		return shape->Autorelease();
	}



	BulletConvexHullShape::BulletConvexHullShape(Mesh *mesh, float margin)
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

	BulletConvexHullShape *BulletConvexHullShape::WithMesh(Mesh *mesh, float margin)
	{
		BulletConvexHullShape *shape = new BulletConvexHullShape(mesh, margin);
		return shape->Autorelease();
	}


	
	BulletCompoundShape::BulletCompoundShape()
	{
		_shape = new btCompoundShape();
	}

	BulletCompoundShape::BulletCompoundShape(Model *model, float margin)
	{
		_shape = new btCompoundShape();

		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for (size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			BulletConvexHullShape *shape = BulletConvexHullShape::WithMesh(mesh, margin);
			AddChild(shape, Vector3(), Quaternion());
		}
	}

	BulletCompoundShape::BulletCompoundShape(const Array *meshes, float margin)
	{
		_shape = new btCompoundShape();

		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			BulletConvexHullShape *shape = BulletConvexHullShape::WithMesh(mesh, margin);
			AddChild(shape, Vector3(), Quaternion());
		});
	}
		
	BulletCompoundShape::~BulletCompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void BulletCompoundShape::AddChild(BulletShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		btCompoundShape *compoundShape = static_cast<btCompoundShape *>(_shape);
			
		_shapes.push_back(shape->Retain());
		
		btTransform transform;
		transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
		transform.setOrigin(btVector3(position.x, position.y, position.z));
		compoundShape->addChildShape(transform, shape->GetBulletShape());
	}

	BulletCompoundShape *BulletCompoundShape::WithModel(Model *model, float margin)
	{
		BulletCompoundShape *shape = new BulletCompoundShape(model, margin);
		return shape->Autorelease();
	}
}
