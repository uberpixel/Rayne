//
//  RNJoltShape.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTSHAPE_H_
#define __RAYNE_JOLTSHAPE_H_

#include "RNJolt.h"
#include "RNJoltMaterial.h"

namespace JPH
{
	class Shape;
}

namespace RN
{
	class Mesh;
	//class JoltDynamicBody;
	//class JoltStaticBody;

	class JoltShape : public Object
	{
	public:
		JoltShape(JPH::Shape *shape);
			
		JTAPI JPH::Shape *GetJoltShape() const { return _shape; }
		JTAPI void SetPose(RN::Vector3 positionOffset, RN::Quaternion rotationOffset);
			
	protected:
		JoltShape();
		~JoltShape() override;
			
		JPH::Shape *_shape;
		JoltMaterial *_material;
			
		RNDeclareMetaAPI(JoltShape, JTAPI)
	};
		
	class JoltSphereShape : public JoltShape
	{
	public:
		JTAPI JoltSphereShape(float radius, JoltMaterial *material);
			
		JTAPI static JoltSphereShape *WithRadius(float radius, JoltMaterial *material);
			
		RNDeclareMetaAPI(JoltSphereShape, JTAPI)
	};
		
	class JoltBoxShape : public JoltShape
	{
	public:
		JTAPI JoltBoxShape(const Vector3 &halfExtents, JoltMaterial *material);
			
		JTAPI static JoltBoxShape *WithHalfExtents(const Vector3& halfExtents, JoltMaterial *material);
			
		RNDeclareMetaAPI(JoltBoxShape, JTAPI)
	};
		
	class JoltCapsuleShape : public JoltShape
	{
	public:
		JTAPI JoltCapsuleShape(float radius, float height, JoltMaterial *material);
			
		JTAPI static JoltCapsuleShape *WithRadius(float radius, float height, JoltMaterial *material);
			
		RNDeclareMetaAPI(JoltCapsuleShape, JTAPI)
	};
		
	class JoltTriangleMeshShape : public JoltShape
	{
	public:
		JTAPI JoltTriangleMeshShape(Mesh *mesh, JoltMaterial *material, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), bool wantsDoubleSided = false);
			
		JTAPI static JoltTriangleMeshShape *WithMesh(Mesh *mesh, JoltMaterial *material, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), bool wantsDoubleSided = false);
			
	private:
		RNDeclareMetaAPI(JoltTriangleMeshShape, JTAPI)
	};

	class JoltConvexHullShape : public JoltShape
	{
	public:
		JTAPI JoltConvexHullShape(Mesh *mesh, JoltMaterial *material, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));

		JTAPI static JoltConvexHullShape *WithMesh(Mesh *mesh, JoltMaterial *material, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));

	private:

		RNDeclareMetaAPI(JoltConvexHullShape, JTAPI)
	};
		
	class JoltCompoundShape : public JoltShape
	{
	public:
		//friend JoltDynamicBody;
		//friend JoltStaticBody;
		JTAPI JoltCompoundShape();
		JTAPI JoltCompoundShape(Model *model, JoltMaterial *material, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
		JTAPI JoltCompoundShape(const Array *meshes, JoltMaterial *material, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
		JTAPI ~JoltCompoundShape();

		JTAPI void AddChild(Mesh *mesh, JoltMaterial *material, const RN::Vector3 &position, const RN::Quaternion &rotation, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
		JTAPI void AddChild(JoltShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);
		
		JoltShape *GetShape(size_t index) const { return _shapes[index]; }
		RN::Vector3 GetPosition(size_t index) const { return _positions[index]; }
		RN::Quaternion GetRotation(size_t index) const { return _rotations[index]; }
		size_t GetNumberOfShapes() const { return _shapes.size(); }

		JTAPI static JoltCompoundShape *WithModel(Model *model, JoltMaterial *material, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
			
	private:
		std::vector<JoltShape *> _shapes;
		std::vector<RN::Vector3> _positions;
		std::vector<RN::Quaternion> _rotations;
			
		RNDeclareMetaAPI(JoltCompoundShape, JTAPI)
	};
}

#endif /* defined(__RAYNE_JOLTSHAPE_H_) */
