//
//  RNInputWindows.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Debug/RNLogger.h"
#include "RNInputWindows.h"

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

/* Since we're not building with the DDK, and the HID header
   files aren't part of the SDK, we have to define all this
   stuff here. In lookup_functions(), the function pointers
   defined below are set. */
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
	static HidD_GetAttributes_ HidD_GetAttributes;
	static HidD_GetSerialNumberString_ HidD_GetSerialNumberString;
	static HidD_GetManufacturerString_ HidD_GetManufacturerString;
	static HidD_GetProductString_ HidD_GetProductString;
	static HidD_SetFeature_ HidD_SetFeature;
	static HidD_GetFeature_ HidD_GetFeature;
	static HidD_GetIndexedString_ HidD_GetIndexedString;
	static HidD_GetPreparsedData_ HidD_GetPreparsedData;
	static HidD_FreePreparsedData_ HidD_FreePreparsedData;
	static HidP_GetCaps_ HidP_GetCaps;
	static HidD_SetNumInputBuffers_ HidD_SetNumInputBuffers;

	static HMODULE _hidModule;

	HANDLE OpenDevice(const char *devicePath, bool enumerate)
	{
		DWORD access = enumerate ? 0 : GENERIC_WRITE | GENERIC_READ;
		return ::CreateFileA(devicePath, access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	}

	void EnumerateDevices()
	{
		GUID interfaceClassGUID = {0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30} };
		HDEVINFO deviceInfo = ::SetupDiGetClassDevsA(&interfaceClassGUID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

		DWORD index = 0;

		while(1)
		{
			// Use the driver setup interface to enumerate through all devices
			SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

			BOOL result = ::SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &interfaceClassGUID, index, &deviceInterfaceData);

			if(!result)
				break;


			DWORD requiredSize = 0;
			::SetupDiGetDeviceInterfaceDetailA(deviceInfo, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);

			SP_DEVICE_INTERFACE_DETAIL_DATA_A *deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)malloc(requiredSize);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

			ScopeGuard dataGuard([&] {

				index ++;
				free(deviceInterfaceDetailData);

			});

			result = ::SetupDiGetDeviceInterfaceDetailA(deviceInfo, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, nullptr, nullptr);

			if(!result)
				continue;

			// Make sure its a HID device
			bool isHIDDevice = false;

			for(DWORD i = 0; ; i ++)
			{
				SP_DEVINFO_DATA devinfoData = { 0 };
				devinfoData.cbSize = sizeof(devinfoData);

				result = ::SetupDiEnumDeviceInfo(deviceInfo, i, &devinfoData);
				if(!result)
					break;

				char driverName[256];
				result = ::SetupDiGetDeviceRegistryPropertyA(deviceInfo, &devinfoData, SPDRP_CLASS, nullptr, reinterpret_cast<PBYTE>(driverName), sizeof(driverName), nullptr);

				if(!result)
					break;

				if(strcmp(driverName, "HIDClass") == 0)
				{
					result = ::SetupDiGetDeviceRegistryPropertyA(deviceInfo, &devinfoData, SPDRP_DRIVER, nullptr, reinterpret_cast<PBYTE>(driverName), sizeof(driverName), nullptr);
					if(result)
					{
						isHIDDevice = true;
						break;
					}
				}
			}

			if(!isHIDDevice)
				continue;


			HANDLE writeHandle = OpenDevice(deviceInterfaceDetailData->DevicePath, true);
			if(writeHandle == INVALID_HANDLE_VALUE)
				continue;

			ScopeGuard handleGuard([&] {

				::CloseHandle(writeHandle);

			});


			HIDD_ATTRIBUTES attributes;
			attributes.Size = sizeof(HIDD_ATTRIBUTES);

			HidD_GetAttributes(writeHandle, &attributes);

			RNDebug("Found HID Device " << attributes.VendorID << ", " << attributes.ProductID);
		}

		::SetupDiDestroyDeviceInfoList(deviceInfo);
	}

	void BuildPlatformDeviceTree()
	{
		_hidModule = ::LoadLibraryA("hid.dll");
		RN_ASSERT(_hidModule, "hid.dll needs to be loadable");

#define RESOLVE_HID(x) do { x = (x##_)GetProcAddress(_hidModule, #x); RN_ASSERT(x, "Could not find %s", #x); } while(0)

		RESOLVE_HID(HidD_GetAttributes);
		RESOLVE_HID(HidD_GetSerialNumberString);
		RESOLVE_HID(HidD_GetManufacturerString);
		RESOLVE_HID(HidD_GetProductString);
		RESOLVE_HID(HidD_SetFeature);
		RESOLVE_HID(HidD_GetFeature);
		RESOLVE_HID(HidD_GetIndexedString);
		RESOLVE_HID(HidD_GetPreparsedData);
		RESOLVE_HID(HidD_FreePreparsedData);
		RESOLVE_HID(HidP_GetCaps);
		RESOLVE_HID(HidD_SetNumInputBuffers);

#undef RESOLVE_HID

		EnumerateDevices();
	}

	void TearDownPlatformDeviceTree()
	{
		::FreeLibrary(_hidModule);
	}
}
