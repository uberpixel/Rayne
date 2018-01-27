//
//  RNPS4Controller.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPS4Controller.h"
#include "../../Base/RNNotificationManager.h"
#include "../../Objects/RNAutoreleasePool.h"
#include "../../Objects/RNValue.h"
#include "../../Debug/RNLogger.h"
#include "../RNInputManager.h"

namespace RN
{
	RNDefineMeta(PS4Controller, InputDevice)

	void PS4Controller::RegisterDriver()
	{
		NotificationManager::GetSharedInstance()->AddSubscriber(kRNInputManagerHIDDeviceAdded, [](Notification *notification) {

			HIDDevice *device = notification->GetInfo<HIDDevice>();
			if(device->GetVendorID() == 0x54c && device->GetProductID() == 0x05c4)
			{
				PS4Controller *controller = new PS4Controller(device);
				controller->Register();
				controller->Release();
			}

		}, reinterpret_cast<void *>(&PS4Controller::RegisterDriver));
	}

	PS4Controller::PS4Controller(HIDDevice *device) :
		InputDevice(device->GetDescriptor()),
		_device(SafeRetain(device))
	{
		BindCommand(RNUTF8STR("rumble"), std::bind(&PS4Controller::SetRumble, this, std::placeholders::_1), { Number::GetMetaClass(), Array::GetMetaClass() });
		BindCommand(RNUTF8STR("light"), std::bind(&PS4Controller::SetLight, this, std::placeholders::_1), { Value::GetMetaClass() });


		_analogLeft = new Linear2DAxisControl(RNCSTR("Analog Left"));
		_analogLeft->SetRange(Vector2(-128), Vector2(128), Vector2(12));

		_analogRight = new Linear2DAxisControl(RNCSTR("Analog Right"));
		_analogRight->SetRange(Vector2(-128), Vector2(128), Vector2(12));
		
		_buttonCross = new ButtonControl(RNCSTR("Button Cross"), Type::Button);

		AddControl(_analogLeft);
		AddControl(_analogRight);
		AddControl(_buttonCross);
	}

	PS4Controller::~PS4Controller()
	{
		_analogLeft->Release();
		_analogRight->Release();
		_buttonCross->Release();

		SafeRelease(_device);
	}

	bool PS4Controller::__Activate()
	{
		try
		{
			_device->Open();
			Reset();

			return true;
		}
		catch(HIDOpenException &e)
		{
			RNWarning("Failed to open PS4 HIDDevice " << e);
			return false;
		}
	}

	bool PS4Controller::__Deactivate()
	{
		_device->Close();
		return true;
	}


	void PS4Controller::Update()
	{
		SendReport();

		Data *report = _device->ReadReport(0x01);
		if(report)
		{
			const uint8 *data = static_cast<uint8 *>(report->GetBytes());

			Vector2 left(data[1], data[2]);
			Vector2 right(data[3], data[4]);

			_analogLeft->SetValue(left - 128);
			_analogRight->SetValue(right - 128);
			
			uint8 buttons = data[5]; //Triangle, Circle, Cross, Square (bit7, bit6, bit5, bit4)
			_buttonCross->SetPressed(buttons & (1<<7));
		}
	}

	Object *PS4Controller::SetRumble(Object *value)
	{
		if(value->IsKindOfClass(Number::GetMetaClass()))
		{
			Number *number = value->Downcast<Number>();
			_rumbleLarge = _rumbleSmall = number->GetUint8Value();
		}

		if(value->IsKindOfClass(Array::GetMetaClass()))
		{
			Array *array = value->Downcast<Array>();

			if(array->GetCount() != 2)
				throw InvalidArgumentException("Array must have two entries (RN::Number)");

			Number *large = array->GetObjectAtIndex<Number>(0);
			Number *tiny = array->GetObjectAtIndex<Number>(1);

			_rumbleLarge = large->GetUint8Value();
			_rumbleSmall = tiny->GetUint8Value();
		}

		return nullptr;
	}
	Object *PS4Controller::SetLight(Object *value)
	{
		Value *object = value->Downcast<Value>();
		if(object && object->CanConvertToType<Vector3>())
		{
			Vector3 vector = object->GetValue<Vector3>();
			vector.Normalize();

			_ledRed   = static_cast<uint8>(vector.x * 255);
			_ledGreen = static_cast<uint8>(vector.y * 255);
			_ledBlue  = static_cast<uint8>(vector.z * 255);
		}

		return nullptr;
	}

	void PS4Controller::Reset()
	{
		_rumbleLarge = 0;
		_rumbleSmall = 0;

		_ledRed = _ledGreen = _ledBlue = 0;

		SendReport();
	}

	void PS4Controller::SendReport()
	{
		uint8 report[32];
		memset(report, 0x0, 32);

		report[0] = 0x05;
		report[1] = 0xff;

		report[4] = _rumbleLarge;
		report[5] = _rumbleSmall;

		report[6] = _ledRed;
		report[7] = _ledGreen;
		report[8] = _ledBlue;

		Data *data = new Data(report, 32, true, false);
		_device->WriteReport(0x05, data);
		data->Release();
	}
}
