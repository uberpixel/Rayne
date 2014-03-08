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
	CutScene::CutScene(RN::Array *keys, RN::SceneNode *node) :
		_time(0.0f),
		_node(node),
		_currentPositionKeyIndex(1),
		_currentRotationKeyIndex(1),
		_oldLerpFactor(0.0f)
	{
		std::vector<PositionKey *> positions;
		std::vector<RotationKey *> rotations;
		
		PositionKey *previousPositionKey = nullptr;
		RotationKey *previousRotationKey = nullptr;
		
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
				
				if(previousPositionKey)
				{
					PositionKey *temp = new PositionKey();
					temp->SetTime((previousPositionKey->GetTime() + time)*0.5f);
					temp->SetPosition(previousPositionKey->GetPosition().GetLerp(toValue, 0.5f));
					AddKey(temp);
				}
				
				PositionKey *temp = new PositionKey();
				temp->SetTime(time);
				temp->SetPosition(toValue);
				
				previousPositionKey = temp;
				
				AddKey(temp);
			}
			
			if(rotation)
			{
				float x = rotation->GetObjectAtIndex<RN::Number>(0)->GetFloatValue();
				float y = rotation->GetObjectAtIndex<RN::Number>(1)->GetFloatValue();
				float z = rotation->GetObjectAtIndex<RN::Number>(2)->GetFloatValue();
				
				RN::Vector3 toValue(x, y, z);
				
				if(previousRotationKey)
				{
					RotationKey *temp = new RotationKey();
					temp->SetTime((previousRotationKey->GetTime() + time)*0.5f);
					temp->SetRotation(previousRotationKey->GetRotation().GetLerpSpherical(toValue, 0.5f));
					AddKey(temp);
				}
				
				RotationKey *temp = new RotationKey();
				temp->SetTime(time);
				temp->SetRotation(toValue);
				
				previousRotationKey = temp;
				
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
		float lerpfactor = fmod(_time/5.0f, 1.0f);
		if(lerpfactor < _oldLerpFactor)
		{
			_currentPositionKeyIndex += 2;
			_currentRotationKeyIndex += 2;
		}
		_oldLerpFactor = lerpfactor;
		
		if(_currentPositionKeyIndex >= _positions.size()-2)
			return;
			
		PositionKey *fromKey = _positions[_currentPositionKeyIndex];
		PositionKey *middleKey = _positions[_currentPositionKeyIndex+1];
		PositionKey *toKey = _positions[_currentPositionKeyIndex+2];
		
		RN::Vector3 fromPosition = fromKey->GetPosition().GetLerp(middleKey->GetPosition(), lerpfactor);
		RN::Vector3 toPosition = middleKey->GetPosition().GetLerp(toKey->GetPosition(), lerpfactor);
		
		_node->SetPosition(fromPosition.GetLerp(toPosition, lerpfactor));
		
		if(_currentRotationKeyIndex >= _positions.size()-2)
			return;
		
		RotationKey *fromRotationKey = _rotations[_currentRotationKeyIndex];
		RotationKey *middleRotationKey = _rotations[_currentRotationKeyIndex+1];
		RotationKey *toRotationKey = _rotations[_currentRotationKeyIndex+2];
		
		RN::Quaternion fromRotation = fromRotationKey->GetRotation().GetLerpSpherical(middleRotationKey->GetRotation(), lerpfactor);
		RN::Quaternion toRotation = middleRotationKey->GetRotation().GetLerpSpherical(toRotationKey->GetRotation(), lerpfactor);
		
		_node->SetRotation(fromRotation.GetLerpSpherical(toRotation, lerpfactor));
		
		_time += delta;
	}
}
