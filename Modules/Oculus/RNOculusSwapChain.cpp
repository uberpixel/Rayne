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

	OculusSwapChain::OculusSwapChain(const Window::SwapChainDescriptor &descriptor) : _submitResult(0), _frameCounter(0), _depthSwapChain(nullptr)
	{
		_session = nullptr;
		_descriptor = descriptor;

		ovrInitParams initParams;
		initParams.Flags = ovrInit_FocusAware;
		initParams.LogCallback = nullptr;
		initParams.UserData = 0;
		initParams.ConnectionTimeoutMS = 0;
		ovrResult result = ovr_Initialize(&initParams);
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
		switch(descriptor.colorFormat)
		{
			case Texture::Format::BGRA8888SRGB:
			{
				swapChainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
				break;
			}
			case Texture::Format::BGRA8888:
			{
				swapChainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM;
				break;
			}
			case Texture::Format::RGBA8888SRGB:
			{
				swapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			}
			case Texture::Format::RGBA8888:
			{
				swapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM;
				break;
			}
			default:
			{
				swapChainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
				break;
			}
		}
		swapChainDesc.Width = bufferSize.w;
		swapChainDesc.Height = bufferSize.h;
		swapChainDesc.MipLevels = 1;
		swapChainDesc.SampleCount = 1;
		swapChainDesc.MiscFlags = 0;// ovrTextureMisc_DX_Typeless;
		swapChainDesc.StaticImage = ovrFalse;
		swapChainDesc.BindFlags = ovrTextureBind_DX_RenderTarget;

		_renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();

		result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &swapChainDesc, &_colorSwapChain);
		if(!OVR_SUCCESS(result))
			return;
		int textureCount = 0;
		ovr_GetTextureSwapChainLength(_session, _colorSwapChain, &textureCount);
		_descriptor.bufferCount = textureCount;

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

		ovr_SetTrackingOriginType(_session, ovrTrackingOrigin_FloorLevel);
	}

	OculusSwapChain::~OculusSwapChain()
	{
		if(_colorSwapChain)
		{
			ovr_DestroyTextureSwapChain(_session, _colorSwapChain);
		}
		if (_depthSwapChain)
		{
			ovr_DestroyTextureSwapChain(_session, _depthSwapChain);
		}
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
		ovr_GetTextureSwapChainCurrentIndex(_session, _colorSwapChain, &currentIndex);
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
		ovr_CommitTextureSwapChain(_session, _colorSwapChain);

		if(_depthSwapChain)
		{
			ovr_CommitTextureSwapChain(_session, _depthSwapChain);
		}

		// Submit frame with one layer we have.
		if(OVR_SUCCESS(_submitResult))
		{
			ovrLayerHeader *layers = &_imageLayer.Header;
			ovrResult tempResult = ovr_EndFrame(_session, _frameCounter, nullptr, &layers, 1);
			if(_submitResult != ovrSuccess_NotVisible || !OVR_SUCCESS(tempResult))
			{
				_submitResult = tempResult;
			}
		}
	}

	ID3D12Resource *OculusSwapChain::GetD3D12ColorBuffer(int i) const
	{
		ID3D12Resource *buffer;
		ovr_GetTextureSwapChainBufferDX(_session, _colorSwapChain, i, IID_PPV_ARGS(&buffer));
		return buffer;
	}

	ID3D12Resource *OculusSwapChain::GetD3D12DepthBuffer(int i) const
	{
		ID3D12Resource *buffer;
		ovr_GetTextureSwapChainBufferDX(_session, _depthSwapChain, i, IID_PPV_ARGS(&buffer));
		return buffer;
	}

	void OculusSwapChain::UpdatePredictedPose()
	{
		double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);	//TODO: Frameindex as second param
		_hmdState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);
		ovr_CalcEyePoses(_hmdState.HeadPose.ThePose, _hmdToEyeViewPose, _imageLayer.RenderPose);
	}

	void OculusSwapChain::SetProjection(float m22, float m23, float m32)
	{
		_imageLayer.ProjectionDesc.Projection22 = m22;
		_imageLayer.ProjectionDesc.Projection23 = m23;
		_imageLayer.ProjectionDesc.Projection32 = m32;
	}
}
