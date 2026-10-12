//
//  RNJoltShape.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltShape.h"
#include "RNJoltInternals.h"
#include "RNJoltWheelCylinderShape.h"
#include "RNJoltWorld.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <Jolt/Core/STLAllocator.h>

namespace RN
{
	RNDefineMeta(JoltShape, Object)
	RNDefineMeta(JoltSphereShape, JoltShape)
	RNDefineMeta(JoltBoxShape, JoltShape)
	RNDefineMeta(JoltCapsuleShape, JoltShape)
	RNDefineMeta(JoltCylinderShape, JoltShape)
	RNDefineMeta(JoltWheelCylinderShape, JoltShape)
	RNDefineMeta(JoltTriangleMeshShape, JoltShape)
	RNDefineMeta(JoltHeightFieldShape, JoltShape)
	RNDefineMeta(JoltConvexHullShape, JoltShape)
	RNDefineMeta(JoltCompoundShape, JoltShape)

	JoltShape::JoltShape() :
		_shape(nullptr)
	{}

	JoltShape::JoltShape(JPH::Shape *shape) :
		_shape(shape)
	{}

	JoltShape::~JoltShape()
	{
		if(_shape) _shape->Release();
	}

	Vector3 JoltShape::GetCenterOfMass() const
	{
		if(!_shape) return Vector3();

		JPH::Vec3 center = _shape->GetCenterOfMass();
		return Vector3(center.GetX(), center.GetY(), center.GetZ());
	}

	AABB JoltShape::GetLocalBounds() const
	{
		if(!_shape) return AABB(Vector3(), Vector3());

		JPH::AABox bounds = _shape->GetLocalBounds();
		AABB result(JoltConversions::ToVector3(bounds.mMin), JoltConversions::ToVector3(bounds.mMax));
		result.position = Vector3();
		return result;
	}

	void JoltShape::SetMass(float mass)
	{
		if(!_shape || mass <= k::EpsilonFloat || _shape->GetType() != JPH::EShapeType::Convex) return;

		JPH::MassProperties properties = _shape->GetMassProperties();
		if(properties.mMass <= k::EpsilonFloat) return;

		JPH::ConvexShape *convexShape = static_cast<JPH::ConvexShape *>(_shape);
		convexShape->SetDensity(convexShape->GetDensity() * mass / properties.mMass);
	}

	void JoltShape::SetPose(RN::Vector3 positionOffset, RN::Quaternion rotationOffset)
	{
		//_shape->setLocalPose(Jolt::PxTransform(Jolt::PxVec3(positionOffset.x, positionOffset.y, positionOffset.z), Jolt::PxQuat(rotationOffset.x, rotationOffset.y, rotationOffset.z, rotationOffset.w)));
	}

	JoltSphereShape::JoltSphereShape(float radius)
	{
		_shape = new JPH::SphereShape(radius);
		_shape->AddRef();
	}

	JoltSphereShape *JoltSphereShape::WithRadius(float radius)
	{
		JoltSphereShape *shape = new JoltSphereShape(radius);
		return shape->Autorelease();
	}


	JoltBoxShape::JoltBoxShape(const Vector3 &halfExtents, float convexRadius)
	{
		_shape = new JPH::BoxShape(JPH::Vec3Arg(halfExtents.x, halfExtents.y, halfExtents.z), convexRadius);
		_shape->AddRef();
	}

	JoltBoxShape *JoltBoxShape::WithHalfExtents(const Vector3 &halfExtents, float convexRadius)
	{
		JoltBoxShape *shape = new JoltBoxShape(halfExtents, convexRadius);
		return shape->Autorelease();
	}


	JoltCapsuleShape::JoltCapsuleShape(float radius, float height)
	{
		_shape = new JPH::CapsuleShape(height * 0.5f, radius);
		_shape->AddRef();
	}

	JoltCapsuleShape *JoltCapsuleShape::WithRadius(float radius, float height)
	{
		JoltCapsuleShape *shape = new JoltCapsuleShape(radius, height);
		return shape->Autorelease();
	}


	JoltCylinderShape::JoltCylinderShape(float radius, float height, float convexRadius)
	{
		_shape = new JPH::CylinderShape(height * 0.5f, radius, convexRadius);
		_shape->AddRef();
	}

	JoltCylinderShape *JoltCylinderShape::WithRadius(float radius, float height, float convexRadius)
	{
		JoltCylinderShape *shape = new JoltCylinderShape(radius, height, convexRadius);
		return shape->Autorelease();
	}

	JoltWheelCylinderShape::JoltWheelCylinderShape(float radius, float height, float convexRadius)
	{
		_shape = new JPH::RNWheelCylinderShape(height * 0.5f, radius, convexRadius);
		_shape->AddRef();
	}

	JoltWheelCylinderShape *JoltWheelCylinderShape::WithRadius(float radius, float height, float convexRadius)
	{
		JoltWheelCylinderShape *shape = new JoltWheelCylinderShape(radius, height, convexRadius);
		return shape->Autorelease();
	}

	JoltTriangleMeshShape::JoltTriangleMeshShape(Mesh *mesh, Vector3 scale, bool wantsDoubleSided)
	{
		JPH::TriangleList triangles;

		Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> iterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		size_t triangleCount = mesh->GetIndicesCount() / 3;

		const float minAreaSq = 1e-12f;   // skip near-zero area tris
		const float maxAspect  = 1e4f;    // skip extremely skinny tris
		
		size_t badTriangleCount = 0;

		for(size_t i = 0; i < triangleCount; i++)
		{
			const Vector3 &posA = *iterator++;
			const Vector3 &posB = *iterator++;
			const Vector3 &posC = *iterator;
			if(i < triangleCount - 1)
			{
				iterator++;
			}

			JPH::Vec3 v0(posA.x * scale.x, posA.y * scale.y, posA.z * scale.z);
			JPH::Vec3 v1(posB.x * scale.x, posB.y * scale.y, posB.z * scale.z);
			JPH::Vec3 v2(posC.x * scale.x, posC.y * scale.y, posC.z * scale.z);

			// Compute area (squared) using cross product
			JPH::Vec3 cross = (v1 - v0).Cross(v2 - v0);
			float areaSq = cross.LengthSq() * 0.25f;

			if(areaSq < minAreaSq)
			{
				// Degenerate / zero-area triangle, skip it
				badTriangleCount += 1;
				continue;
			}

			// Optional: check aspect ratio
			float l0 = (v1 - v0).LengthSq();
			float l1 = (v2 - v1).LengthSq();
			float l2 = (v0 - v2).LengthSq();

			float minL = std::min({l0, l1, l2});
			float maxL = std::max({l0, l1, l2});

			if(minL < 1e-12f || maxL / minL > maxAspect)
			{
				// Too skinny, skip it
				badTriangleCount += 1;
				continue;
			}

			// Passed filters → keep triangle
			triangles.push_back(JPH::Triangle(JPH::Float3(v0.GetX(), v0.GetY(), v0.GetZ()),
											  JPH::Float3(v1.GetX(), v1.GetY(), v1.GetZ()),
											  JPH::Float3(v2.GetX(), v2.GetY(), v2.GetZ())));
		}
		
		//RN_DEBUG_ASSERT(badTriangleCount < triangleCount / 10, "More than 10% invalid triangles!");

		JPH::MeshShapeSettings settings(triangles);
		JPH::Shape::ShapeResult result = settings.Create();

		if(result.IsValid())
		{
			_shape = result.Get();
			_shape->AddRef();
		}
	}

	JoltTriangleMeshShape *JoltTriangleMeshShape::WithMesh(Mesh *mesh, Vector3 scale, bool wantsDoubleSided)
	{
		JoltTriangleMeshShape *shape = new JoltTriangleMeshShape(mesh, scale, wantsDoubleSided);
		return shape->Autorelease();
	}

	JoltHeightFieldShape::JoltHeightFieldShape(const float *samples, uint32 sampleCount, const Vector3 &offset, const Vector3 &scale, uint32 blockSize, uint32 bitsPerSample)
	{
		JPH::HeightFieldShapeSettings settings(samples,
											   JPH::Vec3Arg(offset.x, offset.y, offset.z),
											   JPH::Vec3Arg(scale.x, scale.y, scale.z),
											   sampleCount);
		settings.mBlockSize = blockSize;
		settings.mBitsPerSample = bitsPerSample;

		JPH::Shape::ShapeResult result = settings.Create();
		RN_DEBUG_ASSERT(result.IsValid(), "Invalid height field shape!");
		if(result.IsValid())
		{
			_shape = result.Get();
			_shape->AddRef();
		}
	}

	JoltHeightFieldShape *JoltHeightFieldShape::WithSamples(const float *samples, uint32 sampleCount, const Vector3 &offset, const Vector3 &scale, uint32 blockSize, uint32 bitsPerSample)
	{
		JoltHeightFieldShape *shape = new JoltHeightFieldShape(samples, sampleCount, offset, scale, blockSize, bitsPerSample);
		return shape->Autorelease();
	}

	JoltConvexHullShape::JoltConvexHullShape(Mesh *mesh, Vector3 scale, float convexRadius)
	{
		JPH::Array<JPH::Vec3> vertices;

		Mesh::Chunk chunk = mesh->GetChunk();
		Mesh::ElementIterator<Vector3> iterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		size_t vertexCount = mesh->GetVerticesCount();
		for(size_t i = 0; i < vertexCount; i++)
		{
			const Vector3 &position = *iterator;
			if(i < vertexCount - 1)
			{
				iterator++;
			}

			vertices.push_back(JPH::Vec3(position.x * scale.x, position.y * scale.y, position.z * scale.z));
		}

		JPH::ConvexHullShapeSettings settings(vertices, convexRadius);
		JPH::Shape::ShapeResult result = settings.Create();

		RN_DEBUG_ASSERT(result.IsValid(), "Invalid shape!");
		if(result.IsValid())
		{
			_shape = result.Get();
			_shape->AddRef();
		}
	}

	JoltConvexHullShape *JoltConvexHullShape::WithMesh(Mesh *mesh, Vector3 scale, float convexRadius)
	{
		JoltConvexHullShape *shape = new JoltConvexHullShape(mesh, scale, convexRadius);
		return shape->Autorelease();
	}


	JoltCompoundShape::JoltCompoundShape()
	{
		_shape = new JPH::MutableCompoundShape();
		_shape->AddRef();
	}

	bool JoltCompoundShape::CanScaleMassUniformly(JoltShape *shape, float mass, std::unordered_set<const JPH::Shape *> &visited)
	{
		JPH::Shape *joltShape = shape->GetJoltShape();
		if(!joltShape || !visited.insert(joltShape).second || !std::isfinite(mass) || mass <= k::EpsilonFloat) return false;
		JPH::MassProperties properties = joltShape->GetMassProperties();
		if(!std::isfinite(properties.mMass) || properties.mMass <= k::EpsilonFloat) return false;

		if(JoltCompoundShape *compound = shape->Downcast<JoltCompoundShape>())
		{
			float scale = mass / properties.mMass;
			if(!std::isfinite(scale) || scale <= 0.0f || compound->_shapes.empty()) return false;
			for(JoltShape *child : compound->_shapes)
			{
				if(!child || !child->GetJoltShape()) return false;
				float childMass = child->GetJoltShape()->GetMassProperties().mMass * scale;
				if(!CanScaleMassUniformly(child, childMass, visited)) return false;
			}
			return true;
		}

		if(joltShape->GetType() != JPH::EShapeType::Convex) return false;
		float density = static_cast<JPH::ConvexShape *>(joltShape)->GetDensity() * mass / properties.mMass;
		return std::isfinite(density) && density > 0.0f;
	}

	JoltCompoundShape::JoltCompoundShape(const std::vector<Child> &children)
	{
		if(children.empty() || children.size() > 0xffffffffU) return;

		bool hasCompoundChild = false;
		JPH::uint indexBits = 32 - JPH::CountLeadingZeros(static_cast<JPH::uint32>(children.size() - 1));
		for(const Child &child : children)
		{
			if(!child.shape || !child.shape->GetJoltShape() || !child.position.IsValid() || !child.rotation.IsValid()) return;
			if(indexBits + child.shape->GetJoltShape()->GetSubShapeIDBitsRecursive() > JPH::SubShapeID::MaxBits) return;
			hasCompoundChild |= child.shape->GetJoltShape()->GetType() == JPH::EShapeType::Compound;
		}

		_shapes.reserve(children.size());

		bool canBatchNested = hasCompoundChild;
		if(hasCompoundChild)
		{
			std::unordered_set<const JPH::Shape *> visited;
			for(const Child &child : children)
			{
				if(!CanScaleMassUniformly(child.shape, child.mass, visited))
				{
					canBatchNested = false;
					break;
				}
			}
		}

		// Aliases and suppressed leaf masses can move nested COM nonuniformly.
		if(hasCompoundChild && !canBatchNested)
		{
			_shape = new JPH::MutableCompoundShape();
			_shape->AddRef();
			for(const Child &child : children)
			{
				AddChild(child.shape, child.position, child.rotation, child.userData);
				SetChildMass(_shapes.size() - 1, child.mass);
			}
			return;
		}

		JPH::MutableCompoundShapeSettings settings;
		settings.mSubShapes.reserve(children.size());
		for(const Child &child : children)
		{
			if(!canBatchNested && child.mass > k::EpsilonFloat) child.shape->SetMass(child.mass);
			_shapes.push_back(child.shape->Retain());
			settings.AddShape(JPH::Vec3(child.position.x, child.position.y, child.position.z), JPH::Quat(child.rotation.x, child.rotation.y, child.rotation.z, child.rotation.w), child.shape->GetJoltShape(), child.userData);
		}

		JPH::Shape::ShapeResult result = settings.Create();
		if(!result.IsValid()) return;
		_shape = result.Get();
		_shape->AddRef();
		if(canBatchNested)
		{
			// Capture child transforms before mass changes, as AddChild does.
			for(const Child &child : children) child.shape->SetMass(child.mass);
			UpdateCenterOfMass();
		}
	}

	JoltCompoundShape::JoltCompoundShape(Model *model, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		_shape = new JPH::MutableCompoundShape();
		_shape->AddRef();

		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for(size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			AddChild(mesh, Vector3(), Quaternion(), scale, useTriangleMesh, wantsDoubleSided);
		}
	}

	JoltCompoundShape::JoltCompoundShape(const Array *meshes, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		_shape = new JPH::MutableCompoundShape();
		_shape->AddRef();

		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			AddChild(mesh, Vector3(), Quaternion(), scale, useTriangleMesh, wantsDoubleSided);
		});
	}

	JoltCompoundShape::~JoltCompoundShape()
	{
		for(JoltShape *shape : _shapes)
		{
			if(shape) shape->Release();
		}
	}

	void JoltCompoundShape::UpdateCenterOfMass()
	{
		JPH::MutableCompoundShape *compoundShape = static_cast<JPH::MutableCompoundShape *>(_shape);
		compoundShape->AdjustCenterOfMass();
	}

	void JoltCompoundShape::AddChild(Mesh *mesh, const RN::Vector3 &position, const RN::Quaternion &rotation, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		AddChild(mesh, position, rotation, scale, useTriangleMesh, wantsDoubleSided, 0);
	}

	void JoltCompoundShape::AddChild(Mesh *mesh, const RN::Vector3 &position, const RN::Quaternion &rotation, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided, uint32 userData)
	{
		JoltShape *shape = nullptr;
		if(useTriangleMesh)
		{
			shape = JoltTriangleMeshShape::WithMesh(mesh, scale, wantsDoubleSided);
		}
		else
		{
			shape = JoltConvexHullShape::WithMesh(mesh, scale);
		}

		_shapes.push_back(shape->Retain());
		JPH::MutableCompoundShape *compoundShape = static_cast<JPH::MutableCompoundShape *>(_shape);
		compoundShape->AddShape(JPH::Vec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), shape->GetJoltShape(), userData);
		UpdateCenterOfMass();
	}

	void JoltCompoundShape::AddChild(JoltShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		AddChild(shape, position, rotation, 0);
	}

	void JoltCompoundShape::AddChild(JoltShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation, uint32 userData)
	{
		_shapes.push_back(shape->Retain());
		JPH::MutableCompoundShape *compoundShape = static_cast<JPH::MutableCompoundShape *>(_shape);
		compoundShape->AddShape(JPH::Vec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), shape->GetJoltShape(), userData);
		UpdateCenterOfMass();
	}

	bool JoltCompoundShape::SetChildMass(size_t index, float mass)
	{
		if(index >= _shapes.size() || mass <= k::EpsilonFloat) return false;

		JoltShape *shape = _shapes[index];
		if(!shape) return false;

		shape->SetMass(mass);
		UpdateCenterOfMass();
		return true;
	}

	void JoltCompoundShape::SetMass(float mass)
	{
		if(mass <= k::EpsilonFloat) return;

		JPH::MassProperties properties = _shape->GetMassProperties();
		if(properties.mMass <= k::EpsilonFloat) return;

		float scale = mass / properties.mMass;
		for(JoltShape *shape : _shapes)
		{
			if(!shape) continue;

			JPH::MassProperties childProperties = shape->GetJoltShape()->GetMassProperties();
			if(childProperties.mMass <= k::EpsilonFloat) continue;
			shape->SetMass(childProperties.mMass * scale);
		}

		UpdateCenterOfMass();
	}

	JoltCompoundShape *JoltCompoundShape::WithModel(Model *model, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided)
	{
		JoltCompoundShape *shape = new JoltCompoundShape(model, scale, useTriangleMesh, wantsDoubleSided);
		return shape->Autorelease();
	}
} // namespace RN
