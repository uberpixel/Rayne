//
//  RNOculusMobileVulkanSwapChain.cpp
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusMobileVulkanSwapChain.h"
#include "RNVulkanInternals.h"

#include <unistd.h>
#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/window.h>				// for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h>	// for native window JNI
#include <android_native_app_glue.h>

#include "VrApi_Vulkan.h"
#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"
#include "VrApi_Input.h"

namespace RN
{
	RNDefineMeta(OculusMobileVulkanSwapChain, VulkanSwapChain)

	const uint32 OculusMobileVulkanSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, doesn't appear to be enough though...)

	OculusMobileVulkanSwapChain::OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor) : _session(nullptr), _actualFrameIndex(0), _predictedDisplayTime(0.0), _nativeWindow(nullptr)
	{
		_renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		_device = _renderer->GetVulkanDevice()->GetDevice();

		_descriptor = descriptor;
		_descriptor.depthStencilFormat = Texture::Format::Invalid;
		_descriptor.colorFormat = Texture::Format::RGBA8888;

		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

		_java.Vm = app->activity->vm;
		_java.Vm->AttachCurrentThread(&_java.Env, NULL);
		_java.ActivityObject = app->activity->clazz;

		_mainThreadID = gettid();

		// Note that AttachCurrentThread will reset the thread name.
		prctl(PR_SET_NAME, (long)"Rayne::Main", 0, 0, 0);

		ovrInitParms initParams = vrapi_DefaultInitParms(&_java);
		initParams.GraphicsAPI = VRAPI_GRAPHICS_API_VULKAN_1;
		int32_t initResult = vrapi_Initialize(&initParams);
		if(initResult != VRAPI_INITIALIZE_SUCCESS)
		{
			return;
		}

		RNInfo(GetHMDInfoDescription());

		ovrSystemCreateInfoVulkan systemInfo;
		systemInfo.Instance = _renderer->GetVulkanInstance()->GetInstance();
		systemInfo.PhysicalDevice = _renderer->GetVulkanDevice()->GetPhysicalDevice();
		systemInfo.Device = _device;
		ovrResult result = vrapi_CreateSystemVulkan(&systemInfo);
		if(result != VRAPI_INITIALIZE_SUCCESS)
		{
			return;
		}

		_eyeRenderSize.x = vrapi_GetSystemPropertyInt(&_java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
		_eyeRenderSize.y = vrapi_GetSystemPropertyInt(&_java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);

		_size.x = _eyeRenderSize.x * 2 + kEyePadding;
		_size.y = _eyeRenderSize.y;

		ovrTextureFormat textureFormat = VRAPI_TEXTURE_FORMAT_8888;//_sRGB;
/*		switch(descriptor.colorFormat)
		{
			case Texture::Format::BGRA8888SRGB:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
				break;
			}
			case Texture::Format::BGRA8888:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888;
				break;
			}
			case Texture::Format::RGBA8888SRGB:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
				break;
			}
			case Texture::Format::RGBA8888:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888;
				break;
			}
			default:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
				break;
			}
		}*/

		NotificationManager::GetSharedInstance()->AddSubscriber(kRNAndroidWindowDidChange, [this](Notification *notification) {
			if(notification->GetName()->IsEqual(kRNAndroidWindowDidChange))
			{
				_nativeWindow = nullptr;
				UpdateVRMode();
			}
		}, this);

/*		char name[1024];
		uint32_t blubb = 1024;
		vrapi_GetDeviceExtensionsVulkan(name, &blubb);
		RNDebug(RNCSTR(name) << blubb);

		blubb = 1024;
        vrapi_GetInstanceExtensionsVulkan(name, &blubb);
        RNDebug(RNCSTR(name) << blubb);*/

		//Create opengl swap chain with 3 buffers, but only expose the vulkan texture to renderer as swapchain with only 1 buffer
		_descriptor.bufferCount = 4;
		_colorSwapChain = vrapi_CreateTextureSwapChain(VRAPI_TEXTURE_TYPE_2D, textureFormat, _size.x, _size.y, 1, _descriptor.bufferCount);
		_descriptor.bufferCount = vrapi_GetTextureSwapChainLength(_colorSwapChain);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore;
			VkSemaphore renderSemaphore;
			RNVulkanValidate(vk::CreateSemaphore(_device, &semaphoreInfo, nullptr, &presentSemaphore));
			RNVulkanValidate(vk::CreateSemaphore(_device, &semaphoreInfo, nullptr, &renderSemaphore));
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}

		_framebuffer = new VulkanFramebuffer(_size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);
	}

	OculusMobileVulkanSwapChain::~OculusMobileVulkanSwapChain()
	{
		NotificationManager::GetSharedInstance()->RemoveSubscriber(kRNAndroidWindowDidChange, this);

		if(_session)
		{
			vrapi_LeaveVrMode(_session);
		}

		if(_colorSwapChain)
		{
			vrapi_DestroyTextureSwapChain(_colorSwapChain);
		}

		vrapi_DestroySystemVulkan();

		vrapi_Shutdown();
		_java.Vm->DetachCurrentThread();
	}

	const String *OculusMobileVulkanSwapChain::GetHMDInfoDescription() const
	{
		return RNCSTR("Oculus Go (maybe...)");

/*		if(!_session)
			return RNCSTR("No HMD found.");

		String *description = new String("Using HMD: ");
		description->Append(_hmdDescription.ProductName);
		description->Append(", Vendor: ");
		description->Append(_hmdDescription.Manufacturer);
		description->Append(", Firmware: %i.%i", _hmdDescription.FirmwareMajor, _hmdDescription.FirmwareMinor);

		return description;*/
	}

	void OculusMobileVulkanSwapChain::UpdateVRMode()
    {
    	if(!_nativeWindow)
    	{
    		if(!_session)
    		{
    			android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
                _nativeWindow = app->window;

    			ovrModeParmsVulkan params = vrapi_DefaultModeParmsVulkan(&_java, (unsigned long long)_renderer->GetWorkQueue());
				params.ModeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
                params.ModeParms.WindowSurface = (size_t)_nativeWindow;
                _session = vrapi_EnterVrMode((ovrModeParms *)&params);

    			// If entering VR mode failed then the ANativeWindow was not valid.
    			if(!_session)
    			{
    				RNDebug(RNCSTR("Invalid ANativeWindow!"));
    				_nativeWindow = nullptr;
    			}

    			// Set performance parameters once we have entered VR mode and have a valid ovrMobile.
    			if(_session)
    			{
    				//vrapi_SetDisplayRefreshRate(_session, 72.0f);
    				vrapi_SetRemoteEmulation(_session, false);
    				vrapi_SetClockLevels(_session, 5, 5);
					vrapi_SetPerfThread(_session, VRAPI_PERF_THREAD_TYPE_MAIN, _mainThreadID);
//    				vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, app->RenderThreadTid);
    			}
    		}
    	}
    	else
    	{
    		if(_session)
    		{
    			vrapi_LeaveVrMode(_session);
    			_session = nullptr;
    		}
    	}
    }


	void OculusMobileVulkanSwapChain::AcquireBackBuffer()
	{
		_semaphoreIndex += 1;
		_semaphoreIndex %= _descriptor.bufferCount;

		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 0;
        submitInfo.pCommandBuffers = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &_presentSemaphores[_semaphoreIndex];
        submitInfo.pWaitDstStageMask = &pipelineStageFlags;
        RNVulkanValidate(vk::QueueSubmit(_renderer->GetWorkQueue(), 1, &submitInfo, nullptr));

		if(!_session) return;

		_actualFrameIndex++;
		_predictedDisplayTime = vrapi_GetPredictedDisplayTime(_session, _actualFrameIndex);
	}

    void OculusMobileVulkanSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{

	}

    void OculusMobileVulkanSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{

	}

    void OculusMobileVulkanSwapChain::PresentBackBuffer(VkQueue queue)
	{
		if(_session)
		{
			ovrLayerProjection2 gameLayer = vrapi_DefaultLayerProjection2();

			gameLayer.HeadPose = _hmdState.HeadPose;
			for(int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++)
			{
				gameLayer.Textures[eye].ColorSwapChain = _colorSwapChain;
				gameLayer.Textures[eye].SwapChainIndex = _semaphoreIndex;
				gameLayer.Textures[eye].TexCoordsFromTanAngles = GetTanAngleMatrixForEye(eye);
				gameLayer.Textures[eye].TextureRect.x = (eye * (_eyeRenderSize.x + kEyePadding))/_size.x;
				gameLayer.Textures[eye].TextureRect.y = 0.0f;
				gameLayer.Textures[eye].TextureRect.width = _eyeRenderSize.x/_size.x;
				gameLayer.Textures[eye].TextureRect.height = 1.0f;
			}
			gameLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

			const ovrLayerHeader2 * layers[] = { &gameLayer.Header };

			ovrSubmitFrameDescription2 frameDescription = {};
			frameDescription.Flags = 0;
			frameDescription.SwapInterval = 1;
			frameDescription.FrameIndex = _actualFrameIndex;
			frameDescription.DisplayTime = _predictedDisplayTime;
			frameDescription.LayerCount = 1;
			frameDescription.Layers = layers;
			ovrResult result = vrapi_SubmitFrame2(_session, &frameDescription);
		}


		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 0;
		submitInfo.pCommandBuffers = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &_renderSemaphores[_semaphoreIndex];
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = &pipelineStageFlags;
		RNVulkanValidate(vk::QueueSubmit(queue, 1, &submitInfo, nullptr));
	}

    VkImage OculusMobileVulkanSwapChain::GetVulkanColorBuffer(int i) const
	{
		return vrapi_GetTextureSwapChainBufferVulkan(_colorSwapChain, i);
	}

    VkImage OculusMobileVulkanSwapChain::GetVulkanDepthBuffer(int i) const
	{
		return nullptr;
	}

	void OculusMobileVulkanSwapChain::UpdatePredictedPose()
	{
		if(!_session) return;

		_hmdState = vrapi_GetPredictedTracking2(_session, _predictedDisplayTime);
	}

	void OculusMobileVulkanSwapChain::SetProjection(float m22, float m23, float m32)
	{
//		_imageLayer.ProjectionDesc.Projection22 = m22;
//		_imageLayer.ProjectionDesc.Projection23 = m23;
//		_imageLayer.ProjectionDesc.Projection32 = m32;
	}

	ovrMatrix4f OculusMobileVulkanSwapChain::GetTanAngleMatrixForEye(uint8 eye)
    {
    	ovrMatrix4f *projection = &_hmdState.Eye[eye].ProjectionMatrix;
    	float eyeOffset = (eye * (_eyeRenderSize.x + kEyePadding))/_size.x;
    	float eyeFactor = _eyeRenderSize.x/_size.x;

    	const ovrMatrix4f tanAngleMatrix =
    	{ {
    		{ eyeFactor * 0.5f * projection->M[0][0], 0.0f, 0.5f * projection->M[0][2] - 0.25f - eyeOffset, 0.0f },
    		{ 0.0f, -0.5f * projection->M[1][1], 0.5f * projection->M[1][2] - 0.5f, 0.0f },
    		{ 0.0f, 0.0f, -1.0f, 0.0f },
    		// Store the values to convert a clip-Z to a linear depth in the unused matrix elements.
    		{ projection->M[2][2], projection->M[2][3], projection->M[3][2], 1.0f }
    	} };
    	return tanAngleMatrix;
    }
}
