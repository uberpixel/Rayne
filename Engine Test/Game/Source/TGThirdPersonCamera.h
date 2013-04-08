//
//  TGThirdPersonCamera.h
//  Game-osx
//
//  Created by Sidney Just on 07.04.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Game__TGThirdPersonCamera__
#define __Game__TGThirdPersonCamera__

#include <Rayne.h>

namespace TG
{
	class ThirdPersonCamera : public RN::Camera
	{
	public:
		ThirdPersonCamera(RN::RenderStorage *storage);
		virtual ~ThirdPersonCamera();
	
		void SetTarget(RN::Entity *target);
		virtual void Update(float delta);
		virtual bool CanUpdate(FrameID frame);
		
	private:
		RN::Entity *_target;
		float _distance;
		
		RNDefineConstructorlessMeta(ThirdPersonCamera, RN::Camera)
	};
}

#endif /* defined(__Game__TGThirdPersonCamera__) */
