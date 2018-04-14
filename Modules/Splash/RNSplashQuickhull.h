//
//  RNSplashQuickhull.h
//  Rayne-Quickhull
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPLASHQUICKHULL_H_
#define __RAYNE_SPLASHQUICKHULL_H_

#include "RNSplash.h"

namespace RN
{
	class Mesh;

	class SplashQuickhull
	{
	public:
		struct HullPlane
		{
			Plane plane;
			uint32 indices[3];
		};

		SplashQuickhull();
		SplashQuickhull(const std::vector<Vector3> &vertices);
		void SetVertices(const std::vector<Vector3> &vertices);

		const std::vector<Vector3> &GetVertices() const { return _vertices; }
		const std::vector<uint32> &GetIndices() const { return _indices; }
		const Vector3 &GetCenter() const { return _center; }

	private:
		std::vector<Vector3> _vertices;
		std::vector<uint32> _indices;
		Vector3 _center;
	};
}

#endif /* defined(__RAYNE_SPLASHQUICKHULL_H_) */
