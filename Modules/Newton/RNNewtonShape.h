//
//  RNNewtonShape.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONSHAPE_H_
#define __RAYNE_NEWTONSHAPE_H_

#include "RNNewton.h"

class NewtonCollision;

namespace RN
{
	class Mesh;

	class NewtonShape : public Object
	{
	public:
		NewtonShape(NewtonCollision *shape);
			
		NDAPI NewtonCollision *GetNewtonShape() const { return _shape; }
			
	protected:
		NewtonShape();
		~NewtonShape() override;
			
		NewtonCollision *_shape;
			
		RNDeclareMetaAPI(NewtonShape, NDAPI)
	};
		
	class NewtonSphereShape : public NewtonShape
	{
	public:
		NDAPI NewtonSphereShape(float radius);
			
		NDAPI static NewtonSphereShape *WithRadius(float radius);
			
		RNDeclareMetaAPI(NewtonSphereShape, NDAPI)
	};
		
	class NewtonBoxShape : public NewtonShape
	{
	public:
		NDAPI NewtonBoxShape(const Vector3 &halfExtents);
			
		NDAPI static NewtonBoxShape *WithHalfExtents(const Vector3& halfExtents);
			
		RNDeclareMetaAPI(NewtonBoxShape, NDAPI)
	};
		
	class NewtonCapsuleShape : public NewtonShape
	{
	public:
		NDAPI NewtonCapsuleShape(float radius, float height);
			
		NDAPI static NewtonCapsuleShape *WithRadius(float radius, float height);
			
		RNDeclareMetaAPI(NewtonCapsuleShape, NDAPI)
	};
		
	class NewtonTriangleMeshShape : public NewtonShape
	{
	public:
		NDAPI NewtonTriangleMeshShape(Model *model, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		NDAPI NewtonTriangleMeshShape(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		NDAPI NewtonTriangleMeshShape(const Array *meshes, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		NDAPI NewtonTriangleMeshShape(const String *filename);

		NDAPI void SerializeToFile(const String *filename);

		NDAPI static NewtonTriangleMeshShape *WithModel(Model *model, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		NDAPI static NewtonTriangleMeshShape *WithFile(const String *filename);

	private:
		void AddMesh(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));

		RNDeclareMetaAPI(NewtonTriangleMeshShape, NDAPI)
	};

	class NewtonConvexHullShape : public NewtonShape
	{
	public:
		NDAPI NewtonConvexHullShape(Mesh *mesh);

		NDAPI static NewtonConvexHullShape *WithMesh(Mesh *mesh);

	private:

		RNDeclareMetaAPI(NewtonConvexHullShape, NDAPI)
	};
		
	class NewtonCompoundShape : public NewtonShape
	{
	public:
		NDAPI NewtonCompoundShape();
		NDAPI NewtonCompoundShape(Model *model);
		NDAPI NewtonCompoundShape(const Array *meshes);
		NDAPI ~NewtonCompoundShape();

		void AddChild(NewtonShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);

		NDAPI static NewtonCompoundShape *WithModel(Model *model);
			
	private:
		std::vector<NewtonShape *> _shapes;
		std::vector<RN::Vector3> _positions;
		std::vector<RN::Quaternion> _rotations;

		bool _isEditMode;
			
		RNDeclareMetaAPI(NewtonCompoundShape, NDAPI)
	};
}

#endif /* defined(__RAYNE_NEWTONSHAPE_H_) */
