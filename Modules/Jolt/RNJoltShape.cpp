//
//  RNJoltShape.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltShape.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>

#include <Jolt/Core/STLAllocator.h>

namespace RN
{
	RNDefineMeta(JoltShape, Object)
	RNDefineMeta(JoltSphereShape, JoltShape)
	RNDefineMeta(JoltBoxShape, JoltShape)
	RNDefineMeta(JoltCapsuleShape, JoltShape)
	RNDefineMeta(JoltTriangleMeshShape, JoltShape)
	RNDefineMeta(JoltConvexHullShape, JoltShape)
	RNDefineMeta(JoltCompoundShape, JoltShape)
		
	JoltShape::JoltShape() :
		_shape(nullptr), _material(nullptr)
	{}
		
	JoltShape::JoltShape(JPH::Shape *shape) :
		_shape(shape), _material(nullptr)
	{}
		
	JoltShape::~JoltShape()
	{
		if(_shape) _shape->Release();
		SafeRelease(_material);
	}

	void JoltShape::SetPose(RN::Vector3 positionOffset, RN::Quaternion rotationOffset)
	{
		//_shape->setLocalPose(Jolt::PxTransform(Jolt::PxVec3(positionOffset.x, positionOffset.y, positionOffset.z), Jolt::PxQuat(rotationOffset.x, rotationOffset.y, rotationOffset.z, rotationOffset.w)));
	}
		
		
		
	JoltSphereShape::JoltSphereShape(float radius, JoltMaterial *material)
	{
		_material = SafeRetain(material);
		_shape = new JPH::SphereShape(radius, material? material->GetJoltMaterial() : nullptr);
		_shape->AddRef();
	}
		
	JoltSphereShape *JoltSphereShape::WithRadius(float radius, JoltMaterial *material)
	{
		JoltSphereShape *shape = new JoltSphereShape(radius, material);
		return shape->Autorelease();
	}
		

		
	JoltBoxShape::JoltBoxShape(const Vector3 &halfExtents, JoltMaterial *material)
	{
		_material = SafeRetain(material);
		_shape = new JPH::BoxShape(JPH::Vec3Arg(halfExtents.x, halfExtents.y, halfExtents.z), JPH::cDefaultConvexRadius, material? material->GetJoltMaterial() : nullptr);
		_shape->AddRef();
	}
		
	JoltBoxShape *JoltBoxShape::WithHalfExtents(const Vector3 &halfExtents, JoltMaterial *material)
	{
		JoltBoxShape *shape = new JoltBoxShape(halfExtents, material);
		return shape->Autorelease();
	}
		
		
	JoltCapsuleShape::JoltCapsuleShape(float radius, float height, JoltMaterial *material)
	{
		_material = SafeRetain(material);
		_shape = new JPH::CapsuleShape(height * 0.5f, radius, material? material->GetJoltMaterial() : nullptr);
		_shape->AddRef();
	}
		
	JoltCapsuleShape *JoltCapsuleShape::WithRadius(float radius, float height, JoltMaterial *material)
	{
		JoltCapsuleShape *shape = new JoltCapsuleShape(radius, height, material);
		return shape->Autorelease();
	}

		
	JoltTriangleMeshShape::JoltTriangleMeshShape(Mesh *mesh, JoltMaterial *material, Vector3 scale, bool wantsDoubleSided)
	{
		JPH::TriangleList triangles;
		
		Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> iterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		size_t triangleCount = mesh->GetIndicesCount() / 3;
		for(size_t i = 0; i < triangleCount; i++)
		{
			const Vector3 &posA = *iterator++;
			const Vector3 &posB = *iterator++;
			const Vector3 &posC = *iterator;
			if(i < triangleCount-1)
			{
				iterator++;
			}
			
			triangles.push_back(JPH::Triangle(JPH::Float3(posA.x * scale.x, posA.y * scale.y, posA.z * scale.z), JPH::Float3(posB.x * scale.x, posB.y * scale.y, posB.z * scale.z), JPH::Float3(posC.x * scale.x, posC.y * scale.y, posC.z * scale.z)));
		}

		JPH::PhysicsMaterialList materials;
		if(material) materials.push_back(material->GetJoltMaterial());
		JPH::MeshShapeSettings settings(triangles, materials);
		JPH::Shape::ShapeResult result = settings.Create();

		_material = SafeRetain(material);
		if(result.IsValid())
		{
			_shape = result.Get();
			_shape->AddRef();
		}
	}
		
	JoltTriangleMeshShape *JoltTriangleMeshShape::WithMesh(Mesh *mesh, JoltMaterial *material, Vector3 scale, bool wantsDoubleSided)
	{
		JoltTriangleMeshShape *shape = new JoltTriangleMeshShape(mesh, material, scale, wantsDoubleSided);
		return shape->Autorelease();
	}

	JoltConvexHullShape::JoltConvexHullShape(Mesh *mesh, JoltMaterial *material, Vector3 scale)
	{
		JPH::Array<JPH::Vec3> vertices;
		
		Mesh::Chunk chunk = mesh->GetChunk();
		Mesh::ElementIterator<Vector3> iterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		size_t vertexCount = mesh->GetVerticesCount();
		for(size_t i = 0; i < vertexCount; i++)
		{
			const Vector3 &position = *iterator;
			if(i < vertexCount-1)
			{
				iterator++;
			}
			
			vertices.push_back(JPH::Vec3(position.x * scale.x, position.y * scale.y, position.z * scale.z));
		}

		JPH::ConvexHullShapeSettings settings(vertices, JPH::cDefaultConvexRadius, material? material->GetJoltMaterial() : nullptr);
		JPH::Shape::ShapeResult result = settings.Create();

		_material = SafeRetain(material);
		if(result.IsValid())
		{
			_shape = result.Get();
			_shape->AddRef();
		}
	}

	JoltConvexHullShape *JoltConvexHullShape::WithMesh(Mesh *mesh, JoltMaterial *material, Vector3 scale)
	{
		JoltConvexHullShape *shape = new JoltConvexHullShape(mesh, material, scale);
		return shape->Autorelease();
	}


	
	JoltCompoundShape::JoltCompoundShape()
	{

	}

	JoltCompoundShape::JoltCompoundShape(Model *model, JoltMaterial *material, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		_shape = new JPH::MutableCompoundShape();
		_shape->AddRef();
		
		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for(size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			AddChild(mesh, material, Vector3(), Quaternion(), scale, useTriangleMesh, wantsDoubleSided);
		}
	}

	JoltCompoundShape::JoltCompoundShape(const Array *meshes, JoltMaterial *material, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			AddChild(mesh, material, Vector3(), Quaternion(), scale, useTriangleMesh, wantsDoubleSided);
		});
	}
		
	JoltCompoundShape::~JoltCompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void JoltCompoundShape::AddChild(Mesh *mesh, JoltMaterial *material, const RN::Vector3 &position, const RN::Quaternion &rotation, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		JoltShape *shape = nullptr;
		if(useTriangleMesh)
		{
			shape = JoltTriangleMeshShape::WithMesh(mesh, material, scale, wantsDoubleSided);
		}
		else
		{
			shape = JoltConvexHullShape::WithMesh(mesh, material, scale);
		}
		
		_shapes.push_back(shape->Retain());
		
		JPH::MutableCompoundShape *compoundShape = static_cast<JPH::MutableCompoundShape*>(_shape);
		compoundShape->AddShape(JPH::Vec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), shape->GetJoltShape());
	}

	void JoltCompoundShape::AddChild(JoltShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		_shapes.push_back(shape->Retain());
		JPH::MutableCompoundShape *compoundShape = static_cast<JPH::MutableCompoundShape*>(_shape);
		compoundShape->AddShape(JPH::Vec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), shape->GetJoltShape());
	}

	JoltCompoundShape *JoltCompoundShape::WithModel(Model *model, JoltMaterial *material, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		JoltCompoundShape *shape = new JoltCompoundShape(model, material, scale, useTriangleMesh, wantsDoubleSided);
		return shape->Autorelease();
	}
}
