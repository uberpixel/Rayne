//
//  TGSun.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGSun.h"

#define EARTH_AXIAL_TILT      23.5
#define EARTH_ORBITAL_PERIOD  365.25
#define EARTH_ROTATION_PERIOD 24.0

namespace TG
{
	double cycle(double value, double min, double max)
	{
		while(value < min)
		{
			value += (max - min);
		}
		
		while(value > max)
		{
			value -= (min + max);
		}
		
		return value;
	}
	
	Sun::Sun() :
		RN::Light(RN::Light::Type::DirectionalLight)
	{
		SetDate(181, 2013);
		SetTime(9, 32, 0);
		SetLatitude(48.0f, 81.0f);
		
		_sunGradient.AddColor(RN::Color(0.8f, 0.75f, 0.55f) * 1.5, 0.70f);
		_sunGradient.AddColor(RN::Color(0.8f, 0.75f, 0.55f) * 1.4, 0.45f);
		_sunGradient.AddColor(RN::Color(0.8f, 0.75f, 0.55f) * 1.3, 0.28f);
		_sunGradient.AddColor(RN::Color(0.9f, 0.5f, 0.2f) * 1.5, 0.12f);
		_sunGradient.AddColor(RN::Color(0.9f, 0.5f, 0.2f) * 0.25, 0.08f);
		_sunGradient.AddColor(RN::Color(0.5f, 0.5f, 0.5f) * 0.02, 0.0f);
		
		_ambientGradient.AddColor(RN::Color::White() * 0.8f, 0.79f);
		_ambientGradient.AddColor(RN::Color::White() * 0.8f, 0.28f);
		_ambientGradient.AddColor(RN::Color::White() * 0.6, 0.12f);
		_ambientGradient.AddColor(RN::Color::White() * 0.3, 0.08f);
		_ambientGradient.AddColor(RN::Color::White() * 0.1, 0.05f);
		_ambientGradient.AddColor(RN::Color::White() * 0.05, 0.0f);
		
		_fogGradient.AddColor(RN::Color::Color(0.127f, 0.252f, 0.393f) * 2.0f, 0.79f);
		_fogGradient.AddColor(RN::Color::Color(0.127f, 0.252f, 0.393f) * 2.0f, 0.28f);
		_fogGradient.AddColor(RN::Color::Color(0.127f, 0.252f, 0.393f) * 1.6, 0.12f);
		_fogGradient.AddColor(RN::Color::Color(0.127f, 0.252f, 0.393f) * 1.3, 0.08f);
		_fogGradient.AddColor(RN::Color::Color(0.127f, 0.252f, 0.393f) * 0.7, 0.05f);
		_fogGradient.AddColor(RN::Color::Black(), 0.0f);
		
		for(auto &color : _fogGradient._colors)
			color.first.a = 1.0f;
	}
	
	void Sun::SetTime(uint32 hour, uint32 minute, uint32 second)
	{
		_hour = hour;
		_minute = minute;
		_second = second;
	}
	
	void Sun::SetDate(uint32 day, uint32 year)
	{
		_day = day;
		_year = year;
	}
	
	void Sun::SetLatitude(float latitude, float equinox)
	{
		_latitude = RN::Math::DegreesToRadians(latitude);
		_equinox  = equinox;
	}
	
	
	void Sun::Update(float delta)
	{
		float blub = RN::Input::GetSharedInstance()->IsKeyPressed('q')-RN::Input::GetSharedInstance()->IsKeyPressed('e');
		UpdateTime(delta*blub*10.0);
		UpdateRotation();
		
		RN::Light::Update(delta);
	}
	
	void Sun::UpdateTime(float delta)
	{
		float _factor = 3600.0;
		//if(_hour > 22 || _hour < 6)
		//	_factor *= 15.0f;
		
		
		_second += (delta * _factor);
		
		while(_second >= 60)
		{
			_second -= 60.0;
			_minute ++;
		}
		
		while(_minute >= 60)
		{
			_minute -= 60.0;
			_hour ++;
		}
		
		while(_hour >= 24)
		{
			_hour -= 24;
			_day ++;
		}
		
		while(_day >= 365)
		{
			_day -= 365;
			_year ++;
		}
	}
	
	void Sun::UpdateRotation()
	{
		double B = RN::Math::DegreesToRadians((360.0 / EARTH_ORBITAL_PERIOD) * (_day - _equinox));
		
		double EoT = 9.87 * sin(2 * B) - 7.53 * cos(B) - 1.5 * sin(B);
		
		double LT  = _hour + (_minute / 60.0) + (_second / 3600.0);
		double LST = LT + EoT / 60.0;
		
		double HRA = RN::Math::DegreesToRadians(15.0 * (LST - 12.0));
		
		double d = _day + LT / 24.0;
		double D = RN::Math::DegreesToRadians(EARTH_AXIAL_TILT * sin(RN::Math::DegreesToRadians((360.0 / EARTH_ORBITAL_PERIOD) * (d - _equinox))));
		
		double pitch = cos(HRA) * cos(D) * cos(_latitude) + sin(D) * sin(_latitude);
		pitch = asin(std::min(1.0, std::max(-1.0, pitch)));

		double yaw = (sin(D) - sin(pitch) * sin(_latitude)) / (cos(pitch) * cos(_latitude));
		yaw = acos(std::min(1.0, std::max(-1.0, yaw)));
		
		
		yaw   = RN::Math::RadiansToDegrees(yaw);
		pitch = RN::Math::RadiansToDegrees(pitch);
		
		if(LST > 12.0 && RN::Math::RadiansToDegrees(HRA) > 0.0)
			yaw = 360.0 - yaw;
		
		yaw = cycle(yaw, 0.0, 360.0);
		
		_pitch   = pitch;

		_point = (RN::Math::DegreesToRadians(_pitch)) / 2.0f;
		_isNight = (pitch <= 0.014434f);
		
		//RNDebug("%f", _point);
		
		//RN::Color color = GetAmbientColor();
		//RNDebug("%f: {%f, %f, %f}", _point, color.r, color.g, color.b);
		
		SetColor(_sunGradient.GetColor(_point));
		SetRotation(RN::Quaternion(RN::Vector3(yaw, -pitch, 0.0)));
	}
}
