//
//  RNOpenALListener.cpp
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenALListener.h"
#include "RNOpenALWorld.h"

#include "AL/al.h"
#include "AL/alc.h"

namespace RN
{
	RNDefineMeta(OpenALListener, SceneNodeAttachment)
		
	OpenALListener::OpenALListener() :
		_owner(nullptr)
	{
			
	}
		
	OpenALListener::~OpenALListener()
	{
			
	}
		
	void OpenALListener::ReInsertIntoWorld()
	{
		if(_owner)
		{
			auto world = _owner;
			world->Lock();
			world->SetListener(nullptr);
			world->SetListener(this);
			world->Unlock();
		}
	}
		
	void OpenALListener::InsertIntoWorld(OpenALWorld *world)
	{
		_owner = world;
	}
		
	void OpenALListener::RemoveFromWorld()
	{
		_owner = nullptr;
	}
		
	void OpenALListener::Update(float delta)
	{
		if(!_owner || !GetParent() || delta <= 0.0f)
			return;
			
		Vector3 position = GetWorldPosition();
		Vector3 velocity = position-_oldPosition;
		velocity /= delta;
		_oldPosition = position;
			
		Vector3 orientation[2];
		orientation[0] = GetForward();
		orientation[1] = GetUp();
			
		alListenerfv(AL_POSITION, &position.x);
		alListenerfv(AL_VELOCITY, &velocity.x);
		alListenerfv(AL_ORIENTATION, &orientation[0].x);
	}
		
	void OpenALListener::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNodeAttachment::DidUpdate(changeSet);
/*		if(changeSet & SceneNode::ChangeSet::World)
		{
			World *world = GetParent()->GetWorld();
				
			if(!world && _owner)
			{
				_owner->SetAudioListener(nullptr);
				return;
			}
				
			if(world && !_owner)
			{
				AudioWorld::GetSharedInstance()->SetAudioListener(this);
				return;
			}
		}*/
	}
		
/*	void OpenALListener::DidAddToParent()
	{
		if(!_owner)
			AudioWorld::GetSharedInstance()->SetAudioListener(this);
	}
		
	void OpenALListener::WillRemoveFromParent()
	{
		if(_owner)
			_owner->SetAudioListener(nullptr);
	}*/
}
