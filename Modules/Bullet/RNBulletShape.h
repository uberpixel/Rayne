//
//  RNBulletShape.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETSHAPE_H_
#define __RAYNE_BULLETSHAPE_H_

#include "RNBullet.h"

class btCollisionShape;
class btTriangleMesh;

namespace RN
{
	class BulletShape : public Object
	{
	public:
		BulletShape(btCollisionShape *shape);
			
		BTAPI virtual Vector3 CalculateLocalInertia(float mass);
		BTAPI void SetScale(const Vector3 &scale);
			
		BTAPI btCollisionShape *GetBulletShape() const { return _shape; }
			
	protected:
		BulletShape();
		~BulletShape() override;
			
		btCollisionShape *_shape;
			
		RNDeclareMetaAPI(BulletShape, BTAPI)
	};
		
	class BulletSphereShape : public BulletShape
	{
	public:
		BTAPI BulletSphereShape(float radius);
			
		BTAPI static BulletSphereShape *WithRadius(float radius);
			
		RNDeclareMetaAPI(BulletSphereShape, BTAPI)
	};
		
	class BulletMultiSphereShape : public BulletShape
	{
	public:
		BTAPI BulletMultiSphereShape(const Vector3 *positions, float *radii, int count);
			
		BTAPI static BulletMultiSphereShape *WithHeight(float height, float width);
			
		RNDeclareMetaAPI(BulletMultiSphereShape, BTAPI)
	};
		
	class BulletBoxShape : public BulletShape
	{
	public:
		BTAPI BulletBoxShape(const Vector3 &halfExtents);
			
		BTAPI static BulletBoxShape *WithHalfExtents(const Vector3& halfExtents);
			
		RNDeclareMetaAPI(BulletBoxShape, BTAPI)
	};
		
	class BulletCylinderShape : public BulletShape
	{
	public:
		BTAPI BulletCylinderShape(const Vector3 &halfExtents);
			
		BTAPI static BulletCylinderShape *WithHalfExtents(const Vector3 &halfExtents);
			
		RNDeclareMetaAPI(BulletCylinderShape, BTAPI)
	};
		
	class BulletCapsuleShape : public BulletShape
	{
	public:
		BTAPI BulletCapsuleShape(float radius, float height);
			
		BTAPI static BulletCapsuleShape *WithRadius(float radius, float height);
			
		RNDeclareMetaAPI(BulletCapsuleShape, BTAPI)
	};
		
	class BulletStaticPlaneShape : public BulletShape
	{
	public:
		BTAPI BulletStaticPlaneShape(const Vector3 &normal, float constant);
			
		BTAPI static BulletStaticPlaneShape *WithNormal(const Vector3 &normal, float constant);
			
		RNDeclareMetaAPI(BulletStaticPlaneShape, BTAPI)
	};
		
	class BulletTriangleMeshShape : public BulletShape
	{
	public:
		BTAPI BulletTriangleMeshShape(Model *model);
		BTAPI BulletTriangleMeshShape(Mesh *mesh);
		BTAPI BulletTriangleMeshShape(const Array *meshes);
			
		BTAPI ~BulletTriangleMeshShape() override;
			
		BTAPI Vector3 CalculateLocalInertia(float mass) override;
			
		BTAPI static BulletTriangleMeshShape *WithModel(Model *model);
			
	private:
		void AddMesh(Mesh *mesh);
		btTriangleMesh *_triangleMesh;
			
		RNDeclareMetaAPI(BulletTriangleMeshShape, BTAPI)
	};
		
	class BulletCompoundShape : public BulletShape
	{
	public:
		BTAPI BulletCompoundShape();
		BTAPI ~BulletCompoundShape();
		BTAPI void AddChild(BulletShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);
			
	private:
		std::vector<BulletShape *> _shapes;
			
		RNDeclareMetaAPI(BulletCompoundShape, BTAPI)
	};
}

#endif /* defined(__RAYNE_BULLETSHAPE_H_) */
