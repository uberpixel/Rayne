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
		
		_lightClass = RN::Catalogue::SharedInstance()->ClassWithName("RN::Light");
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
			if(node->IsKindOfClass(_lightClass))
			{
				RN::Light *light = static_cast<RN::Light *>(node);
				RN::Debug::DrawBox(node->BoundingBox(), RN::Color::Yellow());
			}
			else
			{
				RN::Debug::DrawBox(node->BoundingBox(), RN::Color::Red());
			}
		}
	}
}
