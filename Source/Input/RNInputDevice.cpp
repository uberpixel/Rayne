//
//  RNInputDevice.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInputDevice.h"
#include "RNInputManager.h"

namespace RN
{
	RNDefineMeta(InputDevice, InputControl)

	InputDevice::InputDevice(const Descriptor &descriptor) :
		InputControl(descriptor.GetName(), Type::Group),
		_category(descriptor.GetCategory()),
		_serialNumber(nullptr),
		_active(false),
		_manager(nullptr)
	{
		_vendor = SafeCopy(descriptor.GetVendor());
		_vendorID = SafeCopy(descriptor.GetVendorID());
		_productID = SafeCopy(descriptor.GetProductID());

		RN_ASSERT(_vendorID && _productID, "Product and Vendor ID must be set in InputDevice::Descriptor");
	}

	InputDevice::~InputDevice()
	{
		SafeRelease(_serialNumber);
		SafeRelease(_vendor);
		SafeRelease(_vendorID);
		SafeRelease(_productID);
	}

	void InputDevice::AddControl(InputControl *control)
	{
		_controls.PushBack(control->_controlsEntry);
		control->__LinkIntoDeviceWithParent(this, this);
		control->Retain();
	}

	void InputDevice::SetSerialNumber(const String *serialNumber)
	{
		SafeRelease(_serialNumber);
		_serialNumber = SafeCopy(serialNumber);
	}

	void InputDevice::Register()
	{
		InputManager::GetSharedInstance()->__AddDevice(this);
	}
	void InputDevice::Unregister()
	{
		InputManager::GetSharedInstance()->__RemoveDevice(this);
	}


	bool InputDevice::Activate() RN_NOEXCEPT
	{
		RN_ASSERT(!_active, "InputDevice must not be activated already");
		RN_ASSERT(_manager, "InputDevice must be registered");

		bool result;

		try
		{
			result = __Activate();
		}
		catch(...)
		{}

		_active = (result == true);
		return result;
	}

	bool InputDevice::Deactivate() RN_NOEXCEPT
	{
		RN_ASSERT(_active, "InputDevice must be activated");
		RN_ASSERT(_manager, "InputDevice must be registered");

		bool result;

		try
		{
			result = __Deactivate();
		}
		catch(...)
		{}

		_active = (result == false);
		return result;
	}

	Array *InputDevice::GetSupportedCommands() const
	{
		Array *result = new Array();

		for(const ExecutionPort &port : _executionPorts)
			result->AddObject(port.command);

		return result->Autorelease();
	}

	std::vector<MetaClass *> InputDevice::GetSupportArgumentsForCommand(const String *command) const
	{
		const ExecutionPort *port = GetExecutionPortMatching(command, nullptr);
		if(!port)
			throw InvalidArgumentException(RNSTR("Unknown command " << command));

		return port->supportedArguments;
	}


	bool InputDevice::SupportsCommand(const String *command) const
	{
		return (GetExecutionPortMatching(command, nullptr) != nullptr);
	}

	bool InputDevice::SupportsCommand(const String *command, MetaClass *meta) const
	{
		return (GetExecutionPortMatching(command, meta) != nullptr);
	}

	Object *InputDevice::ExecuteCommand(const String *command, Object *argument)
	{
		if(!IsActive())
			throw InconsistencyException("Can't execute commands on de-activate devices");

		const ExecutionPort *port = GetExecutionPortMatching(command, argument->GetClass());
		if(!port)
			throw InvalidArgumentException("ExecuteCommand with unsupport command/argument combo!");

		return port->action(argument);
	}

	const InputDevice::ExecutionPort *InputDevice::GetExecutionPortMatching(const String *command, MetaClass *meta) const
	{
		for(const ExecutionPort &port : _executionPorts)
		{
			if(command->IsEqual(port.command))
			{
				if(!meta)
					return &port;

				auto iterator = std::find(port.supportedArguments.begin(), port.supportedArguments.end(), meta);
				if(iterator != port.supportedArguments.end())
					return &port;
			}
		}

		return nullptr;
	}

	void InputDevice::BindCommand(const String *command, std::function<Object * (Object *)> &&action, std::vector<MetaClass *> &&arguments)
	{
		auto iterator = _executionPorts.emplace(_executionPorts.end(), command->Copy()->Autorelease());
		iterator->action = std::move(action);
		iterator->supportedArguments = std::move(arguments);
	}


	void InputDevice::ControlDidStart(InputControl *control)
	{
		if(!_active)
			return;

		_manager->PerformEvent(InputManager::Event::DidStart, this, control, Number::WithBool(true));
	}
	void InputDevice::ControlDidEnd(InputControl *control)
	{
		if(!_active)
			return;

		_manager->PerformEvent(InputManager::Event::DidEnd, this, control, Number::WithBool(false));
	}
	void InputDevice::ControlDidUpdate(InputControl *control, Object *value)
	{
		if(!_active)
			return;

		_manager->PerformEvent(InputManager::Event::DidUpdate, this, control, value);
	}
}
