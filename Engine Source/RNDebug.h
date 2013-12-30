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
		void AddLinePoint(const Vector3& point, const Color& color);
		void AddLinePoint(const Vector2& point, const Color& color);
		void CloseLine();
		void EndLine();
		
		void DrawBox(const AABB& box, const Color& color);
		void DrawBox(const Vector3& min, const Vector3& max, const Color& color);
		
		void DrawSphere(const Sphere& sphere, const Color& color, const int tesselation = 20);
		void DrawSphere(const Vector3 &pos, const float radius, const Color &color, const int tesselation = 20);
	}
}

#endif /* __RAYNE_DEBUG_H__ */
