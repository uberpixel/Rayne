//
//  RNSceneManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneManager.h"

namespace RN
{
	static SceneManager *__sharedInstance;

	SceneManager::SceneManager() :
		_scenes(new Array())
	{
		__sharedInstance = this;
	}
	SceneManager::~SceneManager()
	{
		_scenes->Release();
		__sharedInstance = nullptr;
	}

	SceneManager *SceneManager::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void SceneManager::AddScene(Scene *scene)
	{
		_scenes->AddObject(scene);
	}
	void SceneManager::RemoveScene(Scene *scene)
	{
		_scenes->RemoveObject(scene);
	}

	void SceneManager::Update(float delta)
	{
		_scenes->Enumerate<Scene>([&](Scene *scene, size_t index, bool &stop) {
			scene->Update(delta);
		});
	}
	void SceneManager::Render(Renderer *renderer)
	{
		_scenes->Enumerate<Scene>([&](Scene *scene, size_t index, bool &stop) {
			scene->Render(renderer);
		});
	}
}
