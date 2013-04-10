//
//  RNWorld.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLD_H__
#define __RAYNE_WORLD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNRenderer.h"
#include "RNTransform.h"
#include "RNCamera.h"
#include "RNWorldAttachment.h"
#include "RNSceneManager.h"

namespace RN
{
	class Kernel;
	class Transform;
	
	class World : public NonConstructingSingleton<World>
	{
	friend class Transform;
	friend class Kernel;
	friend class Camera;
	public:
		RNAPI World(SceneManager *sceneManager);
		RNAPI World(const std::string& sceneManager);
		RNAPI virtual ~World();
		
		RNAPI void TransformUpdated(Transform *transform);
		
		RNAPI void AddAttachment(WorldAttachment *attachment);
		RNAPI void RemoveAttachment(WorldAttachment *attachment);
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual void TransformsUpdated();
		
		RNAPI SceneManager *SceneManager() const { return _sceneManager; }
		
	private:
		static class SceneManager *SceneManagerWithName(const std::string& name);
		void StepWorld(FrameID frame, float delta);
		
		void AddTransform(Transform *transform);
		void RemoveTransform(Transform *transform);
		
		void AddCamera(Camera *camera);
		void RemoveCamera(Camera *camera);
		
		Kernel *_kernel;
		
		Array<WorldAttachment> _attachments;
		
		std::unordered_set<Transform *> _transforms;
		std::vector<Camera *> _cameras;
		
		Renderer *_renderer;
		class SceneManager *_sceneManager;
	};
}

#endif /* __RAYNE_WORLD_H__ */
