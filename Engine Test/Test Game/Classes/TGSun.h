//
//  TGSun.h
//  Test Game
//
//  Created by Sidney Just on 16/01/14.
//  Copyright (c) 2014 Überpixel. All rights reserved.
//

#ifndef __Test_Game__TGSun__
#define __Test_Game__TGSun__

#include <Rayne.h>

namespace TG
{
	class Sun : public RN::Light
	{
	public:
		Sun();
		
		void SetTime(uint32 hour, uint32 minute, uint32 second);
		void SetDate(uint32 day, uint32 year);
		void SetLatitude(float latitude, float equinox);
		
		bool IsNight() const { return _isNight; }
		float GetPitch() const { return _pitch; }
		
		void Update(float delta) override;
		
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
		
		std::vector<RN::Color> _temperatures;
	};
}

#endif /* defined(__Test_Game__TGSun__) */
