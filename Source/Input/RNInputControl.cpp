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
		_toggling(false),
		_controlMap(new Dictionary()),
		_controlGroups(new Array())
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

		_controlMap->Release();
		_controlGroups->Release();

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
		// Changes here should be mirrored in InputDevice::AddControl, if appropriate

		_controls.PushBack(control->_controlsEntry);
		control->__LinkIntoDeviceWithParent(_device, this);
		control->Retain();

		if(control->GetName())
			_controlMap->SetObjectForKey(control, control->GetName());

		if(control->GetType() == Type::Group)
			_controlGroups->AddObject(control);
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

	InputControl *InputControl::__GetControlWithName(const String *name) const
	{
		InputControl *control = _controlMap->GetObjectForKey<InputControl>(name);
		if(control)
			return control;

		size_t count = _controlGroups->GetCount();
		for(size_t i = 0; i < count; i ++)
		{
			control = static_cast<InputControl *>(_controlGroups->GetObjectAtIndex(i));

			InputControl *result = control->GetControlWithName(name);
			if(result)
				return result;
		}

		return nullptr;
	}

	Array *InputControl::GetControlsWithType(Type type) const
	{
		Array *array = new Array();

		InputControl *control = GetFirstControl();
		while(control)
		{
			if(control->GetType() == type)
				array->AddObject(control);

			control = control->GetNextControl();
		}

		return array->Autorelease();
	}

	void InputControl::Update()
	{
		auto control = _controls.GetHead();
		while(control)
		{
			InputControl *inputControl = control->Get();
			inputControl->Update();

			control = control->GetNext();
		}
	}


	void InputControl::Start()
	{
		RN_ASSERT(IsToggle(), "Only toggle controls can start");
		_toggling = true;
		_device->ControlDidStart(this);
	}

	void InputControl::End()
	{
		RN_ASSERT(IsToggle(), "Only toggle controls can end");
		_toggling = false;
		_device->ControlDidEnd(this);
	}

	void InputControl::UpdateValue(Object *value)
	{
		RN_ASSERT(value, "Value mustn't be NULL");
		RN_ASSERT(IsContinuous(), "Only continous controls can have values");

		SafeRelease(_value);
		_value = SafeRetain(value);

		_device->ControlDidUpdate(this, value);
	}

	bool InputControl::IsControlToggling(const String *name) const
	{
		RN_ASSERT(_type == Type::Group, "Only groups can query their child controls");

		InputControl *control;

		if((control = GetControlWithName(name)))
		{
			if(control->IsToggle())
				return control->_toggling;
		}

		return false;
	}
	Object *InputControl::GetControlValue(const String *name) const
	{
		RN_ASSERT(_type == Type::Group, "Only groups can query their child controls");

		InputControl *control;

		if((control = GetControlWithName(name)))
		{
			if(control->IsContinuous())
				return control->_value;
		}

		return nullptr;
	}

	bool InputControl::IsGroup() const
	{
		return false;
	}
	bool InputControl::IsToggle() const
	{
		return false;
	}
	bool InputControl::IsContinuous() const
	{
		return false;
	}


	RNDefineMeta(InputControlGroup, InputControl)

	InputControlGroup::InputControlGroup() :
		InputControl(nullptr, Type::Group)
	{}

	bool InputControlGroup::IsGroup() const
	{
		return true;
	}


	RNDefineMeta(ButtonControl, InputControl)

	ButtonControl::ButtonControl(const String *name, Type type) :
		InputControl(name, type),
		_pressed(false)
	{}

	void ButtonControl::SetPressed(bool pressed)
	{
		if(_pressed == pressed)
			return;

		pressed ? Start() : End();
		_pressed = pressed;
	}

	bool ButtonControl::IsToggle() const
	{
		return true;
	}

	RNDefineMeta(SliderControl, InputControl)

	SliderControl::SliderControl(const String *name) :
		InputControl(name, Type::Slider),
		_max(0.0f),
		_normalizer(0.0f),
		_deadZone(0.0f)
	{}

	void SliderControl::SetRange(float min, float max, float deadZone)
	{
		_max = max;
		_deadZone = min + deadZone;
		_normalizer = 1.0f / (max - deadZone);
	}
	void SliderControl::SetValue(float value)
	{
		if(value < _deadZone)
		{
			UpdateValue(Number::WithFloat(0.0f));
			return;
		}

		float v = (std::min(v, _max) - _deadZone) * _normalizer;
		UpdateValue(Number::WithFloat(v));
	}

	RNDefineMeta(AxisControl, InputControl)

	AxisControl::AxisControl(const String *name, Type type, Axis axis) :
		InputControl(name, type),
		_axis(axis),
		_deadZone(0.0f),
		_center(0.0),
		_min(FLT_MIN),
		_max(FLT_MAX),
		_normalizer(1.0)
	{}

	void AxisControl::SetRange(float min, float max, float deadZone)
	{
		_center = (min + max) * 0.5f;

		_min = min - _center;
		_max = max - _center;
		_deadZone = deadZone;
		_normalizer = 1.0f / (_max - _deadZone);
	}

	void AxisControl::SetValue(float value)
	{
		UpdateValue(Number::WithFloat(value));
	}

	bool AxisControl::IsContinuous() const
	{
		return true;
	}

	RNDefineMeta(DeltaAxisControl, AxisControl)

	DeltaAxisControl::DeltaAxisControl(const String *name, Axis axis) :
		AxisControl(name, Type::DeltaAxis, axis)
	{}

	void DeltaAxisControl::Update()
	{
		InputControl::Update();
		UpdateValue(Number::WithFloat(0.0));
	}

	void DeltaAxisControl::SetValue(float value)
	{
		Number *current = GetValue<Number>();
		if(current)
			value += current->GetFloatValue();

		UpdateValue(Number::WithFloat(value));
	}

	RNDefineMeta(LinearAxisControl, AxisControl)

	LinearAxisControl::LinearAxisControl(const String *name, Axis axis) :
		AxisControl(name, Type::LinearAxis, axis)
	{}

	void LinearAxisControl::SetValue(float value)
	{
		float v = value - GetCenter();
		float deadZone = GetDeadZone();
		float normalizer = GetNormalizer();

		if(Math::FastAbs(v) < deadZone)
		{
			UpdateValue(Number::WithFloat(0.0f));
			return;
		}

		if(v > 0.0f)
		{
			v = (std::min(v, GetMax()) - deadZone) * normalizer;
			UpdateValue(Number::WithFloat(v));
		}
		else
		{
			v = (std::max(v, GetMin()) + deadZone) * normalizer;
			UpdateValue(Number::WithFloat(v));
		}
	}

	RNDefineMeta(RotationAxisControl, AxisControl)

	RotationAxisControl::RotationAxisControl(const String *name, Axis axis) :
		AxisControl(name, Type::RotationAxis, axis)
	{}

	void RotationAxisControl::SetValue(float value)
	{
		float v = value - GetCenter();
		float deadZone = GetDeadZone();
		float normalizer = GetNormalizer();

		if(Math::FastAbs(v) < deadZone)
		{
			UpdateValue(Number::WithFloat(0.0f));
			return;
		}

		if(v > 0.0f)
		{
			v = (std::min(v, GetMax()) - deadZone) * normalizer;
			UpdateValue(Number::WithFloat(v));
		}
		else
		{
			v = (std::max(v, GetMin()) + deadZone) * normalizer;
			UpdateValue(Number::WithFloat(v));
		}
	}
}
