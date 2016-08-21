//
//  RNHIDDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_HIDDEVICE_H_
#define __RAYNE_HIDDEVICE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNData.h"
#include "RNHID.h"
#include "RNInputDevice.h"

namespace RN
{
	class HIDDevice : public Object
	{
	public:
		RNAPI void Register();
		RNAPI void Unregister();

		RNAPI virtual void Open() = 0;
		RNAPI virtual void Close() = 0;

		RNAPI virtual Data *ReadReport(uint32 reportID) const = 0;
		RNAPI virtual Data *ReadFeatureReport(uint32 reportID) const = 0;

		RNAPI virtual size_t WriteReport(uint32 reportID, const Data *data) = 0;

		bool IsVendor() const { return static_cast<uint16>(_usagePage) >= static_cast<uint16>(HIDUsagePage::VendorDefinedStart); }

		HIDUsagePage GetUsagePage() const { return _usagePage; }

		template<class T>
		T GetUsage() const
		{
			static_assert(std::is_enum<T>::value, "T must be an enum");
			return static_cast<T>(_usage);
		}

		RNAPI virtual const String *GetManufacturerString() const = 0;
		RNAPI virtual const String *GetProductString() const = 0;
		RNAPI virtual const String *GetSerialString() const = 0;

		RNAPI virtual size_t GetInputReportLength() const = 0;
		RNAPI virtual size_t GetOutputReportLength() const = 0;
		RNAPI virtual size_t GetFeatureReportLength() const = 0;

		RNAPI virtual uint32 GetVendorID() const = 0;
		RNAPI virtual uint32 GetProductID() const = 0;

		RNAPI const String *GetDescription() const override;

		RNAPI InputDevice::Descriptor GetDescriptor() const;

	protected:
		RNAPI HIDDevice(HIDUsagePage usagePage, uint16 usage);
		RNAPI ~HIDDevice();

	private:
		HIDUsagePage _usagePage;
		uint16 _usage;

		__RNDeclareMetaInternal(HIDDevice)
	};

	RNExceptionType(HIDRead)
	RNExceptionType(HIDWrite)
	RNExceptionType(HIDOpen)
}


#endif /* __RAYNE_HIDDEVICE_H_ */
