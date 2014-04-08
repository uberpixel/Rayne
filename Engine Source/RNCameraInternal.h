//
//  RNCameraInternal.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAMERAINTERNAL_H__
#define __RAYNE_CAMERAINTERNAL_H__

#include "RNBase.h"
#include "RNCamera.h"

namespace RN
{
	class CubemapCamera : public Camera
	{
	public:
		CubemapCamera(const Vector2& size, RenderStorage *storage, Flags flags, float scaleFactor=0.0f);
		
		bool InFrustum(const Vector3& position, float radius) override;
		
		RNDeclareMeta(CubemapCamera)
	};
}

#endif /* __RAYNE_CAMERAINTERNAL_H__ */
