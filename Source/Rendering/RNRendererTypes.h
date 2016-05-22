//
//  RNRendererTypes.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERERTYPES_H_
#define __RAYNE_RENDERERTYPES_H_

#include "../Base/RNBase.h"

namespace RN
{
	enum class DrawMode
	{
		Point,
		Line,
		LineStrip,
		Triangle,
		TriangleStrip
	};

	enum class PrimitiveType
	{
		Uint8,
		Uint16,
		Uint32,

		Int8,
		Int16,
		Int32,

		Float,

		Vector2,
		Vector3,
		Vector4,

		Matrix,
		Quaternion,
		Color
	};
}

#endif /* __RAYNE_RENDERERTYPES_H_ */
