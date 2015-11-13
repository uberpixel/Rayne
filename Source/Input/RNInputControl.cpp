//
//  RNInputControl.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInputControl.h"
#include "RNInputDevice.h"

namespace RN
{
	RNDefineMeta(InputControl, Object)

	InputControl::InputControl(const String *name, Type type) :
		_type(type),
		_controlsEntry(this),
		_name(SafeCopy(name)),
		_device(nullptr),
		_parent(nullptr),
		_value(nullptr),
		_toggling(false)
	{}

	InputControl::~InputControl()
	{
		auto member = _controls.GetHead();
		while(member)
		{
			auto next = member->GetNext();
			member->Get()->Release();

			member = next;
		}

		SafeRelease(_name);
		SafeRelease(_value);
	}

	void InputControl::__LinkIntoDeviceWithParent(InputDevice *device, InputControl *parent)
	{
		RN_ASSERT(_device == nullptr, "Added InputControl multiple times into control hierarchy");

		_device = device;
		_parent = parent;

		auto member = _controls.GetHead();
		while(member)
		{
			member->Get()->_device = nullptr;
			member->Get()->__LinkIntoDeviceWithParent(device, this);

			member = member->GetNext();
		}
	}

	void InputControl::AddControl(InputControl *control)
	{
		_controls.PushBack(control->_controlsEntry);
		control->__LinkIntoDeviceWithParent(_device, this);
		control->Retain();
	}

	InputControl *InputControl::GetFirstControl() const
	{
		auto member = _controls.GetHead();
		if(member)
			return member->Get();

		return nullptr;
	}

	InputControl *InputControl::GetNextControl() const
	{
		if(_controls.GetCount() > 0)
			return _controls.GetHead()->Get();

		auto next = _controlsEntry.GetNext();
		if(next)
			return next->Get();

		if(_parent)
		{
			auto next = _parent->_controlsEntry.GetNext();
			if(next)
				return next->Get();
		}

		return nullptr;
	}

	InputControl *InputControl::GetControlWithName(const String *name) const
	{
		InputControl *control = GetFirstControl();

		while(control)
		{
			if(control->GetName()->IsEqual(name))
				return control;

			control = control->GetNextControl();
		}

		return nullptr;
	}


	void InputControl::Start()
	{
		RN_ASSERT(_type == Type::Toggle, "Only toggle controls can start");
		_toggling = true;
		_device->ControlDidStart(this);
	}

	void InputControl::End()
	{
		RN_ASSERT(_type == Type::Toggle, "Only toggle controls can end");
		_toggling = false;
		_device->ControlDidEnd(this);
	}

	void InputControl::UpdateValue(Object *value)
	{
		RN_ASSERT(value, "Value mustn't be NULL");
		RN_ASSERT(_type == Type::Continuous, "Only continous controls can have values");

		SafeRelease(_value);
		_value = SafeRetain(value);

		_device->ControlDidUpdate(this, value);
	}

	bool InputControl::IsControlToggling(const String *name) const
	{
		RN_ASSERT(_type == Type::Group, "Only groups can query their child controls");

		InputControl *control = GetFirstControl();
		while(control)
		{
			if(control->GetType() == Type::Toggle && control->GetName()->IsEqual(name))
			{
				bool result = control->_toggling;
				if(result)
					return result;
			}

			control = control->GetNextControl();
		}

		return false;
	}
	Object *InputControl::GetControlValue(const String *name) const
	{
		RN_ASSERT(_type == Type::Group, "Only groups can query their child controls");

		InputControl *control = GetFirstControl();
		while(control)
		{
			if(control->GetType() == Type::Continuous && control->GetName()->IsEqual(name))
				return control->_value;

			control = control->GetNextControl();
		}

		return nullptr;
	}


	RNDefineMeta(InputControlGroup, InputControl)

	InputControlGroup::InputControlGroup() :
		InputControl(nullptr, Type::Group)
	{}

	RNDefineMeta(ButtonControl, InputControl)

	ButtonControl::ButtonControl(const String *name) :
		InputControl(name, Type::Toggle),
		_pressed(false)
	{}

	void ButtonControl::SetPressed(bool pressed)
	{
		if(_pressed == pressed)
			return;

		pressed ? Start() : End();
		_pressed = pressed;
	}
}
