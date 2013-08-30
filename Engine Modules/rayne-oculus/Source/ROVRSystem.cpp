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
#include "OVR.h"

namespace RN
{
	namespace oculus
	{
		System::System()
		: _manager(0), _hmd(0), _sensor(0), _closedPrimary(false), _sensorfusion(0)
		{
			OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_None));
		}
		
		System::~System()
		{
			if(_closedPrimary)
				CGDisplayRelease(CGMainDisplayID());
			
			if(_sensorfusion)
			{
				_sensorfusion->AttachToSensor(NULL);
				delete _sensorfusion;
				_sensorfusion = 0;
			}
			
			if(_sensor)
			{
				delete _sensor;
				_sensor = 0;
			}
			
			if(_hmd)
			{
				delete _hmd;
				_hmd = 0;
			}
			
			if(_manager)
			{
				delete _manager;
				_manager = 0;
			}
			
			OVR::System::Destroy();
		}
		
		bool System::Initialize(bool vsync, bool sensorsOnly, bool closePrimary)
		{
			if(_manager)
				return false;
			
			_manager = OVR::DeviceManager::Create();
			_hmd = _manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
			
			OVR::HMDInfo info;
			if(_manager && _hmd && _hmd->GetDeviceInfo(&info))
			{
				_hmdinfo.HResolution = info.HResolution;
				_hmdinfo.VResolution = info.VResolution;
				_hmdinfo.HScreenSize = info.HScreenSize;
				_hmdinfo.VScreenSize = info.VScreenSize;
				_hmdinfo.VScreenCenter = info.VScreenCenter;
				_hmdinfo.EyeToScreenDistance = info.EyeToScreenDistance;
				_hmdinfo.LensSeparationDistance = info.LensSeparationDistance;
				_hmdinfo.InterpupillaryDistance = info.InterpupillaryDistance;
				_hmdinfo.DistortionK[0] = info.DistortionK[0];
				_hmdinfo.DistortionK[1] = info.DistortionK[1];
				_hmdinfo.DistortionK[2] = info.DistortionK[2];
				_hmdinfo.DistortionK[3] = info.DistortionK[3];
				_hmdinfo.ChromaAbCorrection[0] = info.ChromaAbCorrection[0];
				_hmdinfo.ChromaAbCorrection[1] = info.ChromaAbCorrection[1];
				_hmdinfo.ChromaAbCorrection[2] = info.ChromaAbCorrection[2];
				_hmdinfo.ChromaAbCorrection[3] = info.ChromaAbCorrection[3];
				_hmdinfo.DisplayId = info.DisplayId;
				
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
				
				_sensor = _hmd->GetSensor();
				
				if(_sensor)
				{
					_sensorfusion = new OVR::SensorFusion();
					_sensorfusion->AttachToSensor(_sensor);
					_sensorfusion->SetPrediction(0.03);
				}
				
				return true;
			}
			return false;
		}
		
		void System::Update(float delta)
		{
			if(_sensorfusion)
			{
				OVR::Quatf orientation = _sensorfusion->GetPredictedOrientation();
				_sensordata.orientation.x = orientation.x;
				_sensordata.orientation.y = orientation.y;
				_sensordata.orientation.z = orientation.z;
				_sensordata.orientation.w = orientation.w;
				
				OVR::Vector3f acceleration = _sensorfusion->GetAcceleration();
				_sensordata.acceleration.x = acceleration.x;
				_sensordata.acceleration.y = acceleration.y;
				_sensordata.acceleration.z = acceleration.z;
				
				OVR::Vector3f angularVelocity = _sensorfusion->GetAngularVelocity();
				_sensordata.angularVelocity.x = angularVelocity.x;
				_sensordata.angularVelocity.y = angularVelocity.y;
				_sensordata.angularVelocity.z = angularVelocity.z;
				
				OVR::Vector3f magnetometer = _sensorfusion->GetMagnetometer();
				_sensordata.magnetometer.x = magnetometer.x;
				_sensordata.magnetometer.y = magnetometer.y;
				_sensordata.magnetometer.z = magnetometer.z;
			}
		}
		
		bool System::GetHMDConnected()
		{
			return _sensor;
		}
		
		HMDInfo &System::GetHMDInfo()
		{
			return _hmdinfo;
		}
		
		HMDSensors &System::GetHMDSensors()
		{
			return _sensordata;
		}
	}
}
