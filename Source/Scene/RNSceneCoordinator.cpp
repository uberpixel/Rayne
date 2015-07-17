//
//  RNSceneCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneCoordinator.h"

namespace RN
{
	static SceneCoordinator *__sharedInstance;

	SceneCoordinator::SceneCoordinator() :
		_scenes(new Array())
	{
		__sharedInstance = this;
	}
	SceneCoordinator::~SceneCoordinator()
	{
		_scenes->Release();
		__sharedInstance = nullptr;
	}

	SceneCoordinator *SceneCoordinator::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void SceneCoordinator::AddScene(Scene *scene)
	{
		_scenes->AddObject(scene);
	}
	void SceneCoordinator::RemoveScene(Scene *scene)
	{
		_scenes->RemoveObject(scene);
	}

	void SceneCoordinator::Update(float delta)
	{
		_scenes->Enumerate<Scene>([&](Scene *scene, size_t index, bool &stop) {
			scene->Update(delta);
		});
	}
	void SceneCoordinator::Render(Renderer *renderer)
	{
		_scenes->Enumerate<Scene>([&](Scene *scene, size_t index, bool &stop) {
			scene->Render(renderer);
		});
	}
}
