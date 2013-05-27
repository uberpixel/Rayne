//
//  RNDebug.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEBUG_H__
#define __RAYNE_DEBUG_H__

#include "RNDebug.h"
#include "RNVector.h"
#include "RNColor.h"

namespace RN
{
	namespace Debug
	{
		void AddLinePoint(const Vector3& point, const Color& color);
		void EndLine();
	}
}

#endif /* __RAYNE_DEBUG_H__ */
