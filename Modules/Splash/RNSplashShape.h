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
			
	protected:
		~SplashShape() override;
			
		RNDeclareMetaAPI(SplashShape, SPAPI)
	};
		
	class SplashSphereShape : public SplashShape
	{
	public:
		SPAPI SplashSphereShape(float radius);
			
		SPAPI static SplashSphereShape *WithRadius(float radius);
			
		RNDeclareMetaAPI(SplashSphereShape, SPAPI)
	};
		
	class SplashBoxShape : public SplashShape
	{
	public:
		SPAPI SplashBoxShape(const Vector3 &halfExtents);
			
		SPAPI static SplashBoxShape *WithHalfExtents(const Vector3& halfExtents);
			
		RNDeclareMetaAPI(SplashBoxShape, SPAPI)
	};

	class SplashConvexHullShape : public SplashShape
	{
	public:
		SPAPI SplashConvexHullShape(Mesh *mesh);

		SPAPI static SplashConvexHullShape *WithMesh(Mesh *mesh);

	private:

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

		SPAPI static SplashCompoundShape *WithModel(Model *model);
			
	private:
		std::vector<SplashShape *> _shapes;
			
		RNDeclareMetaAPI(SplashCompoundShape, SPAPI)
	};
}

#endif /* defined(__RAYNE_SPLASHSHAPE_H_) */
