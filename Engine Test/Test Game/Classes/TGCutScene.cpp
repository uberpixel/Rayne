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
	CutScene::CutScene() :
		_time(0.0f)
	{
	}
	
	CutScene::CutScene(RN::Array *keys, RN::SceneNode *node) :
		_time(0.0f),
		_smoothing(2.0f),
		_node(node)
	{
		keys->Enumerate<RN::Dictionary>([&, node](RN::Dictionary *animation, size_t index, bool &object) {
			
			RN::Array *position = animation->GetObjectForKey<RN::Array>(RNSTR("position"));
			RN::Array *rotation = animation->GetObjectForKey<RN::Array>(RNSTR("rotation"));
			
			float time = animation->GetObjectForKey<RN::Number>(RNCSTR("time"))->GetFloatValue();
			
			if(index == 0)
			{
				float x = position->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = position->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = position->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				float yaw   = rotation->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float pitch = rotation->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float roll  = rotation->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				node->SetWorldPosition(RN::Vector3(x, y, z));
				node->SetWorldRotation(RN::Vector3(yaw, pitch, roll));
			}
			
			if(position)
			{
				float x = position->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = position->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = position->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				RN::Vector3 toValue(x, y, z);
				
				PositionKey *temp = new PositionKey();
				temp->SetTime(time);
				temp->SetPosition(toValue);
				
				AddKey(temp);
			}
			
			if(rotation)
			{
				float x = rotation->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = rotation->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = rotation->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				RN::Vector3 toValue(x, y, z);
				
				RotationKey *temp = new RotationKey();
				temp->SetTime(time);
				temp->SetRotation(toValue);
				
				AddKey(temp);
			}
		});
	}
	
	CutScene::~CutScene()
	{
		for(auto key : _positions)
			delete key;
		
		for(auto key : _rotations)
			delete key;
	}
	
	void CutScene::AddKey(PositionKey *key)
	{
		_positions.push_back(key);
	}
	
	void CutScene::AddKey(RotationKey *key)
	{
		_rotations.push_back(key);
	}
	
	void CutScene::Reset()
	{
		_time = 0.0f;
	}
	
	void CutScene::Update(float delta)
	{
		PositionKey *fromKey = _positions[0];
		PositionKey *toKey = _positions[0];
		int i;
		for(i = 1; i < _positions.size(); i++)
		{
			toKey = _positions[i];
			if(toKey->GetTime() > (_time - _smoothing))
				break;
			fromKey = toKey;
		}
		
		float fromFactor = (_time - _smoothing - fromKey->GetTime()) / (toKey->GetTime() - fromKey->GetTime());
		fromFactor = fminf(fmaxf(fromFactor, 0.0f), 1.0f);
		RN::Vector3 fromPosition = fromKey->GetPosition().GetLerp(toKey->GetPosition(), fromFactor);
		
		for(; i < _positions.size(); i++)
		{
			toKey = _positions[i];
			if(toKey->GetTime() > (_time + _smoothing))
				break;
			fromKey = toKey;
		}
		
		float toFactor = (_time + _smoothing - fromKey->GetTime()) / (toKey->GetTime() - fromKey->GetTime());
		toFactor = fminf(fmaxf(toFactor, 0.0f), 1.0f);
		RN::Vector3 toPosition = fromKey->GetPosition().GetLerp(toKey->GetPosition(), toFactor);
		
		_node->SetWorldPosition(fromPosition.GetLerp(toPosition, 0.5f));
		
		
		
		
		
		RotationKey *fromKeyRotation = _rotations[0];
		RotationKey *toKeyRotation = _rotations[0];
		for(i = 1; i < _rotations.size(); i++)
		{
			toKeyRotation = _rotations[i];
			if(toKeyRotation->GetTime() > (_time - _smoothing))
				break;
			fromKeyRotation = toKeyRotation;
		}
		
		fromFactor = (_time - _smoothing - fromKeyRotation->GetTime()) / (toKey->GetTime() - fromKeyRotation->GetTime());
		fromFactor = fminf(fmaxf(fromFactor, 0.0f), 1.0f);
		RN::Quaternion fromRotation = fromKeyRotation->GetRotation().GetLerpSpherical(toKeyRotation->GetRotation(), fromFactor);
		
		for(; i < _rotations.size(); i++)
		{
			toKeyRotation = _rotations[i];
			if(toKeyRotation->GetTime() > (_time + _smoothing))
				break;
			fromKeyRotation = toKeyRotation;
		}
		
		toFactor = (_time + _smoothing - fromKeyRotation->GetTime()) / (toKeyRotation->GetTime() - fromKeyRotation->GetTime());
		toFactor = fminf(fmaxf(toFactor, 0.0f), 1.0f);
		RN::Quaternion toRotation = fromKeyRotation->GetRotation().GetLerpSpherical(toKeyRotation->GetRotation(), toFactor);
		
		_node->SetWorldRotation(fromRotation.GetLerpSpherical(toRotation, 0.5f));
		
		
		
		
		_time += delta;
	}
}
