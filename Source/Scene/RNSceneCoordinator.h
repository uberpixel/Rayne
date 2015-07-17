//
//  RNSceneCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SCENECOORDINATOR_H_
#define __RAYNE_SCENECOORDINATOR_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "RNScene.h"

namespace RN
{
	class Kernel;
	class SceneCoordinator
	{
	public:
		friend class Kernel;

		RNAPI static SceneCoordinator *GetSharedInstance();

		RNAPI void AddScene(Scene *scene);
		RNAPI void RemoveScene(Scene *scene);

		RNAPI void Update(float delta);
		RNAPI void Render(Renderer *renderer);

	private:
		SceneCoordinator();
		~SceneCoordinator();

		Array *_scenes;
	};
}


#endif /* __RAYNE_SCENECOORDINATOR_H_ */
