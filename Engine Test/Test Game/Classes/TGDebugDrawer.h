//
//  TGDebugDrawer.h
//  Game-osx
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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
		RN::Camera *GetCamera() const { return _camera; }
		
		void DidBeginCamera(RN::Camera *camera) override;
		void WillRenderSceneNode(RN::SceneNode *node) override;
		
	private:
		bool _canDraw;
		RN::Camera *_camera;
		
		RN::MetaClassBase *_lightClass;
		RN::MetaClassBase *_cameraClass;
	};
}

#endif /* defined(__Game_osx__TGDebugDrawer__) */
