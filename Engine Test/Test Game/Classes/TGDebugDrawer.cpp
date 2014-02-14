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
		
		_lightClass  = RN::Light::MetaClass();
		_cameraClass = RN::Camera::MetaClass();
	}
	
	void DebugDrawer::SetCamera(RN::Camera *camera)
	{
		_camera = camera;
	}
	
	
	
	void DebugDrawer::DidBeginCamera(RN::Camera *camera)
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
				float distance = _camera->GetWorldPosition().GetDistance(node->GetWorldPosition());
				float tessellation = ((_camera->GetClipFar() - distance) / _camera->GetClipFar()) * 15.0f;
				
				tessellation = std::max(5.0f, tessellation);
				
				RN::Debug::DrawSphere(node->GetBoundingSphere(), RN::Color::Yellow(), static_cast<int>(floorf(tessellation)));
			}
			else
			{
				RN::Debug::DrawBox(node->GetBoundingBox(), RN::Color::Red());
			}
		}
	}
}
