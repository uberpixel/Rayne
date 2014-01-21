//
//  TGSun.cpp
//  Test Game
//
//  Created by Sidney Just on 16/01/14.
//  Copyright (c) 2014 Überpixel. All rights reserved.
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
		SetTime(12, 32, 0);
		SetLatitude(48.0f, 81.0f);
		
		RN::Texture2D *texture = static_cast<RN::Texture2D *>(RN::Texture::WithFile("textures/sun_gradient.png"));
		float *color = new float[texture->GetWidth() * texture->GetHeight() * 4];
		
		RN::Texture::PixelData data;
		data.alignment = 1;
		data.format = RN::Texture::Format::RGBA32F;
		data.data = color;
		
		texture->GetData(data);
		
		float *temp = color;
		
		for(size_t i = 0; i < texture->GetWidth(); i ++)
		{
			RN::Color tcolor = RN::Color(color[0], color[1], color[2], 1.0f);
			tcolor *= 0.48f;
			tcolor.a = 1.0;
			
			_temperatures.push_back(std::move(tcolor));
			color += 4;
		}
		
		delete [] temp;
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
		UpdateTime(delta);
		UpdateRotation();
		
		RN::Light::Update(delta);
	}
	
	void Sun::UpdateTime(float delta)
	{
		float _factor = 3600.0;
		
		
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
		
		_isNight = (pitch <= 0.0);
		_pitch   = pitch;
		
		
		double factor = (_second + (60.0 * _minute) + (3600.0 * _hour)) / 86400.0;
		size_t index  = static_cast<size_t>(roundf(_temperatures.size() * factor));
		
		SetColor(_temperatures[index]);
		SetRotation(RN::Quaternion(RN::Vector3(yaw, -pitch, 0.0)));
	}
}
