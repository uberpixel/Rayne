//
//  RNInputManager.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTMANAGER_H_
#define __RAYNE_INPUTMANAGER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNDictionary.h"
#include "RNInputDevice.h"

namespace RN
{
	class InputManager
	{
	public:
		friend class Kernel;
		friend class InputDevice;

		RN_OPTIONS(Event, uint32,
				   DidStart = (1 << 0),
				   DidEnd = (1 << 2),
				   DidUpdate = (1 << 3));

		RN_OPTIONS(MouseMode, uint32,
				   Hidden = (1 << 0),
				   Captured = (1 << 2));

		struct Action
		{
			InputDevice *device;
			InputControl *control;
			Object *value;
			Event event;
		};

		using Callback = std::function<void (const Action &action)>;


		RNAPI static InputManager *GetSharedInstance();

		RNAPI Array *GetDevicesWithCategories(InputDevice::Category categories);

		RNAPI void SetMouseMode(MouseMode mode);

		RNAPI void AddTarget(void *target, Event events, InputDevice::Category category, Callback &&callback);
		RNAPI void AddTarget(void *target, Event events, InputDevice *device, Callback &&callback);

		RNAPI void RemoveTarget(void *target, Event events, InputDevice *device);
		RNAPI void RemoveTarget(void *target, Event events);
		RNAPI void RemoveTarget(void *target);

		RNAPI void Bind(void *target, const String *name, Callback &&callback);
		RNAPI void Unbind(void *target, const String *name);

		RNAPI bool IsControlToggling(const String *name) const;
		RNAPI Object *GetControlValue(const String *name) const;

		RNAPI const Vector2 &GetMouseDelta() const { return _mouseDelta; }

	private:
		InputManager();
		~InputManager();

		void __AddDevice(InputDevice *device);
		void __RemoveDevice(InputDevice *device);

		struct Target
		{
			Target(void *ttarget, Event tevents, InputDevice *tdevice, InputDevice::Category tcategories, Callback &&tcallback) :
				callback(std::move(tcallback)),
				device(tdevice),
				target(ttarget),
				categories(tcategories),
				events(tevents)
			{}

			Target(Target &&other) = default;
			Target &operator =(Target &&other) = default;

			Callback callback;
			InputDevice *device;
			InputDevice::Category categories;
			Event events;
			void *target;
		};

		void PerformEvent(Event event, InputDevice *device, InputControl *control, Object *value);
		void Update(float delta);

		std::mutex _lock;
		Array *_devices;
		std::vector<Target> _targets;
		Dictionary *_bindings;

		MouseMode _mode;

		Array *_mouseDevices;
		Vector2 _previousMouseDelta;
		Vector2 _mouseDelta;
	};
}


#endif /* __RAYNE_INPUTMANAGER_H_ */
