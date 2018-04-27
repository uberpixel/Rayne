//
//  RNSplashShape.h
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPLASHSHAPE_H_
#define __RAYNE_SPLASHSHAPE_H_

#include "RNSplash.h"

namespace RN
{
	class Mesh;

	class SplashShape : public Object
	{
	public:
		SplashShape();

		virtual SplashShape *GetTransformedCopy(const Matrix &transformation) const;
		virtual const Vector3 &GetClosestDistanceVector(SplashShape *other);
			
	protected:
		~SplashShape() override;
			
		RNDeclareMetaAPI(SplashShape, SPAPI)
	};

	class SplashConvexHullShape : public SplashShape
	{
	public:
		SPAPI SplashConvexHullShape(Mesh *mesh);
		SPAPI SplashConvexHullShape(Model *model);

		SplashShape *GetTransformedCopy(const Matrix &transformation) const final;
		const Vector3 &GetClosestDistanceVector(SplashShape *other) final;

		SPAPI static SplashConvexHullShape *WithMesh(Mesh *mesh);
		SPAPI static SplashConvexHullShape *WithModel(Model *model);

	private:
		struct HullPlane
		{
			Plane plane;
			uint32 indices[3];
		};

		SPAPI SplashConvexHullShape();

		void AddMesh(std::vector<Vector3> &vertices, Mesh *mesh);
		void SetVertices(const std::vector<Vector3> &vertices);

		std::vector<Vector3> _vertices;
		std::vector<uint32> _indices;

		RNDeclareMetaAPI(SplashConvexHullShape, SPAPI)
	};
		
	class SplashCompoundShape : public SplashShape
	{
	public:
		SPAPI SplashCompoundShape();
		SPAPI SplashCompoundShape(Model *model);
		SPAPI SplashCompoundShape(const Array *meshes);
		SPAPI ~SplashCompoundShape();

		SPAPI void AddChild(SplashShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation);

		SplashShape *GetTransformedCopy(const Matrix &transformation) const final;
		const Vector3 &GetClosestDistanceVector(SplashShape *other) const final;

		SPAPI static SplashCompoundShape *WithModel(Model *model);
			
	private:
		std::vector<SplashShape *> _shapes;
			
		RNDeclareMetaAPI(SplashCompoundShape, SPAPI)
	};
}

#endif /* defined(__RAYNE_SPLASHSHAPE_H_) */
