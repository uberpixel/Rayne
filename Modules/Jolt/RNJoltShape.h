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
		JTAPI Vector3 GetCenterOfMass() const;
		JTAPI AABB GetLocalBounds() const;
		JTAPI virtual void SetMass(float mass);
		JTAPI void SetPose(RN::Vector3 positionOffset, RN::Quaternion rotationOffset);

	protected:
		JoltShape();
		~JoltShape() override;

		JPH::Shape *_shape;

		RNDeclareMetaAPI(JoltShape, JTAPI)
	};

	class JoltSphereShape : public JoltShape
	{
	public:
		JTAPI JoltSphereShape(float radius);

		JTAPI static JoltSphereShape *WithRadius(float radius);

		RNDeclareMetaAPI(JoltSphereShape, JTAPI)
	};

	class JoltBoxShape : public JoltShape
	{
	public:
		JTAPI JoltBoxShape(const Vector3 &halfExtents, float convexRadius = 0.05f);

		JTAPI static JoltBoxShape *WithHalfExtents(const Vector3 &halfExtents, float convexRadius = 0.05f);

		RNDeclareMetaAPI(JoltBoxShape, JTAPI)
	};

	class JoltCapsuleShape : public JoltShape
	{
	public:
		JTAPI JoltCapsuleShape(float radius, float height);

		JTAPI static JoltCapsuleShape *WithRadius(float radius, float height);

		RNDeclareMetaAPI(JoltCapsuleShape, JTAPI)
	};

	class JoltCylinderShape : public JoltShape
	{
	public:
		JTAPI JoltCylinderShape(float radius, float height, float convexRadius = 0.05f);

		JTAPI static JoltCylinderShape *WithRadius(float radius, float height, float convexRadius = 0.05f);

		RNDeclareMetaAPI(JoltCylinderShape, JTAPI)
	};

	class JoltWheelCylinderShape : public JoltShape
	{
	public:
		JTAPI JoltWheelCylinderShape(float radius, float height, float convexRadius = 0.05f);

		JTAPI static JoltWheelCylinderShape *WithRadius(float radius, float height, float convexRadius = 0.05f);

		RNDeclareMetaAPI(JoltWheelCylinderShape, JTAPI)
	};

	class JoltTriangleMeshShape : public JoltShape
	{
	public:
		JTAPI JoltTriangleMeshShape(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), bool wantsDoubleSided = false);

		JTAPI static JoltTriangleMeshShape *WithMesh(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), bool wantsDoubleSided = false);

	private:
		RNDeclareMetaAPI(JoltTriangleMeshShape, JTAPI)
	};

	class JoltHeightFieldShape : public JoltShape
	{
	public:
		JTAPI JoltHeightFieldShape(const float *samples, uint32 sampleCount, const Vector3 &offset, const Vector3 &scale, uint32 blockSize = 2, uint32 bitsPerSample = 8);

		JTAPI static JoltHeightFieldShape *WithSamples(const float *samples, uint32 sampleCount, const Vector3 &offset, const Vector3 &scale, uint32 blockSize = 2, uint32 bitsPerSample = 8);

	private:
		RNDeclareMetaAPI(JoltHeightFieldShape, JTAPI)
	};

	class JoltConvexHullShape : public JoltShape
	{
	public:
		JTAPI JoltConvexHullShape(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), float convexRadius = 0.05f);

		JTAPI static JoltConvexHullShape *WithMesh(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f), float convexRadius = 0.05f);

	private:
		RNDeclareMetaAPI(JoltConvexHullShape, JTAPI)
	};

	class JoltCompoundShape : public JoltShape
	{
	public:
		//friend JoltDynamicBody;
		//friend JoltStaticBody;
		struct Child
		{
			JoltShape *shape;
			Vector3 position;
			Quaternion rotation;
			float mass;
			uint32 userData;
		};

		// Batches flat or disjoint uniformly scaled children; preserves incremental updates otherwise.
		// GetJoltShape() is null if validation fails.
		JTAPI JoltCompoundShape(const std::vector<Child> &children);
		JTAPI JoltCompoundShape();
		JTAPI JoltCompoundShape(Model *model, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
		JTAPI JoltCompoundShape(const Array *meshes, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
		JTAPI ~JoltCompoundShape();

		JTAPI void AddChild(Mesh *mesh, const RN::Vector3 &position, const RN::Quaternion &rotation, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);
		JTAPI void AddChild(Mesh *mesh, const RN::Vector3 &position, const RN::Quaternion &rotation, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided, uint32 userData);
		JTAPI void AddChild(JoltShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);
		JTAPI void AddChild(JoltShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation, uint32 userData);
		JTAPI bool SetChildMass(size_t index, float mass);
		JTAPI void SetMass(float mass) override;

		JoltShape *GetShape(size_t index) const { return _shapes[index]; }
		size_t GetNumberOfShapes() const { return _shapes.size(); }

		JTAPI static JoltCompoundShape *WithModel(Model *model, Vector3 scale, bool useTriangleMesh, bool wantsDoubleSided = false);

	private:
		static bool CanScaleMassUniformly(JoltShape *shape, float mass, std::unordered_set<const JPH::Shape *> &visited);
		void UpdateCenterOfMass();

		std::vector<JoltShape *> _shapes;

		RNDeclareMetaAPI(JoltCompoundShape, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTSHAPE_H_) */
