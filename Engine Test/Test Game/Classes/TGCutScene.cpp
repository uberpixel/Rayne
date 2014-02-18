//
//  TGCutScene.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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
	
	CutScene::CutScene(RN::Array *animations, RN::SceneNode *node) :
		_animations(new RN::Array()),
		_time(0.0f)
	{
		float time = 0.0f;
		
		RN::Vector3 cPosition = node->GetWorldPosition();
		RN::Vector3 cRotation = node->GetWorldEulerAngle();
		
		animations->Enumerate<RN::Dictionary>([&, node](RN::Dictionary *animation, size_t index, bool &object) {
			
			RN::Array *position = animation->GetObjectForKey<RN::Array>(RNSTR("position"));
			RN::Array *rotation = animation->GetObjectForKey<RN::Array>(RNSTR("rotation"));
			
			if(index == 0)
			{
				float x = position->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = position->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = position->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				float yaw   = rotation->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float pitch = rotation->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float roll  = rotation->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				cPosition = RN::Vector3(x, y, z);
				cRotation = RN::Vector3(yaw, pitch, roll);
				
				node->SetWorldPosition(cPosition);
				node->SetWorldRotation(cRotation);
				
				return;
			}
			
			
			float duration = animation->GetObjectForKey<RN::Number>(RNCSTR("time"))->GetFloatValue();
			
			if(position)
			{
				float x = position->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = position->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = position->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				RN::Vector3 toValue(x, y, z);
				
				duration = toValue.GetDistance(cPosition) * 0.089f;
				
				BasicAnimation<RN::Vector3> *temp = new BasicAnimation<RN::Vector3>();
				temp->SetDuration(duration);
				temp->SetBeginTime(time);
				
				temp->SetFromValue(cPosition);
				temp->SetToValue(toValue);
				temp->SetInterpolation(RN::Interpolator<RN::Vector3>::Type::Linear);
				
				temp->SetApplierFunction([=](const RN::Vector3 &value) {
					node->SetWorldPosition(value);
				});
				
				AddAnimation(temp);
				cPosition = toValue;
			}
			
			if(rotation)
			{
				float x = rotation->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = rotation->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = rotation->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				RN::Vector3 toValue(x, y, z);
				
				BasicAnimation<RN::Vector3> *temp = new BasicAnimation<RN::Vector3>();
				temp->SetDuration(duration);
				temp->SetBeginTime(time);
				
				temp->SetFromValue(cRotation);
				temp->SetToValue(toValue);
				temp->SetInterpolation(RN::Interpolator<RN::Vector3>::Type::SinusoidalEaseInOut);
				
				temp->SetApplierFunction([=](const RN::Vector3 &value) {
					node->SetWorldRotation(value);
				});
				
				AddAnimation(temp);
				cRotation = toValue;
			}
			
			time += duration;
			
		});
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
		
		_animations->Enumerate<Animation>([&](Animation *animation, size_t size, bool &stop) {
			
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
