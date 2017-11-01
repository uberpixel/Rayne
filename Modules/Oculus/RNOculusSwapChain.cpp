//
//  RNOculusSwapChain.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"

namespace RN
{
	RNDefineMeta(OculusSwapChain, D3D12SwapChain)

	const uint32 OculusSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, doesn't appear to be enough though...)

	OculusSwapChain::OculusSwapChain(const Window::SwapChainDescriptor &descriptor) : _submitResult(0), _frameCounter(0)
	{
		_session = nullptr;
		_descriptor = descriptor;

		ovrResult result = ovr_Initialize(nullptr);
		if(OVR_FAILURE(result))
			return;

		
		result = ovr_Create(&_session, &_luID);
		if(OVR_FAILURE(result))
		{
			_session = nullptr;
			RNInfo(GetHMDInfoDescription());
			ovr_Shutdown();
			return;
		}

		_hmdDescription = ovr_GetHmdDesc(_session);
		RNInfo(GetHMDInfoDescription());

		// Configure Stereo settings.
		ovrSizei recommenedTex0Size = ovr_GetFovTextureSize(_session, ovrEye_Left, _hmdDescription.DefaultEyeFov[0], 1.0f);
		ovrSizei recommenedTex1Size = ovr_GetFovTextureSize(_session, ovrEye_Right, _hmdDescription.DefaultEyeFov[1], 1.0f);
		ovrSizei bufferSize;
		bufferSize.w = recommenedTex0Size.w + recommenedTex1Size.w + kEyePadding;
		bufferSize.h = std::max(recommenedTex0Size.h, recommenedTex1Size.h);
		_size.x = bufferSize.w;
		_size.y = bufferSize.h;

		ovrTextureSwapChainDesc swapChainDesc = {};
		swapChainDesc.Type = ovrTexture_2D;
		swapChainDesc.ArraySize = 1;
		swapChainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
		swapChainDesc.Width = bufferSize.w;
		swapChainDesc.Height = bufferSize.h;
		swapChainDesc.MipLevels = 1;
		swapChainDesc.SampleCount = 1;
		swapChainDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
		swapChainDesc.StaticImage = ovrFalse;
		swapChainDesc.BindFlags = ovrTextureBind_DX_RenderTarget;

		_renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();

		result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &swapChainDesc, &_textureSwapChain);
		if(!OVR_SUCCESS(result))
			return;
		int textureCount = 0;
		ovr_GetTextureSwapChainLength(_session, _textureSwapChain, &textureCount);
		_descriptor.bufferCount = textureCount;

		_framebuffer = new D3D12Framebuffer(_size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);

		// Initialize VR structures, filling out description.
		_eyeRenderDesc[0] = ovr_GetRenderDesc(_session, ovrEye_Left, _hmdDescription.DefaultEyeFov[0]);
		_eyeRenderDesc[1] = ovr_GetRenderDesc(_session, ovrEye_Right, _hmdDescription.DefaultEyeFov[1]);
		_hmdToEyeViewPose[0] = _eyeRenderDesc[0].HmdToEyePose;
		_hmdToEyeViewPose[1] = _eyeRenderDesc[1].HmdToEyePose;

		// Initialize our single full screen Fov layer.
		_layer.Header.Type = ovrLayerType_EyeFov;
		_layer.Header.Flags = 0;
		_layer.ColorTexture[0] = _textureSwapChain;
		_layer.ColorTexture[1] = _textureSwapChain;
		_layer.Fov[0] = _eyeRenderDesc[0].Fov;
		_layer.Fov[1] = _eyeRenderDesc[1].Fov;
		_layer.Viewport[0].Pos.x = 0;
		_layer.Viewport[0].Pos.y = 0;
		_layer.Viewport[0].Size.w = recommenedTex0Size.w;
		_layer.Viewport[0].Size.h = recommenedTex0Size.h;
		_layer.Viewport[1].Pos.x = recommenedTex0Size.w + kEyePadding;
		_layer.Viewport[1].Pos.y = 0;
		_layer.Viewport[1].Size.w = recommenedTex1Size.w;
		_layer.Viewport[1].Size.h = recommenedTex1Size.h;

		ovr_SetTrackingOriginType(_session, ovrTrackingOrigin_FloorLevel);
	}

	OculusSwapChain::~OculusSwapChain()
	{
		ovr_Destroy(_session);
		ovr_Shutdown();
	}

	const String *OculusSwapChain::GetHMDInfoDescription() const
	{
		if(!_session)
			return RNCSTR("No HMD found.");

		String *description = new String("Using HMD: ");
		description->Append(_hmdDescription.ProductName);
		description->Append(", Vendor: ");
		description->Append(_hmdDescription.Manufacturer);
		description->Append(", Firmware: %i.%i", _hmdDescription.FirmwareMajor, _hmdDescription.FirmwareMinor);

		return description;
	}


	void OculusSwapChain::AcquireBackBuffer()
	{
		_submitResult = ovr_WaitToBeginFrame(_session, ++_frameCounter);

		// Get next available index of the texture swap chain
		int currentIndex = 0;
		ovr_GetTextureSwapChainCurrentIndex(_session, _textureSwapChain, &currentIndex);
		_frameIndex = currentIndex;
	}

	void OculusSwapChain::Prepare(D3D12CommandList *commandList)
	{
		if(OVR_SUCCESS(_submitResult))
		{
			ovrResult tempResult = ovr_BeginFrame(_session, _frameCounter);
			if(_submitResult != ovrSuccess_NotVisible || !OVR_SUCCESS(tempResult))
			{
				_submitResult = tempResult;
			}
		}
	}

	void OculusSwapChain::Finalize(D3D12CommandList *commandList)
	{

	}

	void OculusSwapChain::PresentBackBuffer()
	{
		// Commit the changes to the texture swap chain
		ovr_CommitTextureSwapChain(_session, _textureSwapChain);

		// Submit frame with one layer we have.
		ovrLayerHeader* layers = &_layer.Header;
		if(OVR_SUCCESS(_submitResult))
		{
			ovrResult tempResult = ovr_EndFrame(_session, _frameCounter, nullptr, &layers, 1);
			if(_submitResult != ovrSuccess_NotVisible || !OVR_SUCCESS(tempResult))
			{
				_submitResult = tempResult;
			}
		}
	}

	ID3D12Resource *OculusSwapChain::GetD3D12Buffer(int i) const
	{
		ID3D12Resource *buffer;
		ovr_GetTextureSwapChainBufferDX(_session, _textureSwapChain, i, IID_PPV_ARGS(&buffer));
		return buffer;
	}

	void OculusSwapChain::UpdatePredictedPose()
	{
		double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);	//TODO: Frameindex as second param
		_hmdState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);
		ovr_CalcEyePoses(_hmdState.HeadPose.ThePose, _hmdToEyeViewPose, _layer.RenderPose);
	}
}
