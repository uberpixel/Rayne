//
//  RNSteamAudioListener.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioListener.h"
#include "RNSteamAudioWorld.h"

namespace RN
{
	RNDefineMeta(SteamAudioListener, SceneNodeAttachment)
		
		SteamAudioListener::SteamAudioListener() :
		_owner(nullptr)
	{
			
	}
		
	SteamAudioListener::~SteamAudioListener()
	{
			
	}
		
	void SteamAudioListener::ReInsertIntoWorld()
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
		
	void SteamAudioListener::InsertIntoWorld(SteamAudioWorld *world)
	{
		_owner = world;
	}
		
	void SteamAudioListener::RemoveFromWorld()
	{
		_owner = nullptr;
	}
		
	void SteamAudioListener::Update(float delta)
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

		//TODO: Use listener properties
	}
		
	void SteamAudioListener::DidUpdate(SceneNode::ChangeSet changeSet)
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
		
/*	void SteamAudioListener::DidAddToParent()
	{
		if(!_owner)
			AudioWorld::GetSharedInstance()->SetAudioListener(this);
	}
		
	void SteamAudioListener::WillRemoveFromParent()
	{
		if(_owner)
			_owner->SetAudioListener(nullptr);
	}*/
}
