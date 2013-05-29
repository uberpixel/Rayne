//
//  TGDebugDrawer.h
//  Game-osx
//
//  Created by Sidney Just on 29.05.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Game_osx__TGDebugDrawer__
#define __Game_osx__TGDebugDrawer__

#include <Rayne.h>

namespace TG
{
	class DebugDrawer : public RN::WorldAttachment
	{
	public:
		DebugDrawer();
		
		void SetCamera(RN::Camera *camera);
		RN::Camera *Camera() const { return _camera; }
		
		void BeginCamera(RN::Camera *camera) override;
		void WillRenderSceneNode(RN::SceneNode *node) override;
		
	private:
		bool _canDraw;
		RN::Camera *_camera;
		
		RN::MetaClass *_lightClass;
		RN::MetaClass *_cameraClass;
	};
}

#endif /* defined(__Game_osx__TGDebugDrawer__) */
