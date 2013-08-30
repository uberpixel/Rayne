//
//  ROVRSystem.h
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

#ifndef __rayne_oculus__ROVRSystem__
#define __rayne_oculus__ROVRSystem__


#include <Rayne.h>

namespace OVR
{
	class DeviceManager;
	class HMDDevice;
	class SensorDevice;
	class SensorFusion;
}

namespace RN
{
	namespace oculus
	{
		class HMDInfo
		{
		public:
			unsigned HResolution, VResolution;
			float HScreenSize, VScreenSize;
			float VScreenCenter;
			float EyeToScreenDistance;
			float LensSeparationDistance;
			float InterpupillaryDistance;
			float DistortionK[4];
			float ChromaAbCorrection[4];
			long DisplayId;
		};
		
		class HMDSensors
		{
		public:
			Quaternion orientation;
			Vector3 acceleration;
			Vector3 angularVelocity;
			Vector3 magnetometer;
		};
		
		class System : public Singleton<System>
		{
		public:
			System();
			~System();
			
			bool Initialize(bool vsync=true, bool sensorsOnly=false, bool closePrimary=false);
			
			bool GetHMDConnected();
			HMDInfo &GetHMDInfo();
			HMDSensors &GetHMDSensors();
			
			void Update(float delta);
			
		private:
			bool _closedPrimary;
			
			OVR::DeviceManager *_manager;
			OVR::HMDDevice *_hmd;
			OVR::SensorDevice *_sensor;
			
			OVR::SensorFusion *_sensorfusion;
			
			HMDInfo _hmdinfo;
			HMDSensors _sensordata;
		};
	}
}

#endif /* defined(__rayne_oculus__ROVRSystem__) */
