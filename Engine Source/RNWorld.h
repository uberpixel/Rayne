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
#include "RNSceneNode.h"
#include "RNCamera.h"
#include "RNWorldAttachment.h"
#include "RNSceneManager.h"

namespace RN
{
	class Kernel;
	class UIServer;
	
	class World : public NonConstructingSingleton<World>
	{
	friend class SceneNode;
	friend class Kernel;
	friend class Camera;
	friend class UIServer;
	public:
		RNAPI World(SceneManager *sceneManager);
		RNAPI World(const std::string& sceneManager);
		RNAPI virtual ~World();
		
		RNAPI void SceneNodeUpdated(SceneNode *node);
		
		RNAPI void AddAttachment(WorldAttachment *attachment);
		RNAPI void RemoveAttachment(WorldAttachment *attachment);
		
		RNAPI void AddSceneNode(SceneNode *node);
		RNAPI void RemoveSceneNode(SceneNode *node);
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual void NodesUpdated();
		
		RNAPI SceneManager *SceneManager() const { return _sceneManager; }
		
	private:
		static class SceneManager *SceneManagerWithName(const std::string& name);
		void StepWorld(FrameID frame, float delta);
		void ForceInsertNode(SceneNode *node);
		
		void ApplyNodes();
		
		Kernel *_kernel;
		
		Array _attachments;
		
		std::unordered_set<SceneNode *> _nodes;
		std::deque<SceneNode *> _addedNodes;
		
		std::vector<Camera *> _cameras;
		
		Renderer *_renderer;
		class SceneManager *_sceneManager;
		class MetaClass *_cameraClass;
	};
}

#endif /* __RAYNE_WORLD_H__ */
