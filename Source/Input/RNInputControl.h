//
//  RNInputControl.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTCONTROL_H_
#define __RAYNE_INPUTCONTROL_H_

#include "../Base/RNBase.h"
#include "../Data/RNIntrusiveList.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNNumber.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"

namespace RN
{
	class InputDevice;
	class InputControl : public Object
	{
	public:
		friend class InputDevice;

		enum class Type
		{
			Group,
			Continuous,
			Toggle
		};

		RNAPI ~InputControl() override;

		template<class T>
		T *GetValue() const
		{
			Object *value = __GetValue();
			if(value)
				return value->Downcast<T>();

			return nullptr;
		}

		Type GetType() const { return _type; }
		const String *GetName() const { return _name; }
		InputDevice *GetDevice() const { return _device; }
		InputControl *GetParent() const { return _parent; }

		RNAPI InputControl *GetFirstControl() const;
		RNAPI InputControl *GetNextControl() const;
		RNAPI InputControl *GetControlWithName(const String *name) const;

		RNAPI virtual void AddControl(InputControl *control);

		RNAPI bool IsControlToggling(const String *name) const;
		RNAPI Object *GetControlValue(const String *name) const;

	protected:
		RNAPI InputControl(const String *name, Type type);

		RNAPI void Start();
		RNAPI void End();
		RNAPI void UpdateValue(Object *value);

	private:
		Object *__GetValue() const { return _value; }
		void __LinkIntoDeviceWithParent(InputDevice *device, InputControl *parent);

		String *_name;
		InputDevice *_device;
		InputControl *_parent;

		Type _type;
		Object *_value;
		bool _toggling;

		IntrusiveList<InputControl> _controls;
		IntrusiveList<InputControl>::Member _controlsEntry;

		RNDeclareMeta(InputControl)
	};

	class InputControlGroup : public InputControl
	{
	public:
		InputControlGroup();

	private:
		RNDeclareMeta(InputControlGroup);
	};

	class ButtonControl : public InputControl
	{
	public:
		ButtonControl(const String *name);

		void SetPressed(bool pressed);
		bool IsPressed() const { return _pressed; }

	private:
		bool _pressed;
		RNDeclareMeta(ButtonControl)
	};
}

#endif /* __RAYNE_INPUTCONTROL_H_ */
