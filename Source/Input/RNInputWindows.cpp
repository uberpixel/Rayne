//
//  RNInputWindows.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Debug/RNLogger.h"
#include "RNInputWindows.h"

namespace RN
{
	HidD_GetAttributes_ HidD_GetAttributes;
	HidD_GetSerialNumberString_ HidD_GetSerialNumberString;
	HidD_GetManufacturerString_ HidD_GetManufacturerString;
	HidD_GetProductString_ HidD_GetProductString;
	HidD_SetFeature_ HidD_SetFeature;
	HidD_GetFeature_ HidD_GetFeature;
	HidD_GetIndexedString_ HidD_GetIndexedString;
	HidD_GetPreparsedData_ HidD_GetPreparsedData;
	HidD_FreePreparsedData_ HidD_FreePreparsedData;
	HidP_GetCaps_ HidP_GetCaps;
	HidD_SetNumInputBuffers_ HidD_SetNumInputBuffers;

	static HMODULE _hidModule;

	HANDLE OpenDevice(const char *devicePath, bool enumerate)
	{
		DWORD access = enumerate ? 0 : GENERIC_WRITE | GENERIC_READ;
		return ::CreateFileA(devicePath, access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	}

	String *CreateWcharString(wchar_t *input)
	{
		size_t length = wcslen(input);
		uint8 *buffer = new uint8[length * 4];

		int result = ::WideCharToMultiByte(CP_UTF8, 0, input, static_cast<int>(length), reinterpret_cast<LPSTR>(buffer), static_cast<int>(length * 4), nullptr, nullptr);
		String *string = new String(buffer, result, Encoding::UTF8);

		delete[] buffer;

		return string;
	}

	RNDefineMeta(WindowsHIDDevice, HIDDevice)

	WindowsHIDDevice::WindowsHIDDevice(const char *devicePath, HIDP_CAPS caps, HIDD_ATTRIBUTES attributes, HANDLE handle) :
		HIDDevice(static_cast<HIDUsagePage>(caps.UsagePage), caps.Usage),
		_deviceHandle(INVALID_HANDLE_VALUE),
		_manufacturerString(nullptr),
		_productString(nullptr),
		_serialString(nullptr),
		_readPending(false),
		_readBuffer(nullptr),
		_outputReportLength(caps.OutputReportByteLength),
		_inputReportLength(caps.InputReportByteLength),
		_vendorID(attributes.VendorID),
		_productID(attributes.ProductID)
	{
		strcpy(_devicePath, devicePath);

		wchar_t string[512];

		BOOL result = HidD_GetSerialNumberString(handle, string, 512);
		if(result)
		{
			string[511] = 0;
			_serialString = CreateWcharString(string);
		}

		result = HidD_GetManufacturerString(handle, string, 512);
		if(result)
		{
			string[511] = 0;
			_manufacturerString = CreateWcharString(string);
		}

		result = HidD_GetProductString(handle, string, 512);
		if(result)
		{
			string[511] = 0;
			_productString = CreateWcharString(string);
		}
	}

	WindowsHIDDevice::~WindowsHIDDevice()
	{
		Close();
		SafeRelease(_productString);
		SafeRelease(_manufacturerString);
		SafeRelease(_serialString);
	}

	void WindowsHIDDevice::Open()
	{
		memset(&_overlapped, 0, sizeof(OVERLAPPED));

		_overlapped.hEvent = ::CreateEvent(nullptr, false, false, nullptr);
		_deviceHandle = OpenDevice(_devicePath, false);

		HidD_SetNumInputBuffers(_deviceHandle, 64);

		_readBuffer = new uint8[_inputReportLength];
	}

	void WindowsHIDDevice::Close()
	{
		if(_deviceHandle != INVALID_HANDLE_VALUE)
		{
			delete[] _readBuffer;
			::CancelIo(_deviceHandle);
			::CloseHandle(_overlapped.hEvent);
			::CloseHandle(_deviceHandle);

			_deviceHandle = INVALID_HANDLE_VALUE;
		}
	}

	const String *WindowsHIDDevice::GetManufacturerString() const
	{
		return _manufacturerString;
	}
	const String *WindowsHIDDevice::GetProductString() const
	{
		return _productString;
	}
	const String *WindowsHIDDevice::GetSerialString() const
	{
		return _serialString;
	}

	size_t WindowsHIDDevice::ReadReport(uint8 *data, size_t length, std::chrono::milliseconds timeout)
	{
		return __ReadReport(data, length, static_cast<int32>(timeout.count()));
	}
	size_t WindowsHIDDevice::ReadReport(uint8 *data, size_t length)
	{
		return __ReadReport(data, length, -1);
	}
	size_t WindowsHIDDevice::__ReadReport(uint8 *data, size_t length, int32 timeout)
	{
		HANDLE event = _overlapped.hEvent;
		DWORD bytesRead;

		if(!_readPending)
		{
			_readPending = true;

			std::fill(_readBuffer, _readBuffer + _inputReportLength, 0);
			::ResetEvent(event);

			BOOL result = ::ReadFile(_deviceHandle, _readBuffer, static_cast<DWORD>(_inputReportLength), &bytesRead, &_overlapped);

			if(!result)
			{
				if(::GetLastError() != ERROR_IO_PENDING)
				{
					::CancelIo(_deviceHandle);
					_readPending = false;

					return 0;
				}
			}
		}

		if(timeout > 0)
		{
			DWORD result = ::WaitForSingleObject(event, static_cast<DWORD>(timeout));
			if(result != WAIT_OBJECT_0)
				return 0;
		}

		BOOL result = ::GetOverlappedResult(_deviceHandle, &_overlapped, &bytesRead, true);

		_readPending = false;

		if(result && bytesRead > 0)
		{
			size_t copyLength;

			if(_readBuffer[0] == 0x0)
			{
				bytesRead --;
				copyLength = length > bytesRead ? bytesRead : length;
				std::copy(_readBuffer + 1, _readBuffer + copyLength, data);
			}
			else
			{
				copyLength = length > bytesRead ? bytesRead : length;
				std::copy(_readBuffer, _readBuffer + copyLength, data);
			}

			return bytesRead;
		}

		return 0;
	}

	size_t WindowsHIDDevice::WriteReport(const uint8 *data, size_t length)
	{
		uint8 *buffer;

		if(length >= _outputReportLength)
		{
			buffer = const_cast<uint8 *>(data);
		}
		else
		{
			buffer = new uint8[_outputReportLength];
			std::copy(data, data + length, buffer);
			std::fill(buffer + length, buffer + _outputReportLength, 0);

			length = _outputReportLength;
		}

		ScopeGuard guard([&] {

			if(buffer != data)
				delete[] buffer;

		});


		OVERLAPPED overlapped = { 0 };

		BOOL result = ::WriteFile(_deviceHandle, buffer, static_cast<DWORD>(length), nullptr, &overlapped);

		if(!result)
		{
			if(::GetLastError() != ERROR_IO_PENDING)
			{
				throw "Fuck";
			}
		}

		DWORD bytesWritten;
		result = ::GetOverlappedResult(_deviceHandle, &overlapped, &bytesWritten, true);

		if(!result)
		{
			throw "Fuck";
		}

		return bytesWritten;
	}

	size_t WindowsHIDDevice::SendFeatureReport(const uint8 *data, size_t length)
	{
		BOOL result = HidD_SetFeature(_deviceHandle, reinterpret_cast<PVOID>(const_cast<uint8 *>(data)), static_cast<ULONG>(length));
		if(!result)
			return 0;

		return length;
	}
	size_t WindowsHIDDevice::GetFeatureReport(uint8 *data, size_t length) const
	{

		DWORD bytesReturned;
		OVERLAPPED overlapped = { 0 };

		BOOL result = ::DeviceIoControl(_deviceHandle, IOCTL_HID_GET_FEATURE, data, static_cast<DWORD>(length), data, static_cast<DWORD>(length), &bytesReturned, &overlapped);
		if(!result)
		{
			throw "Issue";
		}

		result = ::GetOverlappedResult(_deviceHandle, &overlapped, &bytesReturned, true);
		if(!result)
		{
			throw "Issue";
		}

		return bytesReturned + 1;
	}

	const String *WindowsHIDDevice::GetIndexedString(size_t index) const
	{
		wchar_t buffer[1024];
		BOOL result = HidD_GetIndexedString(_deviceHandle, static_cast<ULONG>(index), buffer, 1024);

		if(!result)
			return nullptr;

		buffer[1023] = 0;
		return CreateWcharString(buffer)->Autorelease();
	}

	size_t WindowsHIDDevice::GetInputReportLength() const
	{
		return _inputReportLength;
	}
	size_t WindowsHIDDevice::GetOutputReportLength() const
	{
		return _outputReportLength;
	}


	uint32 WindowsHIDDevice::GetVendorID() const
	{
		return _vendorID;
	}
	uint32 WindowsHIDDevice::GetProductID() const
	{
		return _productID;
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

			PHIDP_PREPARSED_DATA parsedData;

			result = HidD_GetPreparsedData(writeHandle, &parsedData);
			if(!result)
				continue;

			HIDP_CAPS caps;
			NTSTATUS status = HidP_GetCaps(parsedData, &caps);
			if(status != HIDP_STATUS_SUCCESS)
			{
				HidD_FreePreparsedData(parsedData);
				continue;
			}

			WindowsHIDDevice *device = new WindowsHIDDevice(deviceInterfaceDetailData->DevicePath, caps, attributes, writeHandle);
			if(!device->IsVendor())
				RNDebug(device);


			HidD_FreePreparsedData(parsedData);
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
