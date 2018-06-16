//
//  RNOculusMobileVulkanSwapChain.cpp
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusMobileVulkanSwapChain.h"

#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/window.h>				// for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h>	// for native window JNI
#include <android_native_app_glue.h>

#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"

namespace RN
{
	RNDefineMeta(OculusMobileVulkanSwapChain, VulkanSwapChain)

	const uint32 OculusMobileVulkanSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, doesn't appear to be enough though...)

	OculusMobileVulkanSwapChain::OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor) : _frameCounter(0)
	{
		_descriptor = descriptor;

		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

		_java.Vm = app->activity->vm;
		_java.Vm->AttachCurrentThread(&_java.Env, NULL);
		_java.ActivityObject = app->activity->clazz;

		// Note that AttachCurrentThread will reset the thread name.
		prctl(PR_SET_NAME, (long)"Rayne::Main", 0, 0, 0);

		const ovrInitParms initParms = vrapi_DefaultInitParms(&_java);
		int32_t initResult = vrapi_Initialize(&initParms);
		if(initResult != VRAPI_INITIALIZE_SUCCESS)
		{
			return;
		}

		RNInfo(GetHMDInfoDescription());

		int eyeWidth = vrapi_GetSystemPropertyInt(&_java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
		int eyeHeight = vrapi_GetSystemPropertyInt(&_java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);

		_size.x = eyeWidth * 2 + kEyePadding;
        _size.y = eyeHeight;

		ovrTextureFormat textureFormat = VRAPI_TEXTURE_FORMAT_8888_sRGB;
		switch(descriptor.colorFormat)
		{
			case Texture::Format::BGRA8888SRGB:
			{
				textureFormat = VRAPI_TEXTURE_FORMAT_8888;
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
		}

		//TODO: Create and set opengl context!

		_descriptor.bufferCount = 3;
        _colorSwapChain = vrapi_CreateTextureSwapChain2(VRAPI_TEXTURE_TYPE_2D, textureFormat, _size.x, _size.y, 1, _descriptor.bufferCount);
        _descriptor.bufferCount = vrapi_GetTextureSwapChainLength(_colorSwapChain);

        _framebuffer = new VulkanFramebuffer(_size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);

/*
		if (descriptor.depthStencilFormat != Texture::Format::Invalid)
		{
			ovrTextureSwapChainDesc depthSwapChainDesc = {};
			depthSwapChainDesc.Type = ovrTexture_2D;
			depthSwapChainDesc.ArraySize = 1;
			switch(descriptor.depthStencilFormat)
			{
				case Texture::Format::Depth24I:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D24_UNORM_S8_UINT;
					break;
				}
				case Texture::Format::Depth24Stencil8:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D24_UNORM_S8_UINT;
					break;
				}
				case Texture::Format::Depth32F:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D32_FLOAT;
					break;
				}
				case Texture::Format::Depth32FStencil8:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D32_FLOAT_S8X24_UINT;
					break;
				}
				default:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D24_UNORM_S8_UINT;
					break;
				}
			}
			depthSwapChainDesc.Width = bufferSize.w;
			depthSwapChainDesc.Height = bufferSize.h;
			depthSwapChainDesc.MipLevels = 1;
			depthSwapChainDesc.SampleCount = 1;
			depthSwapChainDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
			depthSwapChainDesc.StaticImage = ovrFalse;
			depthSwapChainDesc.BindFlags = ovrTextureBind_DX_DepthStencil;

			result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &depthSwapChainDesc, &_depthSwapChain);
			if (!OVR_SUCCESS(result))
				return;
		}

		_framebuffer = new D3D12Framebuffer(_size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);

		// Initialize VR structures, filling out description.
		_eyeRenderDesc[0] = ovr_GetRenderDesc(_session, ovrEye_Left, _hmdDescription.DefaultEyeFov[0]);
		_eyeRenderDesc[1] = ovr_GetRenderDesc(_session, ovrEye_Right, _hmdDescription.DefaultEyeFov[1]);
		_hmdToEyeViewPose[0] = _eyeRenderDesc[0].HmdToEyePose;
		_hmdToEyeViewPose[1] = _eyeRenderDesc[1].HmdToEyePose;

		// Initialize our single full screen Fov layer.
		_imageLayer.Header.Type = _depthSwapChain?ovrLayerType_EyeFovDepth:ovrLayerType_EyeFov;
		_imageLayer.Header.Flags = 0;
		_imageLayer.ColorTexture[0] = _colorSwapChain;
		_imageLayer.ColorTexture[1] = _colorSwapChain;
		_imageLayer.DepthTexture[0] = _depthSwapChain;
		_imageLayer.DepthTexture[1] = _depthSwapChain;
		_imageLayer.Fov[0] = _eyeRenderDesc[0].Fov;
		_imageLayer.Fov[1] = _eyeRenderDesc[1].Fov;
		_imageLayer.Viewport[0].Pos.x = 0;
		_imageLayer.Viewport[0].Pos.y = 0;
		_imageLayer.Viewport[0].Size.w = recommenedTex0Size.w;
		_imageLayer.Viewport[0].Size.h = recommenedTex0Size.h;
		_imageLayer.Viewport[1].Pos.x = recommenedTex0Size.w + kEyePadding;
		_imageLayer.Viewport[1].Pos.y = 0;
		_imageLayer.Viewport[1].Size.w = recommenedTex1Size.w;
		_imageLayer.Viewport[1].Size.h = recommenedTex1Size.h;

		ovr_SetTrackingOriginType(_session, ovrTrackingOrigin_FloorLevel);*/
	}

	OculusMobileVulkanSwapChain::~OculusMobileVulkanSwapChain()
	{
		if(_colorSwapChain)
		{
			vrapi_DestroyTextureSwapChain(_colorSwapChain);
		}

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


	void OculusMobileVulkanSwapChain::AcquireBackBuffer()
	{
/*		_submitResult = ovr_WaitToBeginFrame(_session, ++_frameCounter);

		// Get next available index of the texture swap chain
		int currentIndex = 0;
		ovr_GetTextureSwapChainCurrentIndex(_session, _colorSwapChain, &currentIndex);
		_frameIndex = currentIndex;*/
	}

    void OculusMobileVulkanSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{

	}

    void OculusMobileVulkanSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{

	}

    void OculusMobileVulkanSwapChain::PresentBackBuffer(VkQueue queue)
	{

	}

    VkImage OculusMobileVulkanSwapChain::GetVulkanColorBuffer(int i) const
	{
		unsigned int textureHanddle = vrapi_GetTextureSwapChainHandle(_colorSwapChain, i);

	}

    VkImage OculusMobileVulkanSwapChain::GetVulkanDepthBuffer(int i) const
	{
		return nullptr;
	}

	void OculusMobileVulkanSwapChain::UpdatePredictedPose()
	{
//		double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);	//TODO: Frameindex as second param
//		_hmdState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);
//		ovr_CalcEyePoses(_hmdState.HeadPose.ThePose, _hmdToEyeViewPose, _imageLayer.RenderPose);
	}

	void OculusMobileVulkanSwapChain::SetProjection(float m22, float m23, float m32)
	{
//		_imageLayer.ProjectionDesc.Projection22 = m22;
//		_imageLayer.ProjectionDesc.Projection23 = m23;
//		_imageLayer.ProjectionDesc.Projection32 = m32;
	}
}
