//
//  RNSceneManager.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEMANAGER_H__
#define __RAYNE_SCENEMANAGER_H__

#include "RNBase.h"
#include "RNTransform.h"
#include "RNCamera.h"
#include "RNEntity.h"
#include "RNRenderer.h"

namespace RN
{
	class SceneManager : public Object
	{
	public:
		virtual void AddTransform(Transform *transform) = 0;
		virtual void RemoveTransform(Transform *transform) = 0;
		virtual void UpdateTransform(Transform *transform) = 0;
		
		virtual void RenderTransforms(Camera *camera) = 0;
		
	protected:
		SceneManager();
		virtual ~SceneManager();
		
		Renderer *_renderer;
		
		RNDefineConstructorlessMeta(SceneManager, Object)
	};
	
	class GenericSceneManager : public SceneManager
	{
	public:
		GenericSceneManager();
		virtual ~GenericSceneManager();
		
		virtual void AddTransform(Transform *transform);
		virtual void RemoveTransform(Transform *transform);
		virtual void UpdateTransform(Transform *transform);
		
		virtual void RenderTransforms(Camera *camera);
		
	private:
		void RenderTransform(Camera *camera, Transform *transform);
		
		std::unordered_set<Transform *> _transforms;
		class MetaClass *_entityClass;
		class MetaClass *_lightClass;
		
		RNDefineMeta(GenericSceneManager, SceneManager);
	};
}

#endif /* __RAYNE_SCENEMANAGER_H__ */
