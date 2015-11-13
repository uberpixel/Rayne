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

	static InputManager *__sharedInstance = nullptr;

	InputManager::InputManager() :
		_devices(new Array()),
		_bindings(new Dictionary())
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


	void InputManager::AddTarget(void *target, Event events, InputDevice::Category category, Callback &&callback)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_targets.emplace_back(target, events, nullptr, category, std::move(callback));
	}
	void InputManager::AddTarget(void *target, Event events, InputDevice *device, Callback &&callback)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_targets.emplace_back(target, events, device, 0, std::move(callback));
	}

	void InputManager::RemoveTarget(void *target, Event events, InputDevice *device)
	{
		std::lock_guard<std::mutex> lock(_lock);

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
		std::lock_guard<std::mutex> lock(_lock);

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

	void InputManager::Bind(void *target, const String *name, Callback &&callback)
	{
		std::lock_guard<std::mutex> lock(_lock);

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
		std::lock_guard<std::mutex> lock(_lock);

		InputBindPoint *bindPoint = _bindings->GetObjectForKey<InputBindPoint>(name);
		if(bindPoint)
		{
			if(bindPoint->RemoveToken(target))
				_bindings->RemoveObjectForKey(name);
		}
	}
	
	void InputManager::PerformEvent(Event event, InputDevice *device, InputControl *control, Object *value)
	{
		std::lock_guard<std::mutex> lock(_lock);

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
}
