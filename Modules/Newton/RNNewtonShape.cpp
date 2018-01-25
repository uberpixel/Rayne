//
//  RNNewtonShape.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonShape.h"
#include "Newton.h"
#include "RNNewtonWorld.h"
#include "RNNewtonInternals.h"

namespace RN
{
	RNDefineMeta(NewtonShape, Object)
	RNDefineMeta(NewtonSphereShape, NewtonShape)
	RNDefineMeta(NewtonBoxShape, NewtonShape)
	RNDefineMeta(NewtonCapsuleShape, NewtonShape)
	RNDefineMeta(NewtonTriangleMeshShape, NewtonShape)
	RNDefineMeta(NewtonConvexHullShape, NewtonShape)
	RNDefineMeta(NewtonCompoundShape, NewtonShape)
		
	NewtonShape::NewtonShape() :
		_shape(nullptr)
	{}
		
	NewtonShape::NewtonShape(NewtonCollision *shape) :
		_shape(shape)
	{}
		
	NewtonShape::~NewtonShape()
	{
		NewtonDestroyCollision(_shape);
	}
		
		
		
	NewtonSphereShape::NewtonSphereShape(float radius)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateSphere(newtonInstance, radius, 0, NULL);
	}
		
	NewtonSphereShape *NewtonSphereShape::WithRadius(float radius)
	{
		NewtonSphereShape *shape = new NewtonSphereShape(radius);
		return shape->Autorelease();
	}
		

		
	NewtonBoxShape::NewtonBoxShape(const Vector3 &halfExtents)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateBox(newtonInstance, halfExtents.x*2.0f, halfExtents.y*2.0f, halfExtents.z*2.0f, 0, NULL);
	}
		
	NewtonBoxShape *NewtonBoxShape::WithHalfExtents(const Vector3 &halfExtents)
	{
		NewtonBoxShape *shape = new NewtonBoxShape(halfExtents);
		return shape->Autorelease();
	}
		
		
	NewtonCapsuleShape::NewtonCapsuleShape(float radius, float height)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateCapsule(newtonInstance, radius, radius, height, 0, NULL);
	}
		
	NewtonCapsuleShape *NewtonCapsuleShape::WithRadius(float radius, float height)
	{
		NewtonCapsuleShape *shape = new NewtonCapsuleShape(radius, height);
		return shape->Autorelease();
	}

		
	NewtonTriangleMeshShape::NewtonTriangleMeshShape(Model *model, Vector3 scale)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateTreeCollision(newtonInstance, 0);
		NewtonTreeCollisionBeginBuild(_shape);

		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for(size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			AddMesh(mesh, scale);
		}

		NewtonTreeCollisionEndBuild(_shape, 1);
	}

	NewtonTriangleMeshShape::NewtonTriangleMeshShape(Mesh *mesh, Vector3 scale)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateTreeCollision(newtonInstance, 0);
		NewtonTreeCollisionBeginBuild(_shape);

		AddMesh(mesh, scale);

		NewtonTreeCollisionEndBuild(_shape, 1);
	}

	NewtonTriangleMeshShape::NewtonTriangleMeshShape(const Array *meshes, Vector3 scale)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateTreeCollision(newtonInstance, 0);
		NewtonTreeCollisionBeginBuild(_shape);

		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {

			AddMesh(mesh, scale);

		});

		NewtonTreeCollisionEndBuild(_shape, 1);
	}

	NewtonTriangleMeshShape::NewtonTriangleMeshShape(const String *filename)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();

		File *file = File::WithName(filename, File::Mode::Read);
		RN_ASSERT(file, "File doesn't exist!");
		_shape = NewtonCreateCollisionFromSerialization(newtonInstance, NewtonSerialization::DeserializeCallback, file);
	}

	void NewtonTriangleMeshShape::SerializeToFile(const String *filename)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();

		File *file = File::WithName(filename, File::Mode::Write);
		NewtonCollisionSerialize(newtonInstance, _shape, NewtonSerialization::SerializeCallback, file);
	}

	NewtonTriangleMeshShape *NewtonTriangleMeshShape::WithModel(Model *model, Vector3 scale)
	{
		NewtonTriangleMeshShape *shape = new NewtonTriangleMeshShape(model, scale);
		return shape->Autorelease();
	}

	NewtonTriangleMeshShape *NewtonTriangleMeshShape::WithFile(const String *filename)
	{
		NewtonTriangleMeshShape *shape = new NewtonTriangleMeshShape(filename);
		return shape->Autorelease();
	}

	void NewtonTriangleMeshShape::AddMesh(Mesh *mesh, Vector3 scale)
	{
		//TODO: Use btTriangleIndexVertexArray which reuses existing indexed vertex data and should be a lot faster to create
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if(!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}

		Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		for(size_t i = 0; i < mesh->GetIndicesCount() / 3; i++)
		{
			if(i > 0) vertexIterator++;
			const Vector3 &vertex1 = *(vertexIterator++) * scale;
			const Vector3 &vertex2 = *(vertexIterator++) * scale;
			const Vector3 &vertex3 = *(vertexIterator)* scale;

			float face[9] = { vertex1.x, vertex1.y, vertex1.z, vertex2.x, vertex2.y, vertex2.z, vertex3.x, vertex3.y, vertex3.z };
			NewtonTreeCollisionAddFace(_shape, 3, face, 3 * sizeof(float), 0);
		}
	}


	NewtonConvexHullShape::NewtonConvexHullShape(Mesh *mesh)
	{
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		RN_ASSERT(vertexAttribute && vertexAttribute->GetType() == PrimitiveType::Vector3, "Mesh needs to have vertices of Vector3!");

		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateConvexHull(newtonInstance, mesh->GetVerticesCount(), static_cast<float*>(mesh->GetCPUVertexBuffer()), mesh->GetStride(), 0.0f, 0, nullptr);
	}

	NewtonConvexHullShape *NewtonConvexHullShape::WithMesh(Mesh *mesh)
	{
		NewtonConvexHullShape *shape = new NewtonConvexHullShape(mesh);
		return shape->Autorelease();
	}

	NewtonCompoundShape::NewtonCompoundShape() : _isEditMode(false)
	{
		
	}

	NewtonCompoundShape::NewtonCompoundShape(Model *model) : _isEditMode(true)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateCompoundCollision(newtonInstance, 0);
		NewtonCompoundCollisionBeginAddRemove(_shape);

		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for (size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);

			NewtonConvexHullShape *shape = NewtonConvexHullShape::WithMesh(mesh);
			AddChild(shape, Vector3(), Quaternion());
		}

		NewtonCompoundCollisionEndAddRemove(_shape);
		_isEditMode = false;
	}

	NewtonCompoundShape::NewtonCompoundShape(const Array *meshes) : _isEditMode(true)
	{
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_shape = NewtonCreateCompoundCollision(newtonInstance, 0);
		NewtonCompoundCollisionBeginAddRemove(_shape);

		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			NewtonConvexHullShape *shape = NewtonConvexHullShape::WithMesh(mesh);
			AddChild(shape, Vector3(), Quaternion());
		});

		NewtonCompoundCollisionEndAddRemove(_shape);
		_isEditMode = false;
	}
		
	NewtonCompoundShape::~NewtonCompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void NewtonCompoundShape::AddChild(NewtonShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		if(!_isEditMode)
			NewtonCompoundCollisionBeginAddRemove(_shape);

		NewtonCompoundCollisionAddSubCollision(_shape, shape->GetNewtonShape());

		if(!_isEditMode)
			NewtonCompoundCollisionEndAddRemove(_shape);

		_shapes.push_back(shape->Retain());
		_positions.push_back(position);
		_rotations.push_back(rotation);
	}

	NewtonCompoundShape *NewtonCompoundShape::WithModel(Model *model)
	{
		NewtonCompoundShape *shape = new NewtonCompoundShape(model);
		return shape->Autorelease();
	}
}
