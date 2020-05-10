//
//  RNOpenVRWindow.cpp
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <RayneConfig.h>

#ifdef RN_OPENVR_SUPPORTS_METAL
#include "RNOpenVRMetalSwapChain.h"
#endif

#ifdef RN_OPENVR_SUPPORTS_D3D12
#include "RNOpenVRD3D12SwapChain.h"
#include "RND3D12Device.h"
#endif

#ifdef RN_OPENVR_SUPPORTS_VULKAN
#include "RNOpenVRVulkanSwapChain.h"
#include "RNVulkanRendererDescriptor.h"
#include "RNVulkanDevice.h"
#endif

#include "RNOpenVRWindow.h"
#include "../../Source/Math/RNMatrix.h"

namespace RN
{
	RNDefineMeta(OpenVRWindow, VRWindow)

	OpenVRWindow::OpenVRWindow() : _vrSystem(nullptr), _swapChain(nullptr), _currentHapticsIndex{ 0, 0 }, _lastSizeChangeTimer(0.0f)
	{
		_hmdTrackingState.position = Vector3(0.0f, 1.0f, 0.0f);
		
		if(!vr::VR_IsHmdPresent())
		{
			RNDebug("OpenVR: No headset connected.");
			return;
		}
			
		vr::EVRInitError eError = vr::VRInitError_None;
		_vrSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);
		
		if(eError != vr::VRInitError_None)
		{
			_vrSystem = nullptr;
			RNDebug("OpenVR: Unable to init VR runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError));
			return;
		}
		
		RNInfo(GetHMDInfoDescription());

		FileManager *fileManager = FileManager::GetSharedInstance();
		RN::String *inputManifest = fileManager->ResolveFullPath(RNSTR(":RayneOpenVR:/inputmanifest.json"), 0);
		RN_ASSERT(inputManifest, "Missing OpenVR input manifest! Forgot to add OpenVR to the rayne modules in manifest.json?"); //It's needed in the module list to make the :RayneOpenVR: placeholder work
		vr::VRInput()->SetActionManifestPath(inputManifest->GetUTF8String());

		vr::VRInput()->GetActionSetHandle("/actions/main", &_inputActionSetHandle);
		
		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftLower", &_inputActionHandle[InputAction::ButtonLeftLower]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftUpper", &_inputActionHandle[InputAction::ButtonLeftUpper]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftStick", &_inputActionHandle[InputAction::ButtonLeftStick]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftMenu", &_inputActionHandle[InputAction::ButtonLeftMenu]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightLower", &_inputActionHandle[InputAction::ButtonRightLower]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightUpper", &_inputActionHandle[InputAction::ButtonRightUpper]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightStick", &_inputActionHandle[InputAction::ButtonRightStick]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightMenu", &_inputActionHandle[InputAction::ButtonRightMenu]);

		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftTrigger", &_inputActionHandle[InputAction::TriggerLeftTrigger]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftGrab", &_inputActionHandle[InputAction::TriggerLeftGrab]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightTrigger", &_inputActionHandle[InputAction::TriggerRightTrigger]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightGrab", &_inputActionHandle[InputAction::TriggerRightGrab]);

		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftStickPosition", &_inputActionHandle[InputAction::AxisLeftStick]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightStickPosition", &_inputActionHandle[InputAction::AxisRightStick]);

		vr::VRInput()->GetActionSetHandle("/actions/main/out/LeftHandHaptics", &_inputActionHandle[InputAction::HapticsLeftHand]);
		vr::VRInput()->GetActionSetHandle("/actions/main/out/RightHandHaptics", &_inputActionHandle[InputAction::HapticsRightHand]);

		vr::VRInput()->GetActionSetHandle("/actions/main/in/LeftHandPosition", &_inputActionHandle[InputAction::PoseLeftHand]);
		vr::VRInput()->GetActionSetHandle("/actions/main/in/RightHandPosition", &_inputActionHandle[InputAction::PoseRightHand]);
	}

	OpenVRWindow::~OpenVRWindow()
	{
		StopRendering();
		if(_vrSystem) vr::VR_Shutdown();
		_vrSystem = nullptr;
	}
	
	void OpenVRWindow::StartRendering(const SwapChainDescriptor &descriptor)
	{
		if(!_vrSystem)
			return;
		
#ifdef RN_OPENVR_SUPPORTS_METAL
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Metal")))
		{
			_swapChain = new OpenVRMetalSwapChain(descriptor, _vrSystem);
			_swapChainType = SwapChainType::Metal;
			return;
		}
#endif

#ifdef RN_OPENVR_SUPPORTS_D3D12
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("D3D12")))
		{
			_swapChain = new OpenVRD3D12SwapChain(descriptor, _vrSystem);
			_swapChainType = SwapChainType::D3D12;
			return;
		}
#endif

#ifdef RN_OPENVR_SUPPORTS_VULKAN
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Vulkan")))
		{
			_swapChain = new OpenVRVulkanSwapChain(descriptor, _vrSystem);
			_swapChainType = SwapChainType::Vulkan;
			return;
		}
#endif
	}
	
	void OpenVRWindow::StopRendering()
	{
		if(_swapChain)
		{
#ifdef RN_OPENVR_SUPPORTS_METAL
			if(_swapChainType == SwapChainType::Metal)
			{
				OpenVRMetalSwapChain *swapChain = static_cast<OpenVRMetalSwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
				return;
			}
#endif

#ifdef RN_OPENVR_SUPPORTS_D3D12
			if(_swapChainType == SwapChainType::D3D12)
			{
				OpenVRD3D12SwapChain *swapChain = static_cast<OpenVRD3D12SwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
				return;
			}
#endif

#ifdef RN_OPENVR_SUPPORTS_VULKAN
			if(_swapChainType == SwapChainType::Vulkan)
			{
				OpenVRVulkanSwapChain *swapChain = static_cast<OpenVRVulkanSwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
				return;
			}
#endif
		}

		RN_ASSERT(0, "The active renderer is not supported by the OpenVR module!");
	}
	
	bool OpenVRWindow::IsRendering() const
	{
		return (_swapChain != nullptr);
	}
	
	const String *OpenVRWindow::GetHMDInfoDescription() const
	{
		if (!_vrSystem)
			return RNCSTR("No HMD found.");
		
		
		int firstHMDDeviceIndex = 0;
		for(int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
		{
			if(_vrSystem->GetTrackedDeviceClass(nDevice) == vr::TrackedDeviceClass_HMD)
			{
				firstHMDDeviceIndex = nDevice;
				break;
			}
		}
		
		char *propertyString = new char[vr::k_unMaxPropertyStringSize];
		
		_vrSystem->GetStringTrackedDeviceProperty(firstHMDDeviceIndex, vr::Prop_RenderModelName_String, propertyString, vr::k_unMaxPropertyStringSize);
		String *description = new String("Using HMD: ");
		description->Append(propertyString);
		
		_vrSystem->GetStringTrackedDeviceProperty(firstHMDDeviceIndex, vr::Prop_ManufacturerName_String, propertyString, vr::k_unMaxPropertyStringSize);
		description->Append(", Vendor: ");
		description->Append(propertyString);
		
		_vrSystem->GetStringTrackedDeviceProperty(firstHMDDeviceIndex, vr::Prop_TrackingFirmwareVersion_String, propertyString, vr::k_unMaxPropertyStringSize);
		description->Append(", Firmware: ");
		description->Append(propertyString);
		
		delete[] propertyString;
		
		return description;
	}

	Vector2 OpenVRWindow::GetSize() const
	{
		if(!_swapChain)
			return Vector2();
		
		return _swapChain->GetOpenVRSwapChainSize();
	}

	Framebuffer *OpenVRWindow::GetFramebuffer() const
	{
		if(!_swapChain)
			return nullptr;
		
		return _swapChain->GetOpenVRSwapChainFramebuffer();
	}

	uint32 OpenVRWindow::GetEyePadding() const
	{
		return OpenVRSwapChain::kEyePadding;
	}

	static Matrix GetMatrixForOVRMatrix(const vr::HmdMatrix44_t &ovrMatrix)
	{
		Matrix result;
		for(int q = 0; q < 4; q++)
		{
			for(int t = 0; t < 4; t++)
			{
				result.m[q * 4 + t] = ovrMatrix.m[t][q];
			}
		}
		return result;
	}

	static Matrix GetRotationMatrixForOVRMatrix(const vr::HmdMatrix34_t &ovrMatrix)
	{
		Matrix result;
		for(int q = 0; q < 3; q++)
		{
			for(int t = 0; t < 3; t++)
			{
				result.m[q * 4 + t] = ovrMatrix.m[t][q];
			}
		}
		return result;
	}

	void OpenVRWindow::Update(float delta, float near, float far)
	{
		if(!_swapChain)
			return;
		
		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_vrSystem->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		Vector2 newSize(recommendedWidth * 2 + GetEyePadding(), recommendedHeight);

		if(newSize.GetSquaredDistance(_lastSize) > 0.001f)
		{
			_lastSizeChangeTimer = 0.0f;
		}
		_lastSize = newSize;

		if(_lastSizeChangeTimer > 0.5f && newSize.GetSquaredDistance(_swapChain->GetOpenVRSwapChainSize()) > 0.001f)
		{
			UpdateSize(newSize);
		}

		_lastSizeChangeTimer += delta;

		uint16 trackedDevices[3] = { vr::k_unMaxTrackedDeviceCount, vr::k_unMaxTrackedDeviceCount, vr::k_unMaxTrackedDeviceCount };
		_swapChain->UpdatePredictedPose();

		vr::HmdMatrix44_t leftProjection = _vrSystem->GetProjectionMatrix(vr::Eye_Left, near, far);
		vr::HmdMatrix44_t rightProjection = _vrSystem->GetProjectionMatrix(vr::Eye_Right, near, far);

		_hmdTrackingState.eyeOffset[0] = _swapChain->_hmdToEyeViewOffset[0];
		_hmdTrackingState.eyeOffset[1] = _swapChain->_hmdToEyeViewOffset[1];
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(leftProjection);
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(rightProjection);

		if(_swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			vr::HmdMatrix34_t headPose = _swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
			Matrix rotationPose = GetRotationMatrixForOVRMatrix(headPose);
			_hmdTrackingState.position.x = headPose.m[0][3];
			_hmdTrackingState.position.y = headPose.m[1][3];
			_hmdTrackingState.position.z = headPose.m[2][3];
			_hmdTrackingState.rotation = rotationPose.GetEulerAngle();
		}

		_hmdTrackingState.mode = vr::VROverlay()->IsDashboardVisible() ? VRHMDTrackingState::Mode::Paused : VRHMDTrackingState::Mode::Rendering;
		vr::VREvent_t event;
		while(_vrSystem->PollNextEvent(&event, sizeof(event)))
		{
			//TODO: Handle more OpenVR events
			switch(event.eventType)
			{
			case vr::VREvent_Quit:
				_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
				break;
			}
		}

		//Update input state
		vr::VRActiveActionSet_t actionSet;
		actionSet.ulActionSet = _inputActionSetHandle;
		actionSet.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
		actionSet.ulSecondaryActionSet = vr::k_ulInvalidActionSetHandle;
		vr::VRInput()->UpdateActionState(&actionSet, sizeof(vr::VRActiveActionSet_t), 1);

		vr::InputPoseActionData_t handPose[2];
		vr::VRInput()->GetPoseActionDataForNextFrame(_inputActionHandle[InputAction::PoseLeftHand], vr::ETrackingUniverseOrigin::TrackingUniverseStanding, &handPose[0], sizeof(vr::InputPoseActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetPoseActionDataForNextFrame(_inputActionHandle[InputAction::PoseRightHand], vr::ETrackingUniverseOrigin::TrackingUniverseStanding, &handPose[1], sizeof(vr::InputPoseActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputAnalogActionData_t triggerTrigger[2];
		vr::VRInput()->GetAnalogActionData(_inputActionHandle[InputAction::TriggerLeftTrigger], &triggerTrigger[0], sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetAnalogActionData(_inputActionHandle[InputAction::TriggerRightTrigger], &triggerTrigger[1], sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputAnalogActionData_t grabTrigger[2];
		vr::VRInput()->GetAnalogActionData(_inputActionHandle[InputAction::TriggerLeftGrab], &grabTrigger[0], sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetAnalogActionData(_inputActionHandle[InputAction::TriggerRightGrab], &grabTrigger[1], sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputAnalogActionData_t stickPosition[2];
		vr::VRInput()->GetAnalogActionData(_inputActionHandle[InputAction::AxisLeftStick], &stickPosition[0], sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetAnalogActionData(_inputActionHandle[InputAction::AxisRightStick], &stickPosition[1], sizeof(vr::InputAnalogActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputDigitalActionData_t lowerButton[2];
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonLeftLower], &lowerButton[0], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonRightLower], &lowerButton[1], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputDigitalActionData_t upperButton[2];
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonLeftUpper], &upperButton[0], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonRightUpper], &upperButton[1], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputDigitalActionData_t stickButton[2];
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonLeftStick], &stickButton[0], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonRightStick], &stickButton[1], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);

		vr::InputDigitalActionData_t menuButton[2];
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonLeftMenu], &menuButton[0], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);
		vr::VRInput()->GetDigitalActionData(_inputActionHandle[InputAction::ButtonRightMenu], &menuButton[1], sizeof(vr::InputDigitalActionData_t), vr::k_ulInvalidActionHandle);

		for(int i = 0; i < 2; i++)
		{
			_controllerTrackingState[i].active = false;;
			_controllerTrackingState[i].tracking = false;
			if(handPose[i].bActive && handPose[i].pose.bDeviceIsConnected)
			{
				_controllerTrackingState[i].active = true;
				_controllerTrackingState[i].tracking = handPose[i].pose.bPoseIsValid;

				vr::HmdMatrix34_t handMatrix = handPose[i].pose.mDeviceToAbsoluteTracking;
				Matrix rotationPose = GetRotationMatrixForOVRMatrix(handMatrix);
				_controllerTrackingState[i].rotation = rotationPose.GetEulerAngle();
				_controllerTrackingState[i].position.x = handMatrix.m[0][3];
				_controllerTrackingState[i].position.y = handMatrix.m[1][3];
				_controllerTrackingState[i].position.z = handMatrix.m[2][3];
			}

			if(triggerTrigger[i].bActive)
			{
				_controllerTrackingState[i].indexTrigger = triggerTrigger[i].x;
			}
			else
			{
				_controllerTrackingState[i].indexTrigger = 0.0f;
			}

			if(grabTrigger[i].bActive)
			{
				_controllerTrackingState[i].handTrigger = grabTrigger[i].x;
			}
			else
			{
				_controllerTrackingState[i].handTrigger = 0.0f;
			}

			if(stickPosition[i].bActive)
			{
				_controllerTrackingState[i].thumbstick.x = stickPosition[i].x;
				_controllerTrackingState[i].thumbstick.y = stickPosition[i].y;
			}
			else
			{
				_controllerTrackingState[i].thumbstick = RN::Vector2();
			}

			_controllerTrackingState[i].button[VRControllerTrackingState::Button::AX] = lowerButton[i].bActive && lowerButton[i].bState;
			_controllerTrackingState[i].button[VRControllerTrackingState::Button::BY] = upperButton[i].bActive && upperButton[i].bState;
			_controllerTrackingState[i].button[VRControllerTrackingState::Button::Stick] = stickButton[i].bActive && stickButton[i].bState;
			_controllerTrackingState[i].button[VRControllerTrackingState::Button::Start] = menuButton[i].bActive && menuButton[i].bState;

			if(_currentHapticsIndex[i] < _haptics[i].sampleCount)
			{
				float strength = _haptics[i].samples[_currentHapticsIndex[i]++];
				vr::VRInput()->TriggerHapticVibrationAction(_inputActionHandle[i == 0 ? InputAction::HapticsLeftHand : InputAction::HapticsRightHand], 0.0f, delta*2.0f, 320.0f, strength, vr::k_ulInvalidActionHandle);
			}
			else
			{
				vr::VRInput()->TriggerHapticVibrationAction(_inputActionHandle[i == 0 ? InputAction::HapticsLeftHand : InputAction::HapticsRightHand], 0.0f, delta*2.0f, 320.0f, 0.0f, vr::k_ulInvalidActionHandle);
			}
		}

		//TODO: Add tracker support
		_trackerTrackingState.active = false;
		_trackerTrackingState.tracking = false;
	}

	void OpenVRWindow::UpdateSize(const Vector2 &size)
	{
		if(!_swapChain)
			return;
		
		_swapChain->ResizeOpenVRSwapChain(size);
		NotificationManager::GetSharedInstance()->PostNotification(kRNWindowDidChangeSize, this);
	}

	const VRHMDTrackingState &OpenVRWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const VRControllerTrackingState &OpenVRWindow::GetControllerTrackingState(uint8 index) const
	{
		return _controllerTrackingState[index];
	}

	const VRControllerTrackingState &OpenVRWindow::GetTrackerTrackingState(uint8 index) const
	{
		return _trackerTrackingState;
	}

	void OpenVRWindow::SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics)
	{
		_currentHapticsIndex[index] = 0;
		_haptics[index] = haptics;
	}
	
	void OpenVRWindow::PreparePreviewWindow(Window *window) const
	{
#if RN_PLATFORM_MAC_OS
		window->Downcast<MetalWindow>()->GetSwapChain()->SetFrameDivider(1);
#elif RN_PLATFORM_WINDOWS

#elif RN_PLATFORM_LINUX

#endif
	}
	
	RenderingDevice *OpenVRWindow::GetOutputDevice(RendererDescriptor *descriptor) const
	{
		if(!_vrSystem)
			return nullptr;

#ifdef RN_OPENVR_SUPPORTS_METAL
		if(descriptor->GetAPI()->IsEqual(RNCSTR("Metal")))
		{
			id<MTLDevice> mtlDevice = nil;
			_vrSystem->GetOutputDevice((uint64_t*)&mtlDevice, vr::TextureType_IOSurface);
			MetalDevice *device = nullptr;
			if(mtlDevice)
			{
				device = new MetalDevice(mtlDevice);
			}
			return device;
		}
#endif

#ifdef RN_OPENVR_SUPPORTS_D3D12
		if(descriptor->GetAPI()->IsEqual(RNCSTR("D3D12")))
		{
			LUID adapterLUID;
			_vrSystem->GetOutputDevice(reinterpret_cast<uint64_t*>(&adapterLUID), vr::TextureType_DirectX12);

			D3D12Device *outputDevice = nullptr;
			descriptor->GetDevices()->Enumerate<D3D12Device>([&](D3D12Device *device, size_t index, bool &stop){
				DXGI_ADAPTER_DESC adapterDescription;
				device->GetAdapter()->GetDesc(&adapterDescription);
				if(adapterDescription.AdapterLuid.LowPart == adapterLUID.LowPart && adapterDescription.AdapterLuid.HighPart == adapterLUID.HighPart)
				{
					outputDevice = device;
					stop = true;
				}
			});
			
			return outputDevice;
		}
#endif

#ifdef RN_OPENVR_SUPPORTS_VULKAN
		if(descriptor->GetAPI()->IsEqual(RNCSTR("Vulkan")))
		{
			VulkanRendererDescriptor *vulkanDescriptor = descriptor->Downcast<VulkanRendererDescriptor>();
			
			VkInstance instance = vulkanDescriptor->GetInstance()->GetInstance();
			VkPhysicalDevice physicalDevice;
			_vrSystem->GetOutputDevice(reinterpret_cast<uint64_t*>(&physicalDevice), vr::TextureType_Vulkan, instance);

			VulkanDevice *outputDevice = nullptr;
			descriptor->GetDevices()->Enumerate<VulkanDevice>([&](VulkanDevice *device, size_t index, bool &stop) {
				if(device->GetPhysicalDevice() == physicalDevice)
				{
					outputDevice = device;
					stop = true;
				}
			});

			return outputDevice;
		}
#endif

		return nullptr;
	}
	
	Mesh *OpenVRWindow::GetHiddenAreaMesh(uint8 eye) const
	{
		if(!_vrSystem)
			return nullptr;
		
		vr::HiddenAreaMesh_t hiddenAreaMesh = _vrSystem->GetHiddenAreaMesh(static_cast<vr::Hmd_Eye>(eye));
		
		if(hiddenAreaMesh.unTriangleCount <= 0)
			return nullptr;
		
		Mesh *mesh = new Mesh({Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2)}, hiddenAreaMesh.unTriangleCount * 3, 0);
		
		mesh->BeginChanges();
		mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, hiddenAreaMesh.pVertexData);
		mesh->EndChanges();
		
		return mesh->Autorelease();
	}
	
	const Window::SwapChainDescriptor &OpenVRWindow::GetSwapChainDescriptor() const
	{
		return _swapChain->GetOpenVRSwapChainDescriptor();
	}

	VRWindow::Availability OpenVRWindow::GetAvailability()
	{
		VRWindow::Availability availability = Availability::None;
		if(!vr::VR_IsRuntimeInstalled())
		{
			return availability;
		}
		availability = Availability::Software;

		if(!vr::VR_IsHmdPresent())
		{
			return availability;
		}
		availability = Availability::HMD;

		return availability;
	}

	bool OpenVRWindow::IsSteamVRRunning()
	{
#if RN_PLATFORM_WINDOWS
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;

		if(!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		{
			return false;
		}


		// Calculate how many process identifiers were returned.
		cProcesses = cbNeeded / sizeof(DWORD);

		for(i = 0; i < cProcesses; i++)
		{
			if(aProcesses[i] != 0)
			{
				DWORD processID = aProcesses[i];
				TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

				// Get a handle to the process.
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

				// Get the process name.
				if(NULL != hProcess)
				{
					HMODULE hMod;
					DWORD cbNeeded;

					if(EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					{
						GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
					}
				}

				if(RNSTR(szProcessName)->IsEqual(RNSTR("vrmonitor.exe")))
				{
					return true;
				}

				// Release the handle to the process.
				CloseHandle(hProcess);
			}
		}
#endif
		return false;
	}

#ifdef RN_OPENVR_SUPPORTS_VULKAN
	OVRAPI Array *OpenVRWindow::GetRequiredVulkanInstanceExtensions() const
	{
		uint32_t buffersize;
		buffersize = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(nullptr, 0);
		char *extensions = (char*)malloc(buffersize);
		vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(extensions, buffersize);

		String *result = RNSTR(extensions);
		free(extensions);

		Array *extensionArray = result->GetComponentsSeparatedByString(RNCSTR(" "));
		return extensionArray;
	}

	OVRAPI Array *OpenVRWindow::GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *renderingDevice) const
	{
		if(!descriptor->GetAPI()->IsEqual(RNCSTR("Vulkan")))
		{
			return nullptr;
		}

		VulkanDevice *vulkanDevice = renderingDevice->Downcast<VulkanDevice>();

		uint32_t buffersize;
		buffersize = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(vulkanDevice->GetPhysicalDevice(), nullptr, 0);
		char *extensions = (char*)malloc(buffersize);
		vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(vulkanDevice->GetPhysicalDevice(), extensions, buffersize);

		String *result = RNSTR(extensions);
		free(extensions);

		Array *extensionArray = result->GetComponentsSeparatedByString(RNCSTR(" "));
		return extensionArray;
	}
#endif
}
