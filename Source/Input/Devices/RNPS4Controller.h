//
//  RNPS4Controller.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PS4CONTROLLER_H_
#define __RAYNE_PS4CONTROLLER_H_

#include "../../Base/RNBase.h"
#include "../RNInputDevice.h"
#include "../RNHIDDevice.h"

namespace RN
{
	class PS4Controller : public InputDevice
	{
	public:
		RNAPI static void RegisterDriver();

		RNAPI PS4Controller(HIDDevice *device);
		RNAPI ~PS4Controller();

		RNAPI void Update() override;

		RNAPI Object *SetRumble(Object *value);
		RNAPI Object *SetLight(Object *value);

	protected:
		RNAPI bool __Activate() final;
		RNAPI bool __Deactivate() final;

	private:
		void SendReport();
		void Reset();

		HIDDevice *_device;

		Linear2DAxisControl *_analogLeft;
		Linear2DAxisControl *_analogRight;
		
		ButtonControl *_buttonCross;

		uint8 _rumbleLarge;
		uint8 _rumbleSmall;

		uint8 _ledRed;
		uint8 _ledGreen;
		uint8 _ledBlue;

		RNDeclareMeta(PS4Controller)
	};
}


#endif /* __RAYNE_PS4CONTROLLER_H_ */
