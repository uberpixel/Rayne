//
//  RNGamepad.h
//  Rayne
//
//  Created by Nils Daumann on 10.08.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Rayne__RNGamepad__
#define __Rayne__RNGamepad__

#include "Rayne.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDManager.h>

namespace RN
{
	class Gamepad
	{
	public:
		enum Mapping
		{
			Back,
			LStick,
			RStick,
			Start,
			Up,
			Right,
			Down,
			Left,
			L1,
			R1,
			B1,
			B2,
			B3,
			B4
		};
		
		Gamepad()
		{
			buttons = 0;
			trigger0 = 0.0f;
			trigger1 = 0.0f;
		}
		
		unsigned long buttons;
		Vector2 axis0;
		Vector2 axis1;
		float trigger0;
		float trigger1;
		
		void SetButton(unsigned char button, bool state)
		{
			if(state)
			{
				buttons |= (1 << button);
			}
			else
			{
				buttons &= ~(1 << button);
			}
		}
		
		bool GetButton(unsigned char button)
		{
			return (buttons & (1 << button));
		}
	};
	
	class GamepadManager : public Singleton<GamepadManager>
	{
	public:
		GamepadManager();
		~GamepadManager();
		
		int GetIntDeviceProperty(IOHIDDeviceRef device, CFStringRef key);
		float MapAnalogAxis(IOHIDValueRef value, IOHIDElementRef element);
		
		Gamepad *gamepad;
		
	private:
		static void OnDeviceConnected(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
		
		static void OnDeviceRemoved(void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
		
		static void OnDeviceValueChanged(void* context, IOReturn result, void* sender, IOHIDValueRef value);
		
		IOHIDManagerRef _hidManager;
	};
}

#endif /* defined(__Rayne__RNGamepad__) */
