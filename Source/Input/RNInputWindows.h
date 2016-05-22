//
//  RNInputWindows.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTWINDOWS_H_
#define __RAYNE_INPUTWINDOWS_H_

#include "../Base/RNBase.h"
#include "RNInputManager.h"
#include "RNHIDDevice.h"

#include <ntdef.h>
#include <winbase.h>

#ifdef __cplusplus
extern "C" {
#endif
	#include <setupapi.h>
	#include <winioctl.h>

	/* Copied from inc/ddk/hidclass.h, part of the Windows DDK. */
	#define HID_OUT_CTL_CODE(id)  \
				CTL_CODE(FILE_DEVICE_KEYBOARD, (id), METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
	#define IOCTL_HID_GET_FEATURE                   HID_OUT_CTL_CODE(100)

#ifdef __cplusplus
} /* extern "C" */
#endif

typedef struct _HIDD_ATTRIBUTES{
	ULONG Size;
	USHORT VendorID;
	USHORT ProductID;
	USHORT VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

typedef USHORT USAGE;
typedef struct _HIDP_CAPS {
	USAGE Usage;
	USAGE UsagePage;
	USHORT InputReportByteLength;
	USHORT OutputReportByteLength;
	USHORT FeatureReportByteLength;
	USHORT Reserved[17];
	USHORT fields_not_used_by_hidapi[10];
} HIDP_CAPS, *PHIDP_CAPS;
typedef void* PHIDP_PREPARSED_DATA;
#define HIDP_STATUS_SUCCESS 0x110000

typedef BOOLEAN (__stdcall *HidD_GetAttributes_)(HANDLE device, PHIDD_ATTRIBUTES attrib);
typedef BOOLEAN (__stdcall *HidD_GetSerialNumberString_)(HANDLE device, PVOID buffer, ULONG buffer_len);
typedef BOOLEAN (__stdcall *HidD_GetManufacturerString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
typedef BOOLEAN (__stdcall *HidD_GetProductString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
typedef BOOLEAN (__stdcall *HidD_SetFeature_)(HANDLE handle, PVOID data, ULONG length);
typedef BOOLEAN (__stdcall *HidD_GetFeature_)(HANDLE handle, PVOID data, ULONG length);
typedef BOOLEAN (__stdcall *HidD_GetIndexedString_)(HANDLE handle, ULONG string_index, PVOID buffer, ULONG buffer_len);
typedef BOOLEAN (__stdcall *HidD_GetPreparsedData_)(HANDLE handle, PHIDP_PREPARSED_DATA *preparsed_data);
typedef BOOLEAN (__stdcall *HidD_FreePreparsedData_)(PHIDP_PREPARSED_DATA preparsed_data);
typedef NTSTATUS (__stdcall *HidP_GetCaps_)(PHIDP_PREPARSED_DATA preparsed_data, HIDP_CAPS *caps);
typedef BOOLEAN (__stdcall *HidD_SetNumInputBuffers_)(HANDLE handle, ULONG number_buffers);

namespace RN
{
	extern HidD_GetAttributes_ HidD_GetAttributes;
	extern HidD_GetSerialNumberString_ HidD_GetSerialNumberString;
	extern HidD_GetManufacturerString_ HidD_GetManufacturerString;
	extern HidD_GetProductString_ HidD_GetProductString;
	extern HidD_SetFeature_ HidD_SetFeature;
	extern HidD_GetFeature_ HidD_GetFeature;
	extern HidD_GetIndexedString_ HidD_GetIndexedString;
	extern HidD_GetPreparsedData_ HidD_GetPreparsedData;
	extern HidD_FreePreparsedData_ HidD_FreePreparsedData;
	extern HidP_GetCaps_ HidP_GetCaps;
	extern HidD_SetNumInputBuffers_ HidD_SetNumInputBuffers;

	class WindowsHIDDevice : public HIDDevice
	{
	public:
		RNAPI WindowsHIDDevice(const char *devicePath, HIDP_CAPS caps, HIDD_ATTRIBUTES attributes, HANDLE handle);
		RNAPI ~WindowsHIDDevice();

		RNAPI void Open() final;
		RNAPI void Close() final;

		RNAPI const String *GetManufacturerString() const final;
		RNAPI const String *GetProductString() const final;
		RNAPI const String *GetSerialString() const final;

		RNAPI size_t ReadReport(uint8 *data, size_t length, std::chrono::milliseconds timeout) final;
		RNAPI size_t ReadReport(uint8 *data, size_t length) final;

		RNAPI size_t WriteReport(const uint8 *data, size_t length) final;

		RNAPI size_t SendFeatureReport(const uint8 *data, size_t length) final;
		RNAPI size_t GetFeatureReport(uint8 *data, size_t length) const final;

		RNAPI const String *GetIndexedString(size_t index) const final;

		RNAPI size_t GetInputReportLength() const final;
		RNAPI size_t GetOutputReportLength() const final;

		RNAPI uint32 GetVendorID() const final;
		RNAPI uint32 GetProductID() const final;

	private:
		size_t __ReadReport(uint8 *data, size_t length, int32 timeout);

		char _devicePath[256];
		uint32 _vendorID;
		uint32 _productID;

		HANDLE _deviceHandle;

		String *_manufacturerString;
		String *_productString;
		String *_serialString;

		size_t _outputReportLength;
		size_t _inputReportLength;

		bool _readPending;
		uint8 *_readBuffer;

		OVERLAPPED _overlapped;

		__RNDeclareMetaInternal(WindowsHIDDevice)
	};
}

#endif /* __RAYNE_INPUTWINDOWS_H_ */
