//
//  RNDebug.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEBUG_H__
#define __RAYNE_DEBUG_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNColor.h"
#include "RNAABB.h"

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
	}
}

#endif /* __RAYNE_DEBUG_H__ */
