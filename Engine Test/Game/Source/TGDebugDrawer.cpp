//
//  TGDebugDrawer.cpp
//  Game-osx
//
//  Created by Sidney Just on 29.05.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGDebugDrawer.h"

namespace TG
{
	DebugDrawer::DebugDrawer()
	{
		_canDraw = false;
		_camera = nullptr;
		
		_lightClass  = RN::Catalogue::SharedInstance()->ClassWithName("RN::Light");
		_cameraClass = RN::Catalogue::SharedInstance()->ClassWithName("RN::Camera");
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
				RN::Debug::DrawSphere(node->BoundingSphere(), RN::Color::Yellow(), 10);
//				RN::Debug::DrawBox(node->BoundingBox(), RN::Color::Blue());
			}
			else
			{
//				RN::Debug::DrawSphere(node->BoundingSphere(), RN::Color::Green());
				RN::Debug::DrawBox(node->BoundingBox(), RN::Color::Red());
			}
		}
	}
}
