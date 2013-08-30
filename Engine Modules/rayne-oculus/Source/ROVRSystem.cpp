//
//  ROVRSystem.cpp
//  rayne-oculus
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "ROVRSystem.h"

namespace RN
{
	namespace oculus
	{
		System::System()
		: _manager(0), _hmd(0), _sensor(0), _closedPrimary(false)
		{
			OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_None));
		}
		
		System::~System()
		{
			_sensorvalues.AttachToSensor(NULL);
			_sensor.Clear();
			_hmd.Clear();
			_manager.Clear();
			
			OVR::System::Destroy();
		}
		
		bool System::Initialize(bool vsync, bool sensorsOnly, bool closePrimary)
		{
			_manager = *OVR::DeviceManager::Create();
			_hmd = *_manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
			
			if(_manager && _hmd->GetDeviceInfo(&_hmdinfo))
			{
				if(closePrimary && !sensorsOnly)
				{
					_closedPrimary = true;
					CGDisplayCapture(CGMainDisplayID());
				}
				
				int resx = _hmdinfo.HResolution;
				int resy = _hmdinfo.VResolution;
				
				CGDirectDisplayID RiftDisplayId = (CGDirectDisplayID)_hmdinfo.DisplayId;
				
				RN::Screen *screen = RN::Window::GetSharedInstance()->GetScreenWithID(RiftDisplayId);
				if(screen && !sensorsOnly)
				{
					RN::WindowConfiguration *configuration = new RN::WindowConfiguration(resx, resy, screen);
					RN::Window::GetSharedInstance()->SetConfiguration(configuration, vsync?(RN::Window::WindowMaskFullscreen|RN::Window::WindowMaskVSync):RN::Window::WindowMaskFullscreen);
					
					configuration->Release();
				}
				
				RN::Window::GetSharedInstance()->HideCursor();
				
				_sensor.Clear();
				_sensor = *_hmd->GetSensor();
				
				if(_sensor)
				{
					_sensorvalues.AttachToSensor(_sensor);
					_sensorvalues.SetPrediction(0.03);
				}
				
				return true;
			}
			return false;
		}
		
		bool System::GetHMDConnected()
		{
			return _sensor;
		}
		
		OVR::HMDInfo &System::GetHMDInfo()
		{
			return _hmdinfo;
		}
		
		OVR::SensorFusion &System::GetHMDSensors()
		{
			return _sensorvalues;
		}
	}
}
