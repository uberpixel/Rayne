//
//  RNAppleXRWindow.cpp
//  Rayne-AppleXR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <RayneConfig.h>

#ifdef RN_APPLEXR_SUPPORTS_METAL
#include "RNAppleXRMetalSwapChain.h"
#endif

#include "RNAppleXRWindow.h"
#include "../../Source/Math/RNMatrix.h"

namespace RN
{
	RNDefineMeta(AppleXRWindow, VRWindow)

	AppleXRWindow::AppleXRWindow() : _layerRenderer(nullptr), _swapChain(nullptr), _currentHapticsIndex{ 0, 0 }, _isSessionRunning(false)
	{
		_hmdTrackingState.position = Vector3(0.0f, 1.0f, 0.0f);
		_layerRenderer = static_cast<cp_layer_renderer_t>(Kernel::GetSharedInstance()->GetLayerRenderer());
		
		ar_world_tracking_configuration_t worldTrackingConfiguration = ar_world_tracking_configuration_create();
		_worldTrackingProvider = ar_world_tracking_provider_create(worldTrackingConfiguration);

		ar_data_providers_t dataProviders = ar_data_providers_create_with_data_providers(_worldTrackingProvider, nil);

		_arSession = ar_session_create();
		ar_session_run(_arSession, dataProviders);
		
		RNInfo(GetHMDInfoDescription());
	}

	AppleXRWindow::~AppleXRWindow()
	{
		ar_session_stop(_arSession);
		StopRendering();
		_layerRenderer = nullptr;
	}
	
	void AppleXRWindow::StartRendering(const SwapChainDescriptor &descriptor, float eyeResolutionFactor)
	{
		if(!_layerRenderer)
			return;
		
#ifdef RN_APPLEXR_SUPPORTS_METAL
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Metal")))
		{
			_swapChain = new AppleXRMetalSwapChain(descriptor, _layerRenderer);
			_swapChainType = SwapChainType::Metal;
			return;
		}
#endif
	}
	
	void AppleXRWindow::StopRendering()
	{
		if(_swapChain)
		{
#ifdef RN_APPLEXR_SUPPORTS_METAL
			if(_swapChainType == SwapChainType::Metal)
			{
				AppleXRMetalSwapChain *swapChain = static_cast<AppleXRMetalSwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
				return;
			}
#endif
		}

		RN_ASSERT(0, "The active renderer is not supported by the AppleXR module!");
	}
	
	bool AppleXRWindow::IsRendering() const
	{
		return (_swapChain != nullptr);
	}
	
	const String *AppleXRWindow::GetHMDInfoDescription() const
	{
		if(!_layerRenderer)
			return RNCSTR("No HMD found.");

		String *description = new String("Using HMD: Apple Vision Pro");
		
		return description;
	}

	Vector2 AppleXRWindow::GetSize() const
	{
		if(!_swapChain)
			return Vector2();
		
		return _swapChain->GetAppleXRSwapChainSize();
	}

	Framebuffer *AppleXRWindow::GetFramebuffer() const
	{
		if(!_swapChain)
			return nullptr;
		
		return _swapChain->GetAppleXRSwapChainFramebuffer();
	}

	void AppleXRWindow::BeginFrame(float delta)
	{
		if(!_swapChain) return;
		
		switch(cp_layer_renderer_get_state(_layerRenderer))
		{
			case cp_layer_renderer_state_paused:
				_isSessionRunning = false;
				_swapChain->isActive = false;
				cp_layer_renderer_wait_until_running(_layerRenderer);
				break;
			case cp_layer_renderer_state_running:
				_isSessionRunning = true;
				_swapChain->isActive = true;
				break;
			case cp_layer_renderer_state_invalidated:
				_isSessionRunning = false;
				_swapChain->isActive = false;
				break;
		}
		
		if(!_isSessionRunning) return;
		
		_swapChain->_frame = cp_layer_renderer_query_next_frame(_layerRenderer);
		if(_swapChain->_frame == nullptr) return;
		
		_swapChain->_predictedTime = cp_frame_predict_timing(_swapChain->_frame);
		if(_swapChain->_predictedTime == nullptr) return;

		cp_frame_start_update(_swapChain->_frame);
		
		_swapChain->_worldAnchor = ar_device_anchor_create();

		// Fetch the device anchor from ARKit.
		CFTimeInterval p_time = cp_time_to_cf_time_interval(cp_frame_timing_get_presentation_time(_swapChain->_predictedTime));
		ar_device_anchor_query_status_t anchor_status = ar_world_tracking_provider_query_device_anchor_at_timestamp(_worldTrackingProvider, p_time, _swapChain->_worldAnchor);
		if(anchor_status != ar_device_anchor_query_status_success) return;
		
/*		simd_float4x4 head_position = ar_anchor_get_origin_from_anchor_transform(_swapChain->_worldAnchor);


		cp_view_t view = cp_drawable_get_view(drawable, index);
		simd_float4 tangents = cp_view_get_tangents(view);
		simd_float2 depth_range = cp_drawable_get_depth_range(drawable);
		simd_float4x4 transform = makeProjectiveTransformFromTangents(tangents[0], /* left */
//																	  tangents[1], /* right */
//																	  tangents[2], /* top */
//																	  tangents[3], /* bottom */
//																	  depth_range[1], /* nearZ */
//																	  depth_range[0], /* farZ */
//																	  true); /* reverseZ */
/*		uniforms[index].projectionMatrix = transform;


		// Adjust the camera transform for the current eye position.
		simd_float4x4 camera_transform = simd_mul(head_position, cp_view_get_transform(view));
		uniforms[index].viewMatrix = simd_inverse(camera_transform);*/
	}

	void AppleXRWindow::Update(float delta, float near, float far)
	{
		if(!_swapChain)
			return;

		_swapChain->UpdatePredictedPose();

		//_hmdTrackingState.eyeOffset[0] = _swapChain->_hmdToEyeViewOffset[0];
		//_hmdTrackingState.eyeOffset[1] = _swapChain->_hmdToEyeViewOffset[1];
		//_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(leftProjection);
		//_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(rightProjection);

		/*if(_swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			vr::HmdMatrix34_t headPose = _swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
			Matrix rotationPose = GetRotationMatrixForOVRMatrix(headPose);
			_hmdTrackingState.position.x = headPose.m[0][3];
			_hmdTrackingState.position.y = headPose.m[1][3];
			_hmdTrackingState.position.z = headPose.m[2][3];
			_hmdTrackingState.rotation = rotationPose.GetEulerAngle();
		}*/

		/*_hmdTrackingState.mode = vr::VROverlay()->IsDashboardVisible() ? VRHMDTrackingState::Mode::Paused : VRHMDTrackingState::Mode::Rendering;
		vr::VREvent_t event;
		while(_vrSystem->PollNextEvent(&event, sizeof(event)))
		{
			//TODO: Handle more AppleXR events
			switch(event.eventType)
			{
			case vr::VREvent_Quit:
				_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
				break;
			}
		}*/

		//TODO: Add tracker support
		_trackerTrackingState.active = false;
		_trackerTrackingState.tracking = false;
	}

	const VRHMDTrackingState &AppleXRWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const VRControllerTrackingState &AppleXRWindow::GetControllerTrackingState(uint8 index) const
	{
		return _controllerTrackingState[index];
	}

	const VRControllerTrackingState &AppleXRWindow::GetTrackerTrackingState(uint8 index) const
	{
		return _trackerTrackingState;
	}

	const VRHandTrackingState &AppleXRWindow::GetHandTrackingState(uint8 index) const
	{
		return _handTrackingState[index];
	}

	void AppleXRWindow::SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics)
	{
		_currentHapticsIndex[index] = 0;
		_haptics[index] = haptics;
	}
	
	RenderingDevice *AppleXRWindow::GetOutputDevice(RendererDescriptor *descriptor) const
	{
		if(!_layerRenderer)
			return nullptr;

#ifdef RN_APPLEXR_SUPPORTS_METAL
		if(descriptor->GetAPI()->IsEqual(RNCSTR("Metal")))
		{
			id<MTLDevice> mtlDevice = cp_layer_renderer_get_device(_layerRenderer);
			MetalDevice *device = nullptr;
			if(mtlDevice)
			{
				device = new MetalDevice(mtlDevice);
			}
			return device;
		}
#endif

		return nullptr;
	}
	
	const Window::SwapChainDescriptor &AppleXRWindow::GetSwapChainDescriptor() const
	{
		return _swapChain->GetAppleXRSwapChainDescriptor();
	}
}
