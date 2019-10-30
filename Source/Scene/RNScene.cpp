//
//  RNScene.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNScene.h"

namespace RN
{
	RNDefineMeta(Scene, Object)
	RNDefineMeta(SceneInfo, Object)

	Scene::Scene() : _attachments(nullptr)
	{
		
	}
	
	Scene::~Scene()
	{
		if(_attachments)
			_attachments->Release();
	}

	void Scene::Update(float delta)
	{
		//Update scene attachments
		if(_attachments)
		{
			_attachments->Enumerate<SceneAttachment>([delta](SceneAttachment *attachment, size_t index, bool &stop) {
				attachment->Update(delta);
			});
		}
	}
	
	void Scene::UpdateNode(SceneNode *node, float delta)
	{
		if(!node->HasFlags(RN::SceneNode::Flags::Static))
		{
			node->Update(delta);
			node->UpdateInternalData();
		}
	}

	void Scene::AddAttachment(SceneAttachment *attachment)
	{
		RN_ASSERT(attachment->_scene == nullptr, "AddAttachment() must be called on an Attachment not owned by the scene");

		if(!_attachments)
			_attachments = new Array();

		_attachments->AddObject(attachment);
		attachment->_scene = this;
	}

	void Scene::RemoveAttachment(SceneAttachment *attachment)
	{
		RN_ASSERT(attachment->_scene == this, "RemoveAttachment() must be called on an Attachment owned by the scene");

        attachment->_scene = nullptr;
		_attachments->RemoveObject(attachment);
	}

	void Scene::WillBecomeActive()
	{}
	void Scene::DidBecomeActive()
	{}

	void Scene::WillResignActive()
	{}
	void Scene::DidResignActive()
	{}

	void Scene::WillUpdate(float delta)
	{}
	void Scene::DidUpdate(float delta)
	{}
	void Scene::WillRender(Renderer *renderer)
	{}
	void Scene::DidRender(Renderer *renderer)
	{}
	
	
	SceneInfo::SceneInfo(Scene *scene) : _scene(scene)
	{
		
	}
	
	SceneInfo::~SceneInfo()
	{
		
	}
	
	Scene *SceneInfo::GetScene() const
	{
		return _scene;
	}
}
