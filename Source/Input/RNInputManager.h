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
#include "RNInputDevice.h"

namespace RN
{
	class InputManager
	{
	public:
		friend class Kernel;
		friend class InputDevice;

		RN_OPTIONS(Action, uint32,
				   DidStart = (1 << 0),
				   DidEnd = (1 << 2),
				   DidUpdate = (1 << 3));

		using Callback = std::function<void (InputDevice *device, InputControl *control, Object *value, Action action)>;


		RNAPI static InputManager *GetSharedInstance();

		RNAPI Array *GetDevicesWithCategories(InputDevice::Category categories);

		RNAPI void AddTarget(void *target, Action actions, InputDevice::Category category, Callback &&callback);
		RNAPI void AddTarget(void *target, Action actions, InputDevice *device, Callback &&callback);

		RNAPI void RemoveTarget(void *target, Action actions, InputDevice *device);
		RNAPI void RemoveTarget(void *target, Action actions);
		RNAPI void RemoveTarget(void *target);

	private:
		InputManager();
		~InputManager();

		void __AddDevice(InputDevice *device);
		void __RemoveDevice(InputDevice *device);

		struct Target
		{
			Target(void *ttarget, Action tactions, InputDevice *tdevice, InputDevice::Category tcategories, Callback &&tcallback) :
				callback(std::move(tcallback)),
				device(tdevice),
				target(ttarget),
				categories(tcategories),
				actions(tactions)
			{}

			Target(Target &&other) = default;
			Target &operator =(Target &&other) = default;

			Callback callback;
			InputDevice *device;
			InputDevice::Category categories;
			Action actions;
			void *target;
		};

		void SendActionToTargets(Action action, InputDevice *device, InputControl *control, Object *value);
		void Update(float delta);

		std::mutex _lock;
		Array *_devices;
		std::vector<Target> _targets;
	};
}


#endif /* __RAYNE_INPUTMANAGER_H_ */
