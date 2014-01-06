//
//  RNCameraInternal.cpp
//  Rayne
//
//  Created by Sidney Just on 06/01/14.
//  Copyright (c) 2014 Sidney Just. All rights reserved.
//

#include "RNCameraInternal.h"

namespace RN
{
	RNDeclareMeta(CubemapCamera)
	
	CubemapCamera::CubemapCamera(const Vector2 &size, RenderStorage *storage, Flags flags, float scaleFactor) :
		Camera(size, storage, flags, scaleFactor)
	{}
	
	bool CubemapCamera::InFrustum(const RN::Vector3 &position, float radius)
	{
		return (GetWorldPosition().Distance(position) < clipfar + radius);
	}
}
