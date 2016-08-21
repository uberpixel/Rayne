//
//  RNInputManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNNotificationManager.h"
#include "../Debug/RNLogger.h"
#include "Devices/RNPS4Controller.h"
#include "RNInputManager.h"

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


	extern void BuildPlatformDeviceTree();
	extern void TearDownPlatformDeviceTree();

	static InputManager *__sharedInstance = nullptr;

	InputManager::InputManager() :
		_devices(new Array()),
		_bindings(new Dictionary()),
		_mouseDevices(new Array()),
		_mode(0),
		_hidDevices(new Array())
	{
		__sharedInstance = this;

#if RN_PLATFORM_WINDOWS
		memset(_keyPressed, 0, 256*sizeof(bool));

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

		PS4Controller::RegisterDriver();

		// Avoid any and all re-ordering
		std::atomic_thread_fence(std::memory_order_seq_cst);
		BuildPlatformDeviceTree();
	}

	InputManager::~InputManager()
	{
		TearDownPlatformDeviceTree();
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
		UINT dwSize;
		GetRawInputData(lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		LPBYTE lpb = new BYTE[dwSize];
		if(lpb == NULL){ return; }

		RN_ASSERT(GetRawInputData(lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize , "GetRawInputData does not return correct size !");

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
		}

		delete[] lpb;
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

#if RN_PLATFORM_WINDOWS
		_mouseDelta += _mouseMovement;
		_mouseMovement = Vector3();
#endif

		if(_mode & MouseMode::Smoothed)
		{
			float x = _mouseDelta.x * 0.5f;
			float y = _mouseDelta.y * 0.5f;

			_mouseDelta.x = _previousMouseDelta.x*0.5 + x;
			_mouseDelta.y = _previousMouseDelta.y*0.5 + y;
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

	bool InputManager::IsControlToggling(const String *name) const
	{
		size_t count = _devices->GetCount();
		for(size_t i = 0; i < count; i ++)
		{
			InputDevice *device = static_cast<InputDevice *>(_devices->GetObjectAtIndex(i));
			if(device->IsControlToggling(name))
				return true;
		}

#if RN_PLATFORM_WINDOWS
		if(name->IsEqual(RNCSTR("W")))
		{
			return _keyPressed[0x57];
		}
		if(name->IsEqual(RNCSTR("A")))
		{
			return _keyPressed[0x41];
		}
		if(name->IsEqual(RNCSTR("S")))
		{
			return _keyPressed[0x53];
		}
		if(name->IsEqual(RNCSTR("D")))
		{
			return _keyPressed[0x44];
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
