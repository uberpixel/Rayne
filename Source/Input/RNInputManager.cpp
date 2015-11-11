//
//  RNInputManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInputManager.h"

namespace RN
{
	extern void BuildPlatformDeviceTree();

	static InputManager *__sharedInstance = nullptr;

	InputManager::InputManager() :
		_devices(new Array())
	{
		__sharedInstance = this;

		// Avoid any and all re-ordering
		std::atomic_thread_fence(std::memory_order_seq_cst);
		BuildPlatformDeviceTree();
	}

	InputManager::~InputManager()
	{
		_devices->Release();
	}

	InputManager *InputManager::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void InputManager::__AddDevice(InputDevice *device)
	{
		std::lock_guard<std::mutex> lock(_lock);

		RN_ASSERT(device->IsRegistered() == false, "Device is already registered");
		device->_manager = this;

		_devices->AddObject(device);
	}

	void InputManager::__RemoveDevice(InputDevice *device)
	{
		std::lock_guard<std::mutex> lock(_lock);

		RN_ASSERT(device->IsRegistered(), "Device must be registered");
		device->_manager = nullptr;

		_devices->RemoveObject(device);
	}

	void InputManager::Update(float delta)
	{
		RN::Array *devices;

		{
			std::lock_guard<std::mutex> lock(_lock);
			devices = _devices->Copy()->Autorelease();
		}

		devices->Enumerate<InputDevice>([&](InputDevice *device, size_t index, bool &stop) {
			device->Update();
		});
	}


	Array *InputManager::GetDevicesWithCategories(InputDevice::Category categories)
	{
		Array *result = new Array();

		{
			std::lock_guard<std::mutex> lock(_lock);
			_devices->Enumerate<InputDevice>([&](InputDevice *device, size_t index, bool &stop) {

				if(device->GetCategory() & categories)
					result->AddObject(result);

			});
		}

		return result->Autorelease();
	}


	void InputManager::AddTarget(void *target, Action actions, InputDevice::Category category, Callback &&callback)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_targets.emplace_back(target, actions, nullptr, category, std::move(callback));
	}
	void InputManager::AddTarget(void *target, Action actions, InputDevice *device, Callback &&callback)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_targets.emplace_back(target, actions, device, 0, std::move(callback));
	}

	void InputManager::RemoveTarget(void *target, Action actions, InputDevice *device)
	{
		std::lock_guard<std::mutex> lock(_lock);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if(iterator->target == target && iterator->actions & actions && iterator->device == device)
			{
				iterator->actions &= ~actions;

				if(iterator->actions == 0)
				{
					iterator = _targets.erase(iterator);
					continue;
				}
			}

			iterator ++;
		}
	}

	void InputManager::RemoveTarget(void *target, Action actions)
	{
		std::lock_guard<std::mutex> lock(_lock);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if(iterator->target == target && iterator->actions & actions)
			{
				iterator->actions &= ~actions;

				if(iterator->actions == 0)
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
		std::lock_guard<std::mutex> lock(_lock);

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

	void InputManager::SendActionToTargets(Action action, InputDevice *device, InputControl *control, Object *value)
	{
		std::lock_guard<std::mutex> lock(_lock);

		for(auto iterator = _targets.begin(); iterator != _targets.end();)
		{
			if((iterator->actions & action) && (iterator->device == device || (iterator->device == nullptr && iterator->categories & device->GetCategory())))
				iterator->callback(device, control, value, action);

			iterator ++;
		}
	}
}
