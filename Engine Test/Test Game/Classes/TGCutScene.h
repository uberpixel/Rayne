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
	class Animation : public RN::Object
	{
	public:
		Animation();
		virtual ~Animation();
		
		virtual void SetBeginTime(float time);
		virtual void SetDuration(float duration);
		virtual void SetTimeOffset(float offset);
		
		virtual void Apply(float time) = 0;
		
		float GetBeginTime() const { return _beginTime; }
		float GetDuration() const { return _duration; }
		float GetTimeOffset() const { return _offset; }
		
	private:
		float _offset;
		float _duration;
		float _beginTime;
	};
	
	template<class T>
	class BasicAnimation : public Animation
	{
	public:
		BasicAnimation()
		{}
		~BasicAnimation()
		{}
		
		
		void Apply(float time) override
		{
			if(_applier)
			{
				T value = _interpolator.GetValue(time);
				_applier(value);
			}
		}
		
		void SetApplierFunction(std::function<void (T&)> applier)
		{
			_applier = std::move(applier);
		}
		void SetDuration(float duration)
		{
			Animation::SetDuration(duration);
			_interpolator.SetDuration(duration);
		}
		void SetFromValue(const T& value)
		{
			_interpolator.SetStartValue(value);
		}
		void SetToValue(const T& value)
		{
			_interpolator.SetEndValue(value);
		}
		void SetInterpolation(typename RN::Interpolator<T>::Type type)
		{
			_interpolator.SetType(type);
		}
		
	private:
		RN::Interpolator<T> _interpolator;
		std::function<void (T&)> _applier;
	};
	
	class CutScene
	{
	public:
		CutScene();
		CutScene(RN::Array *animations, RN::SceneNode *node);
		~CutScene();
		
		void AddAnimation(Animation *animation);
		void Update(float delta);
		void Reset();
		
		bool HasAnimations() const { return _animations->GetCount() > 0; }
		
	private:
		float _time;
		RN::Array *_animations;
	};
}

#endif /* defined(__Test_Game__TGCutScene__) */
