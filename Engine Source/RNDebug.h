//
//  RNDebug.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEBUG_H__
#define __RAYNE_DEBUG_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNColor.h"
#include "RNAABB.h"
#include "RNSphere.h"

namespace RN
{
	namespace Debug
	{
		RNAPI void AddLinePoint(const Vector3 &point, const Color &color);
		RNAPI void AddLinePoint(const Vector2 &point, const Color &color);
		RNAPI void CloseLine();
		RNAPI void EndLine();
		
		RNAPI void DrawBox(const AABB &box, const Color &color);
		RNAPI void DrawBox(const Vector3 &min, const Vector3 &max, const Color &color);
		
		RNAPI void DrawSphere(const Sphere &sphere, const Color &color, const int tesselation = 20);
		RNAPI void DrawSphere(const Vector3 &pos, const float radius, const Color &color, const int tesselation = 20);
	}
}

#endif /* __RAYNE_DEBUG_H__ */
