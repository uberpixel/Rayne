//
//  TGDebugDrawer.cpp
//  Game-osx
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGDebugDrawer.h"

namespace TG
{
	DebugDrawer::DebugDrawer()
	{
		_canDraw = false;
		_camera = nullptr;
		
		_lightClass  = RN::Catalogue::GetSharedInstance()->GetClassWithName("RN::Light");
		_cameraClass = RN::Catalogue::GetSharedInstance()->GetClassWithName("RN::Camera");
	}
	
	void DebugDrawer::SetCamera(RN::Camera *camera)
	{
		_camera = camera;
	}
	
	
	
	void DebugDrawer::BeginCamera(RN::Camera *camera)
	{
		_canDraw = (camera == _camera);
	}
	
	void DebugDrawer::WillRenderSceneNode(RN::SceneNode *node)
	{
		if(_canDraw)
		{
			if(node->IsKindOfClass(_cameraClass))
				return;
			
			if(node->IsKindOfClass(_lightClass))
			{
				RN::Debug::DrawSphere(node->GetBoundingSphere(), RN::Color::Yellow(), 10);
//				RN::Debug::DrawBox(node->BoundingBox(), RN::Color::Blue());
			}
			else
			{
//				RN::Debug::DrawSphere(node->BoundingSphere(), RN::Color::Green());
				RN::Debug::DrawBox(node->GetBoundingBox(), RN::Color::Red());
			}
		}
	}
}
