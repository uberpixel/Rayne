//
//  TGCutScene.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Test_Game__TGCutScene__
#define __Test_Game__TGCutScene__

#include <Rayne.h>

namespace TG
{
	class KeyFrame
	{
	public:
		KeyFrame() : _time(0.0f) {}
		void SetTime(float t) { _time = t; }
		float GetTime() const { return _time; }
		
	private:
		float _time;
	};
	
	class PositionKey : public KeyFrame
	{
	public:
		void SetPosition(RN::Vector3 position) { _position = position; }
		RN::Vector3 GetPosition() const { return _position; }
		
	private:
		RN::Vector3 _position;
	};
	
	class RotationKey : public KeyFrame
	{
	public:
		void SetRotation(RN::Quaternion rotation) { _rotation = rotation; }
		RN::Quaternion GetRotation() const { return _rotation; }
		
	private:
		RN::Quaternion _rotation;
	};
	
	class CutScene
	{
	public:
		CutScene();
		CutScene(RN::Array *keys, RN::SceneNode *node);
		~CutScene();
		
		void AddKey(PositionKey *key);
		void AddKey(RotationKey *key);
		void Update(float delta);
		void Reset();
		void SetSmoothing();
		void SetOffset();
		bool HasKeys() const {return (_positions.size()+_rotations.size()) > 0;}
		
	private:
		RN::SceneNode *_node;
		float _time;
		float _smoothing;
		std::vector<PositionKey *> _positions;
		std::vector<RotationKey *> _rotations;
	};
}

#endif /* defined(__Test_Game__TGCutScene__) */
