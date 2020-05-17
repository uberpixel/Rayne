//
//  RNPhysXShape.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXSHAPE_H_
#define __RAYNE_PHYSXSHAPE_H_

#include "RNPhysX.h"
#include "RNPhysXMaterial.h"

namespace physx
{
	class PxShape;
}

namespace RN
{
	class Mesh;
	class PhysXDynamicBody;
	class PhysXStaticBody;

	class PhysXShape : public Object
	{
	public:
		PhysXShape(physx::PxShape *shape);
			
		PXAPI physx::PxShape *GetPhysXShape() const { return _shape; }
		PXAPI void SetPose(RN::Vector3 positionOffset, RN::Quaternion rotationOffset);
			
	protected:
		PhysXShape();
		~PhysXShape() override;
			
		physx::PxShape *_shape;
		PhysXMaterial *_material;
			
		RNDeclareMetaAPI(PhysXShape, PXAPI)
	};
		
	class PhysXSphereShape : public PhysXShape
	{
	public:
		PXAPI PhysXSphereShape(float radius, PhysXMaterial *material);
			
		PXAPI static PhysXSphereShape *WithRadius(float radius, PhysXMaterial *material);
			
		RNDeclareMetaAPI(PhysXSphereShape, PXAPI)
	};
		
	class PhysXBoxShape : public PhysXShape
	{
	public:
		PXAPI PhysXBoxShape(const Vector3 &halfExtents, PhysXMaterial *material);
			
		PXAPI static PhysXBoxShape *WithHalfExtents(const Vector3& halfExtents, PhysXMaterial *material);
			
		RNDeclareMetaAPI(PhysXBoxShape, PXAPI)
	};
		
	class PhysXCapsuleShape : public PhysXShape
	{
	public:
		PXAPI PhysXCapsuleShape(float radius, float height, PhysXMaterial *material);
			
		PXAPI static PhysXCapsuleShape *WithRadius(float radius, float height, PhysXMaterial *material);
			
		RNDeclareMetaAPI(PhysXCapsuleShape, PXAPI)
	};
		
	class PhysXStaticPlaneShape : public PhysXShape
	{
	public:
		PXAPI PhysXStaticPlaneShape(PhysXMaterial *material);
			
		PXAPI static PhysXStaticPlaneShape *WithMaterial(PhysXMaterial *material);
			
		RNDeclareMetaAPI(PhysXStaticPlaneShape, PXAPI)
	};
		
	class PhysXTriangleMeshShape : public PhysXShape
	{
	public:
		PXAPI PhysXTriangleMeshShape(Mesh *mesh, PhysXMaterial *material, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
			
		PXAPI static PhysXTriangleMeshShape *WithMesh(Mesh *mesh, PhysXMaterial *material, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
			
	private:
		RNDeclareMetaAPI(PhysXTriangleMeshShape, PXAPI)
	};

	class PhysXConvexHullShape : public PhysXShape
	{
	public:
		PXAPI PhysXConvexHullShape(Mesh *mesh, PhysXMaterial *material);

		PXAPI static PhysXConvexHullShape *WithMesh(Mesh *mesh, PhysXMaterial *material);

	private:

		RNDeclareMetaAPI(PhysXConvexHullShape, PXAPI)
	};
		
	class PhysXCompoundShape : public PhysXShape
	{
	public:
		friend PhysXDynamicBody;
		friend PhysXStaticBody;
		PXAPI PhysXCompoundShape();
		PXAPI PhysXCompoundShape(Model *model, PhysXMaterial *material, bool fromConcaveMesh);
		PXAPI PhysXCompoundShape(const Array *meshes, PhysXMaterial *material, bool fromConcaveMesh);
		PXAPI ~PhysXCompoundShape();

		PXAPI void AddChild(PhysXShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);
		PhysXShape *GetShape(size_t index) const { return _shapes[index]; }
		size_t GetNumberOfShapes() const { return _shapes.size(); }

		PXAPI static PhysXCompoundShape *WithModel(Model *model, PhysXMaterial *material, bool fromConcaveMesh);
			
	private:
		std::vector<PhysXShape *> _shapes;
		std::vector<RN::Vector3> _positions;
		std::vector<RN::Quaternion> _rotations;
			
		RNDeclareMetaAPI(PhysXCompoundShape, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXSHAPE_H_) */
