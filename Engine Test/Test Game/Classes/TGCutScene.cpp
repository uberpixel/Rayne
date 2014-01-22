//
//  TGCutScene.cpp
//  Test Game
//
//  Created by Sidney Just on 22/01/14.
//  Copyright (c) 2014 Überpixel. All rights reserved.
//

#include "TGCutScene.h"

namespace TG
{
	Animation::Animation() :
		_duration(1.0f),
		_beginTime(0.0f),
		_offset(0.0f)
	{}
	
	Animation::~Animation()
	{}
	
	void Animation::SetBeginTime(float time)
	{
		_beginTime = time;
	}
	void Animation::SetDuration(float duration)
	{
		_duration = duration;
	}
	void Animation::SetTimeOffset(float offset)
	{
		_offset = offset;
	}
	
	
	
	CutScene::CutScene() :
		_animations(new RN::Array()),
		_time(0.0f)
	{
	}
	CutScene::~CutScene()
	{
		RN::SafeRelease(_animations);
	}
	
	
	void CutScene::AddAnimation(Animation *animation)
	{
		_animations->AddObject(animation);
	}
	void CutScene::Reset()
	{
		_time = 0.0f;
		_animations->RemoveAllObjects();
	}
	
	void CutScene::Update(float delta)
	{
		std::vector<Animation *> finishedAnimations;
		
		_animations->Enumerate<Animation>([&](Animation *animation, size_t size, bool *stop) {
			
			if(_time >= animation->GetBeginTime())
			{
				float atime = (_time - animation->GetBeginTime()) + animation->GetTimeOffset();
				animation->Apply(atime);
			}
			
			if(_time >= animation->GetBeginTime() + animation->GetDuration())
			{
				finishedAnimations.push_back(animation);
			}
			
		});
		
		if(!finishedAnimations.empty())
		{
			for(Animation *animation : finishedAnimations)
			{
				_animations->RemoveObject(animation);
			}
		}
		
		_time += delta;
	}
}
