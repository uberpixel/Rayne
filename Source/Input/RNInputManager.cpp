//
//  RNInputManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNNotificationManager.h"
#include "../Base/RNScopeAllocator.h"
#include "../Debug/RNLogger.h"
#include "Devices/RNPS4Controller.h"
#include "RNInputManager.h"

#if RN_PLATFORM_LINUX
#include <X11/X.h>
#include <X11/Xlib.h>
//#include <X11/Intrinsic.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#endif

#if RN_PLATFORM_IOS
#include "OISInputManager.h"
#include "OISMultiTouch.h"
#endif

namespace RN
{
	class InputBindPoint : public Object
	{
	public:
		InputBindPoint()
		{}

		void AddCallback(InputManager::Callback &&callback, void *token)
		{
			_callbacks.emplace_back(std::make_pair(std::move(callback), token));
		}
		bool RemoveToken(void *token)
		{
			for(auto iterator = _callbacks.begin(); iterator != _callbacks.end();)
			{
				if(iterator->second == token)
				{
					iterator = _callbacks.erase(iterator);
					break;
				}

				iterator ++;
			}

			return (_callbacks.size() == 0);
		}

		void Perform(const InputManager::Action &action)
		{
			for(auto &callback : _callbacks)
				callback.first(action);
		}

	private:
		std::vector<std::pair<InputManager::Callback, void *>> _callbacks;
	};

#if !RN_PLATFORM_IOS && !RN_PLATFORM_VISIONOS
	extern void BuildPlatformDeviceTree();
	extern void TearDownPlatformDeviceTree();
#endif

#if RN_PLATFORM_IOS
	class OISInputHandler : public OIS::MultiTouchListener
	{
	public:
		OISInputHandler(InputManager *inputManager) { _inputManager = inputManager; }
		
		bool touchMoved(const OIS::MultiTouchEvent& arg) override
		{
			_inputManager->_lock.Lock();
			_inputManager->_mouseMovement = Vector3(arg.state.X.rel, arg.state.Y.rel, arg.state.Z.rel);
			_inputManager->_lock.Unlock();
			return true;
		}

		bool touchPressed(const OIS::MultiTouchEvent& arg) override
		{
			_inputManager->_lock.Lock();
			_inputManager->_currentTouchCount += 1;
			_inputManager->_lock.Unlock();
			return true;
		}

		bool touchReleased(const OIS::MultiTouchEvent& arg) override
		{
			_inputManager->_lock.Lock();
			_inputManager->_currentTouchCount -= 1;
			_inputManager->_lock.Unlock();
			return true;
		}

		bool touchCancelled(const OIS::MultiTouchEvent& arg) override
		{
			_inputManager->_lock.Lock();
			_inputManager->_currentTouchCount -= 1;
			_inputManager->_lock.Unlock();
			return true;
		}
		
	private:
		InputManager *_inputManager;
	};
#endif

	static InputManager *__sharedInstance = nullptr;

	InputManager::InputManager() :
		_devices(new Array()),
		_bindings(new Dictionary()),
		_mode(0),
		_mouseDevices(new Array()),
		_hidDevices(new Array())
	{
		__sharedInstance = this;

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
		memset(_keyPressed, 0, 256*sizeof(bool));
		memset(_mouseButton, 0, 2 * sizeof(bool));
#endif

#if RN_PLATFORM_WINDOWS
		RAWINPUTDEVICE Rid[2];

		Rid[0].usUsagePage = 0x01;
		Rid[0].usUsage = 0x02;
		Rid[0].dwFlags = 0;   // adds HID mouse and also ignores legacy mouse messages
		Rid[0].hwndTarget = 0;

		Rid[1].usUsagePage = 0x01;
		Rid[1].usUsage = 0x06;
		Rid[1].dwFlags = 0;   // adds HID keyboard and also ignores legacy keyboard messages
		Rid[1].hwndTarget = 0;

		RN_ASSERT(RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])), "Raw input shit is broken, yo!");
#endif

#if RN_PLATFORM_LINUX
		_xDisplay = XOpenDisplay(NULL);
		if(!_xDisplay)
		{
			RNDebug("Failed to open X display for input.");
			return;
		}

		int event, error;
		if(!XQueryExtension(_xDisplay, "XInputExtension", &_xiOpcode, &event, &error))
		{
			RNDebug("X Input extension not available.");
			return;
		}

		int major = 2;
		int minor = 2;
		int rc = XIQueryVersion(_xDisplay, &major, &minor);
		if(rc == BadRequest)
		{
			RNDebug("No XI2 support.");
			return;
		}
		else if(rc != Success)
		{
			RNDebug("Internal Error! This is a bug in Xlib.");
		}

		RNDebug("XI2 supported.");


		XIEventMask evmask;

		uint8 maskLen = (XI_LASTEVENT + 7)/8;
		unsigned char mask1[maskLen];
		memset(mask1, 0, sizeof(mask1));

		/* select for button and key events from all master devices */
		XISetMask(mask1, XI_RawMotion);
		XISetMask(mask1, XI_RawKeyPress);
		XISetMask(mask1, XI_RawKeyRelease);

		evmask.deviceid = XIAllMasterDevices;
		evmask.mask_len = sizeof(mask1);
		evmask.mask = mask1;

		XISelectEvents(_xDisplay, DefaultRootWindow(_xDisplay), &evmask, 1);
		XFlush(_xDisplay);
#endif
		
#if RN_PLATFORM_IOS
		_currentTouchCount = 0;
		
		OIS::ParamList pl;
		_inputManager = OIS::InputManager::createInputSystem(pl);
		
		//Lets enable all addons that were compiled in:
		_inputManager->enableAddOnFactory(OIS::InputManager::AddOn_All);
		
		OIS::MultiTouch *multitouch = (OIS::MultiTouch*)_inputManager->createInputObject(OIS::OISMultiTouch, true);
		
		_oisInputHandler = new OISInputHandler(this);
		multitouch->setEventCallback(_oisInputHandler);
#endif

		PS4Controller::RegisterDriver();

		// Avoid any and all re-ordering
		std::atomic_thread_fence(std::memory_order_seq_cst);
		
#if !RN_PLATFORM_IOS && !RN_PLATFORM_VISIONOS
		BuildPlatformDeviceTree();
#endif
	}

	InputManager::~InputManager()
	{
#if !RN_PLATFORM_IOS && !RN_PLATFORM_VISIONOS
		TearDownPlatformDeviceTree();
#endif
		
#if RN_PLATFORM_IOS
		delete _oisInputHandler;
#endif
		
		_devices->Release();
		_bindings->Release();
		_mouseDevices->Release();
		_hidDevices->Release();
	}

	InputManager *InputManager::GetSharedInstance()
	{
		return __sharedInstance;
	}


#if RN_PLATFORM_WINDOWS
	void InputManager::__HandleRawInput(HRAWINPUT lParam)
	{
		ScopeAllocator allocator;

		UINT dwSize = 0;
		GetRawInputData(lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
		auto lpb = allocator.AllocBytes<BYTE>(dwSize);

		RN_ASSERT(GetRawInputData(lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize , "GetRawInputData does not return correct size!");

		RAWINPUT* rawInput = (RAWINPUT*)lpb;

		if(rawInput->header.dwType == RIM_TYPEKEYBOARD)
		{
			if(rawInput->data.keyboard.VKey < 256)
				_keyPressed[rawInput->data.keyboard.VKey] = (rawInput->data.keyboard.Message == WM_KEYDOWN);
		}
		else if(rawInput->header.dwType == RIM_TYPEMOUSE)
		{
			_mouseMovement.x += rawInput->data.mouse.lLastX;
			_mouseMovement.y += rawInput->data.mouse.lLastY;

			if((rawInput->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL)
			{
				 float wheelDelta = (float)(short)rawInput->data.mouse.usButtonData;
				 _mouseMovement.z = wheelDelta / WHEEL_DELTA;;
			}

			if((rawInput->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) == RI_MOUSE_LEFT_BUTTON_DOWN)
			{
				_mouseButton[0] = true;
			}

			if((rawInput->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) == RI_MOUSE_LEFT_BUTTON_UP)
			{
				_mouseButton[0] = false;
			}

			if((rawInput->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) == RI_MOUSE_RIGHT_BUTTON_DOWN)
			{
				_mouseButton[1] = true;
			}

			if((rawInput->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) == RI_MOUSE_RIGHT_BUTTON_UP)
			{
				_mouseButton[1] = false;
			}
		}
	}
#endif
	
#if RN_PLATFORM_MAC_OS
	void InputManager::ProcessKeyEvent(uint16 keyCode, bool state)
	{
		if(keyCode < 256) _keyPressed[keyCode] = state;
	}
#endif

	void InputManager::__AddDevice(InputDevice *device)
	{
		LockGuard<Lockable> lock(_lock);

		RN_ASSERT(device->IsRegistered() == false, "Device is already registered");
		device->_manager = this;

		_devices->AddObject(device);

		if(device->GetCategory() == InputDevice::Category::Mouse)
			_mouseDevices->AddObject(device);

		RNDebug("(Input) Added device: " << device);
	}

	void InputManager::__RemoveDevice(InputDevice *device)
	{
		LockGuard<Lockable> lock(_lock);

		RN_ASSERT(device->IsRegistered(), "Device must be registered");
		device->_manager = nullptr;

		RNDebug("(Input) Removed device: " << device);

		if(device->GetCategory() == InputDevice::Category::Mouse)
			_mouseDevices->RemoveObject(device);

		_devices->RemoveObject(device);
	}

	void InputManager::SetMouseMode(MouseMode mode)
	{
		_mode = mode;

#if RN_PLATFORM_MAC_OS
		if(_mode & MouseMode::Hidden)
			CGDisplayHideCursor(kCGDirectMainDisplay);
		else
			CGDisplayShowCursor(kCGDirectMainDisplay);

		if(_mode & MouseMode::Captured)
			CGAssociateMouseAndMouseCursorPosition(false);
		else
			CGAssociateMouseAndMouseCursorPosition(true);
#endif
	}

	void InputManager::Update(float delta)
	{
		RN::Array *devices;

		{
			LockGuard<Lockable> lock(_lock);
			devices = _devices->Copy()->Autorelease();
		}

		_previousMouseDelta = _mouseDelta;
		_mouseDelta = Vector3();

		devices->Enumerate<InputDevice>([&](InputDevice *device, size_t index, bool &stop) {
			device->Update();
		});

		_mouseDevices->Enumerate<InputDevice>([&](InputDevice *device, size_t index, bool &stop) {

			Array *controls = device->GetControlsWithType(InputDevice::Type::DeltaAxis);

			controls->Enumerate<DeltaAxisControl>([&](DeltaAxisControl *control, size_t index, bool &stop) {

				Number *value = control->GetValue<Number>();
				if(!value)
					return;

				switch(control->GetAxis())
				{
					case AxisControl::Axis::X:
						_mouseDelta.x += value->GetFloatValue();
						break;
					case AxisControl::Axis::Y:
						_mouseDelta.y += value->GetFloatValue();
						break;
					case AxisControl::Axis::Z:
						_mouseDelta.z += value->GetFloatValue();
						break;

					default:
						break;
				}

			});

		});

#if RN_PLATFORM_LINUX
		if(!_xDisplay) return;

		XEvent ev;
		while(XPending(_xDisplay) > 0)
		{
			XNextEvent(_xDisplay, &ev);
			XGenericEventCookie *cookie = &ev.xcookie;
			if(cookie->type != GenericEvent || cookie->extension != _xiOpcode || !XGetEventData(_xDisplay, cookie)) continue;

			switch(cookie->evtype)
			{
				case XI_RawMotion:
				{
					XIRawEvent *re = (XIRawEvent *) cookie->data;

					double *raw_valuator = re->raw_values;
					double *valuator = re->valuators.values;
					for(int i = 0; i < re->valuators.mask_len * 8; i++)
					{
						if(XIMaskIsSet(re->valuators.mask, i))
						{
							if(i == 0)
								_mouseMovement.x += static_cast<float>(*raw_valuator);
							if(i == 1)
								_mouseMovement.y += static_cast<float>(*raw_valuator);

							valuator++;
							raw_valuator++;
						}
					}
					break;
				}

				case XI_RawKeyPress:
				{
					XIRawEvent *re = (XIRawEvent *) cookie->data;
					_keyPressed[re->detail] = true;
					break;
				}

				case XI_RawKeyRelease:
				{
					XIRawEvent *re = (XIRawEvent *) cookie->data;
					_keyPressed[re->detail] = false;
					break;
				}
			}
			XFreeEventData(_xDisplay, cookie);
		}
#endif

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX
		_mouseDelta += _mouseMovement;
		_mouseMovement = Vector3();
#endif
		
#if RN_PLATFORM_IOS
		_lock.Lock();
		_mouseDelta += _mouseMovement;
		_mouseMovement = Vector3();
		_lock.Unlock();
#endif

		if(_mode & MouseMode::Smoothed)
		{
			float x = _mouseDelta.x * 0.5f;
			float y = _mouseDelta.y * 0.5f;

			_mouseDelta.x = _previousMouseDelta.x * 0.5f + x;
			_mouseDelta.y = _previousMouseDelta.y * 0.5f + y;
		}
	}


	Array *InputManager::GetDevicesWithCategories(InputDevice::Category categories)
	{
		Array *result = new Array();

		{
			LockGuard<Lockable> lock(_lock);
			_devices->Enumerate<InputDevice>([&](InputDevice *device, size_t index, bool &stop) {

				if(device->GetCategory() & categories)
					result->AddObject(device);

			});
		}

		return result->Autorelease();
	}


	void InputManager::AddTarget(void *target, Event events, InputDevice::Category category, Callback &&callback)
	{
		LockGuard<Lockable> lock(_lock);
		_targets.emplace_back(target, events, nullptr, category, std::move(callback));
	}
	void InputManager::AddTarget(void *target, Event events, InputDevice *device, Callback &&callback)
	{
		LockGuard<Lockable> lock(_lock);
		_targets.emplace_back(target, events, device, 0, std::move(callback));
	}

	void InputManager::RemoveTarget(void *target, Event events, InputDevice *device)
	{
		LockGuard<Lockable> lock(_lock);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if(iterator->target == target && (iterator->events & events) && iterator->device == device)
			{
				iterator->events &= ~events;

				if(iterator->events == 0)
				{
					iterator = _targets.erase(iterator);
					continue;
				}
			}

			iterator ++;
		}
	}

	void InputManager::RemoveTarget(void *target, Event events)
	{
		LockGuard<Lockable> lock(_lock);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if(iterator->target == target && iterator->events & events)
			{
				iterator->events &= ~events;

				if(iterator->events == 0)
				{
					iterator = _targets.erase(iterator);
					continue;
				}
			}

			iterator ++;
		}
	}
	void InputManager::RemoveTarget(void *target)
	{
		LockGuard<Lockable> lock(_lock);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if(iterator->target == target)
			{
				iterator = _targets.erase(iterator);
				continue;
			}

			iterator ++;
		}
	}

	void InputManager::Bind(void *target, const String *name, Callback &&callback)
	{
		LockGuard<Lockable> lock(_lock);

		InputBindPoint *bindPoint = _bindings->GetObjectForKey<InputBindPoint>(name);
		if(!bindPoint)
		{
			bindPoint = new InputBindPoint();
			_bindings->SetObjectForKey(bindPoint, name);
			bindPoint->Release();
		}

		bindPoint->AddCallback(std::move(callback), target);
	}

	void InputManager::Unbind(void *target, const String *name)
	{
		LockGuard<Lockable> lock(_lock);

		InputBindPoint *bindPoint = _bindings->GetObjectForKey<InputBindPoint>(name);
		if(bindPoint)
		{
			if(bindPoint->RemoveToken(target))
				_bindings->RemoveObjectForKey(name);
		}
	}
	
	void InputManager::PerformEvent(Event event, InputDevice *device, InputControl *control, Object *value)
	{
		LockGuard<Lockable> lock(_lock);

		Action action;
		action.event = event;
		action.device = device;
		action.control = control;
		action.value = value;

		InputBindPoint *bindPoint = _bindings->GetObjectForKey<InputBindPoint>(control->GetName());
		if(bindPoint)
			bindPoint->Perform(action);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if((iterator->events & event) && (iterator->device == device || (iterator->device == nullptr && iterator->categories & device->GetCategory())))
				iterator->callback(action);

			iterator ++;
		}
	}

	bool InputManager:: IsControlToggling(const String *name) const
	{
		size_t count = _devices->GetCount();
		for(size_t i = 0; i < count; i ++)
		{
			InputDevice *device = static_cast<InputDevice *>(_devices->GetObjectAtIndex(i));
			if(device->IsControlToggling(name))
				return true;
		}

#if RN_PLATFORM_WINDOWS
		//TODO: Support all keys
		if(name->IsEqual(RNCSTR("0")))
		{
			return _keyPressed[0x30];
		}
		if(name->IsEqual(RNCSTR("1")))
		{
			return _keyPressed[0x31];
		}
		if(name->IsEqual(RNCSTR("2")))
		{
			return _keyPressed[0x32];
		}
		if(name->IsEqual(RNCSTR("3")))
		{
			return _keyPressed[0x33];
		}
		if(name->IsEqual(RNCSTR("4")))
		{
			return _keyPressed[0x34];
		}
		if(name->IsEqual(RNCSTR("5")))
		{
			return _keyPressed[0x35];
		}
		if(name->IsEqual(RNCSTR("6")))
		{
			return _keyPressed[0x36];
		}
		if(name->IsEqual(RNCSTR("7")))
		{
			return _keyPressed[0x37];
		}
		if(name->IsEqual(RNCSTR("8")))
		{
			return _keyPressed[0x38];
		}
		if(name->IsEqual(RNCSTR("9")))
		{
			return _keyPressed[0x39];
		}

		if(name->IsEqual(RNCSTR("A")))
		{
			return _keyPressed[0x41];
		}
		if(name->IsEqual(RNCSTR("B")))
		{
			return _keyPressed[0x42];
		}
		if(name->IsEqual(RNCSTR("C")))
		{
			return _keyPressed[0x43];
		}
		if(name->IsEqual(RNCSTR("D")))
		{
			return _keyPressed[0x44];
		}
		if(name->IsEqual(RNCSTR("E")))
		{
			return _keyPressed[0x45];
		}
		if(name->IsEqual(RNCSTR("F")))
		{
			return _keyPressed[0x46];
		}
		if(name->IsEqual(RNCSTR("G")))
		{
			return _keyPressed[0x47];
		}
		if(name->IsEqual(RNCSTR("H")))
		{
			return _keyPressed[0x48];
		}
		if(name->IsEqual(RNCSTR("I")))
		{
			return _keyPressed[0x49];
		}
		if(name->IsEqual(RNCSTR("J")))
		{
			return _keyPressed[0x4A];
		}
		if(name->IsEqual(RNCSTR("K")))
		{
			return _keyPressed[0x4B];
		}
		if(name->IsEqual(RNCSTR("L")))
		{
			return _keyPressed[0x4C];
		}
		if(name->IsEqual(RNCSTR("M")))
		{
			return _keyPressed[0x4D];
		}
		if(name->IsEqual(RNCSTR("N")))
		{
			return _keyPressed[0x4E];
		}
		if(name->IsEqual(RNCSTR("O")))
		{
			return _keyPressed[0x4F];
		}
		if(name->IsEqual(RNCSTR("P")))
		{
			return _keyPressed[0x50];
		}
		if(name->IsEqual(RNCSTR("Q")))
		{
			return _keyPressed[0x51];
		}
		if(name->IsEqual(RNCSTR("R")))
		{
			return _keyPressed[0x52];
		}
		if(name->IsEqual(RNCSTR("S")))
		{
			return _keyPressed[0x53];
		}
		if(name->IsEqual(RNCSTR("T")))
		{
			return _keyPressed[0x54];
		}
		if(name->IsEqual(RNCSTR("U")))
		{
			return _keyPressed[0x55];
		}
		if(name->IsEqual(RNCSTR("V")))
		{
			return _keyPressed[0x56];
		}
		if(name->IsEqual(RNCSTR("W")))
		{
			return _keyPressed[0x57];
		}
		if(name->IsEqual(RNCSTR("X")))
		{
			return _keyPressed[0x58];
		}
		if(name->IsEqual(RNCSTR("Y")))
		{
			return _keyPressed[0x59];
		}
		if(name->IsEqual(RNCSTR("Z")))
		{
			return _keyPressed[0x5A];
		}

		if(name->IsEqual(RNCSTR("SPACE")))
		{
			return _keyPressed[0x20];
		}

		if(name->IsEqual(RNCSTR("SHIFT")))
		{
			return _keyPressed[0x10];
		}

		if (name->IsEqual(RNCSTR("ESC")))
		{
			return _keyPressed[0x1B];
		}
#endif
		
#if RN_PLATFORM_MAC_OS
		//TODO: Support all keys
		if(name->IsEqual(RNCSTR("0")))
		{
			return _keyPressed[29];
		}
		if(name->IsEqual(RNCSTR("1")))
		{
			return _keyPressed[18];
		}
		if(name->IsEqual(RNCSTR("2")))
		{
			return _keyPressed[19];
		}
		if(name->IsEqual(RNCSTR("3")))
		{
			return _keyPressed[20];
		}
		if(name->IsEqual(RNCSTR("4")))
		{
			return _keyPressed[21];
		}
		if(name->IsEqual(RNCSTR("5")))
		{
			return _keyPressed[23];
		}
		if(name->IsEqual(RNCSTR("6")))
		{
			return _keyPressed[22];
		}
		if(name->IsEqual(RNCSTR("7")))
		{
			return _keyPressed[26];
		}
		if(name->IsEqual(RNCSTR("8")))
		{
			return _keyPressed[28];
		}
		if(name->IsEqual(RNCSTR("9")))
		{
			return _keyPressed[25];
		}
		
		if(name->IsEqual(RNCSTR("A")))
		{
			return _keyPressed[0];
		}
		if(name->IsEqual(RNCSTR("B")))
		{
			return _keyPressed[11];
		}
		if(name->IsEqual(RNCSTR("C")))
		{
			return _keyPressed[8];
		}
		if(name->IsEqual(RNCSTR("D")))
		{
			return _keyPressed[2];
		}
		if(name->IsEqual(RNCSTR("E")))
		{
			return _keyPressed[14];
		}
		if(name->IsEqual(RNCSTR("F")))
		{
			return _keyPressed[3];
		}
		if(name->IsEqual(RNCSTR("G")))
		{
			return _keyPressed[5];
		}
		if(name->IsEqual(RNCSTR("H")))
		{
			return _keyPressed[4];
		}
		if(name->IsEqual(RNCSTR("I")))
		{
			return _keyPressed[34];
		}
		if(name->IsEqual(RNCSTR("J")))
		{
			return _keyPressed[38];
		}
		if(name->IsEqual(RNCSTR("K")))
		{
			return _keyPressed[40];
		}
		if(name->IsEqual(RNCSTR("L")))
		{
			return _keyPressed[37];
		}
		if(name->IsEqual(RNCSTR("M")))
		{
			return _keyPressed[46];
		}
		if(name->IsEqual(RNCSTR("N")))
		{
			return _keyPressed[45];
		}
		if(name->IsEqual(RNCSTR("O")))
		{
			return _keyPressed[31];
		}
		if(name->IsEqual(RNCSTR("P")))
		{
			return _keyPressed[35];
		}
		if(name->IsEqual(RNCSTR("Q")))
		{
			return _keyPressed[12];
		}
		if(name->IsEqual(RNCSTR("R")))
		{
			return _keyPressed[15];
		}
		if(name->IsEqual(RNCSTR("S")))
		{
			return _keyPressed[1];
		}
		if(name->IsEqual(RNCSTR("T")))
		{
			return _keyPressed[17];
		}
		if(name->IsEqual(RNCSTR("U")))
		{
			return _keyPressed[32];
		}
		if(name->IsEqual(RNCSTR("V")))
		{
			return _keyPressed[9];
		}
		if(name->IsEqual(RNCSTR("W")))
		{
			return _keyPressed[13];
		}
		if(name->IsEqual(RNCSTR("X")))
		{
			return _keyPressed[7];
		}
		if(name->IsEqual(RNCSTR("Y")))
		{
			return _keyPressed[16];
		}
		if(name->IsEqual(RNCSTR("Z")))
		{
			return _keyPressed[6];
		}
		
		if(name->IsEqual(RNCSTR("SPACE")))
		{
			return _keyPressed[49];
		}
		if(name->IsEqual(RNCSTR("SHIFT")))
		{
			return _keyPressed[56] || _keyPressed[60];
		}
		
		if (name->IsEqual(RNCSTR("ESC")))
		{
			return _keyPressed[53];
		}
#endif

#if RN_PLATFORM_LINUX
		//TODO: Support all keys
		if(name->IsEqual(RNCSTR("W")))
		{
			return _keyPressed[25];
		}
		if(name->IsEqual(RNCSTR("A")))
		{
			return _keyPressed[38];
		}
		if(name->IsEqual(RNCSTR("S")))
		{
			return _keyPressed[39];
		}
		if(name->IsEqual(RNCSTR("D")))
		{
			return _keyPressed[40];
		}
		if(name->IsEqual(RNCSTR("E")))
		{
			return _keyPressed[26];
		}
		if(name->IsEqual(RNCSTR("F")))
		{
			return _keyPressed[41];
		}
		if (name->IsEqual(RNCSTR("ESC")))
		{
			return _keyPressed[9];
		}
#endif
		
#if RN_PLATFORM_IOS
		if(name->IsEqual(RNCSTR("W")))
		{
			//Simulate W button for forward movement if more than one finger touches the screen
			/*size_t touchCount = 0;
			_lock.Lock();
			touchCount = _currentTouchCount;
			_lock.Unlock();*/
			
			return (_currentTouchCount == 2);
		}
		
		if(name->IsEqual(RNCSTR("E")))
		{
			//Simulate W button for forward movement if more than one finger touches the screen
			/*size_t touchCount = 0;
			_lock.Lock();
			touchCount = _currentTouchCount;
			_lock.Unlock();*/
			
			return (_currentTouchCount == 3);
		}
#endif

		return false;
	}
	Object *InputManager::GetControlValue(const String *name) const
	{
		size_t count = _devices->GetCount();
		for(size_t i = 0; i < count; i ++)
		{
			Object *value;
			InputDevice *device = static_cast<InputDevice *>(_devices->GetObjectAtIndex(i));
			if((value = device->GetControlValue(name)))
				return value;
		}

		return nullptr;
	}

	HIDDevice *InputManager::GetHIDDevice(uint16 vendorID, uint16 productID) const
	{
		LockGuard<Lockable> lock(const_cast<Lockable &>(_lock));
		HIDDevice *result = nullptr;

		_hidDevices->Enumerate<HIDDevice>([&](HIDDevice *device, size_t index, bool &stop) {

			if(device->GetProductID() == productID && device->GetVendorID() == vendorID)
			{
				result = device;
				stop = true;
			}

		});

		if(result)
			return result->Retain()->Autorelease();

		return nullptr;
	}

	void InputManager::__AddRawHIDDevice(HIDDevice *device)
	{
		{
			LockGuard<Lockable> lock(_lock);
			_hidDevices->AddObject(device);
		}

		NotificationManager::GetSharedInstance()->PostNotification(kRNInputManagerHIDDeviceAdded, device);
	}

	void InputManager::__RemoveRawHIDDevice(HIDDevice *device)
	{
		{
			LockGuard<Lockable> lock(_lock);
			_hidDevices->RemoveObject(device);
		}

		NotificationManager::GetSharedInstance()->PostNotification(kRNInputManagerHIDDeviceRemoved, device);
	}
}
