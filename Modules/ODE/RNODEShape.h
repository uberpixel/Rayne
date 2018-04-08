//
//  RNODEShape.h
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ODESHAPE_H_
#define __RAYNE_ODESHAPE_H_

#include "RNODE.h"

namespace RN
{
	class Mesh;

	class ODEShape : public Object
	{
	public:
			
	protected:
		ODEShape();
		~ODEShape() override;
			
		RNDeclareMetaAPI(ODEShape, ODEAPI)
	};
		
	class ODESphereShape : public ODEShape
	{
	public:
		ODEAPI ODESphereShape(float radius);
		float GetRadius() const { return _radius; }

		ODEAPI static ODESphereShape *WithRadius(float radius);

	private:
		float _radius;
			
		RNDeclareMetaAPI(ODESphereShape, ODEAPI)
	};
		
	class ODEBoxShape : public ODEShape
	{
	public:
		ODEAPI ODEBoxShape(const Vector3 &halfExtents);
		const Vector3 &GetHalfExtends() const { return _halfExtents; }
			
		ODEAPI static ODEBoxShape *WithHalfExtents(const Vector3& halfExtents);

	private:
		Vector3 _halfExtents;
			
		RNDeclareMetaAPI(ODEBoxShape, ODEAPI)
	};
		
	class ODEStaticPlaneShape : public ODEShape
	{
	public:
		ODEAPI ODEStaticPlaneShape(const Vector3 &normal, float constant);
		const Vector4 &GetPlane() const { return _plane; }
			
		ODEAPI static ODEStaticPlaneShape *WithNormal(const Vector3 &normal, float constant);

	private:
		RN::Vector4 _plane;
			
		RNDeclareMetaAPI(ODEStaticPlaneShape, ODEAPI)
	};
		
	class ODETriangleMeshShape : public ODEShape
	{
	public:
		ODEAPI ODETriangleMeshShape(Model *model, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		ODEAPI ODETriangleMeshShape(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		ODEAPI ODETriangleMeshShape(const Array *meshes, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
			
		ODEAPI ~ODETriangleMeshShape() override;
			
		ODEAPI static ODETriangleMeshShape *WithModel(Model *model, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));

		uint32 GetVertexCount() const { return _vertices.size(); }
		uint32 GetIndexCount() const { return _indices.size(); }
		const float *GetVertices() const { return &_vertices[0].x; }
		const float *GetNormals() const { return &_normals[0].x; }
		const uint32 *GetIndices() const { return &_indices[0]; }
			
	private:
		void AddMesh(Mesh *mesh, Vector3 scale = Vector3(1.0f, 1.0f, 1.0f));
		std::vector<Vector3> _vertices;
		std::vector<Vector3> _normals;
		std::vector<uint32> _indices;
			
		RNDeclareMetaAPI(ODETriangleMeshShape, ODEAPI)
	};
	/*
	class ODEConvexHullShape : public ODEShape
	{
	public:
		ODEAPI ODEConvexHullShape(Mesh *mesh);

		ODEAPI static ODEConvexHullShape *WithMesh(Mesh *mesh);

	private:


		RNDeclareMetaAPI(ODEConvexHullShape, ODEAPI)
	};*/
		
	class ODECompoundShape : public ODEShape
	{
	public:
		ODEAPI ODECompoundShape();
		ODEAPI ~ODECompoundShape();

		ODEAPI void AddChild(ODEShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);
			
	private:
		std::vector<ODEShape *> _shapes;
			
		RNDeclareMetaAPI(ODECompoundShape, ODEAPI)
	};
}

#endif /* defined(__RAYNE_ODESHAPE_H_) */
