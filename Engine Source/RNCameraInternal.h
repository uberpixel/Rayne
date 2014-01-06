//
//  RNCameraInternal.h
//  Rayne
//
//  Created by Sidney Just on 06/01/14.
//  Copyright (c) 2014 Sidney Just. All rights reserved.
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
		
		RNDefineMeta(CubemapCamera, Camera)
	};
}

#endif /* __RAYNE_CAMERAINTERNAL_H__ */
