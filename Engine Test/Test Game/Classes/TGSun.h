//
//  TGSun.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Test_Game__TGSun__
#define __Test_Game__TGSun__

#include <Rayne.h>

namespace TG
{
	struct Gradient
	{
		void AddColor(const RN::Color &color, float point)
		{
			_colors.emplace_back(std::make_pair(color, point));
			
			std::sort(_colors.begin(), _colors.end(), [](const std::pair<RN::Color, float> &color1, const std::pair<RN::Color, float> &color2) {
				return color1.second < color2.second;
			});
		}
		
		RN::Color GetColor(float point) const
		{
			if(_colors.empty())
			{
				return RN::Color::Black();
			}
			else if(_colors.size() == 1)
			{
				return _colors[0].first;
			}
			
			std::pair<size_t, float> minBound, maxBound;
			
			// Min value
			minBound.first  = 0;
			minBound.second = -1;
			
			for(size_t i = 0; i < _colors.size(); i ++)
			{
				if(_colors[i].second < point && _colors[i].second > minBound.second)
				{
					minBound.first  = i;
					minBound.second = _colors[i].second;
				}
			}
			
			// Max value
			maxBound.first  = 0;
			maxBound.second = 2;
			
			for(size_t i = 0; i < _colors.size(); i ++)
			{
				if(_colors[i].second > point && _colors[i].second < maxBound.second)
				{
					maxBound.first  = i;
					maxBound.second = _colors[i].second;
				}
			}
			
			float range = maxBound.second - minBound.second,
			rangepoint = (point - minBound.second) / range;
			
			return _colors.at(minBound.first).first * (1 - rangepoint) + _colors.at(maxBound.first).first * rangepoint;
		}
		
		std::vector<std::pair<RN::Color, float>> _colors;
	};
	
	class Sun : public RN::Light
	{
	public:
		Sun();
		Sun(RN::Deserializer *deserializer);
		
		void Serialize(RN::Serializer *serializer) override;
		
		void SetTime(uint32 hour, uint32 minute, uint32 second);
		void SetDate(uint32 day, uint32 year);
		void SetLatitude(float latitude, float equinox);
		
		bool IsNight() const { return _isNight; }
		float GetPitch() const { return _pitch; }
		
		RN::Color GetAmbientColor() const { return _ambientGradient.GetColor(_point); }
		RN::Color GetFogColor() const { return _fogGradient.GetColor(_point); }
		
		float GetTime() const;
		void SetTime(float time);
		
		void Update(float delta) override;
		void UpdateEditMode(float delta) override;
		
	private:
		void UpdateRotation();
		void UpdateTime(float delta);
		
		uint32 _hour;
		uint32 _minute;
		float _second;
		
		uint32 _day;
		uint32 _year;
		
		float _latitude;
		float _equinox;
		
		bool _isNight;
		float _pitch;
		float _point;
		
		RN::Observable<float, Sun> _time;
		
		Gradient _sunGradient;
		Gradient _ambientGradient;
		Gradient _fogGradient;
		
		RNDeclareMeta(Sun)
	};
}

#endif /* defined(__Test_Game__TGSun__) */
