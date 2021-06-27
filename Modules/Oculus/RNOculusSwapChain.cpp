//
//  RNOculusSwapChain.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"
#include "RND3D12Internals.h"

namespace RN
{
	RNDefineMeta(OculusSwapChain, D3D12SwapChain)

	OculusSwapChain::OculusSwapChain(const Window::SwapChainDescriptor &descriptor) : _submitResult(0), _frameCounter(0), _colorSwapChain{ nullptr, nullptr }, _depthSwapChain{nullptr, nullptr}, _colorTexture(nullptr), _depthTexture(nullptr)
	{
		_session = nullptr;
		_descriptor = descriptor;
		_descriptor.layerCount = 2;

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
		bufferSize.w = std::max(recommenedTex0Size.w, recommenedTex1Size.w);
		bufferSize.h = std::max(recommenedTex0Size.h, recommenedTex1Size.h);
		_size.x = bufferSize.w;
		_size.y = bufferSize.h;

		ovrTextureSwapChainDesc swapChainDesc = {};
		swapChainDesc.Type = ovrTexture_2D;
		swapChainDesc.ArraySize = 2;
		switch(descriptor.colorFormat)
		{
			case Texture::Format::BGRA_8_SRGB:
			{
				swapChainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
				break;
			}
			case Texture::Format::BGRA_8:
			{
				swapChainDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM;
				break;
			}
			case Texture::Format::RGBA_8_SRGB:
			{
				swapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			}
			case Texture::Format::RGBA_8:
			{
				swapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM;
				break;
			}
			case Texture::Format::RGBA_16F:
			{
				swapChainDesc.Format = OVR_FORMAT_R16G16B16A16_FLOAT;
				break;
			}
			default:
			{
				RN_ASSERT(false, "The swap chain color format is not supported!");
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

		result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &swapChainDesc, &_colorSwapChain[0]);
		if(!OVR_SUCCESS(result))
			return;
		result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &swapChainDesc, &_colorSwapChain[1]);
		if (!OVR_SUCCESS(result))
			return;
		
		int textureCount = 0;
		ovr_GetTextureSwapChainLength(_session, _colorSwapChain[0], &textureCount);
		_descriptor.bufferCount = textureCount;

		if(descriptor.depthStencilFormat != Texture::Format::Invalid)
		{
			ovrTextureSwapChainDesc depthSwapChainDesc = {};
			depthSwapChainDesc.Type = ovrTexture_2D;
			depthSwapChainDesc.ArraySize = 1;
			switch(descriptor.depthStencilFormat)
			{
				case Texture::Format::Depth_16I:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D16_UNORM;
					break;
				}
				case Texture::Format::Depth_24I:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D24_UNORM_S8_UINT;
					break;
				}
				case Texture::Format::Depth_24_Stencil_8:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D24_UNORM_S8_UINT;
					break;
				}
				case Texture::Format::Depth_32F:
				{
					depthSwapChainDesc.Format = OVR_FORMAT_D32_FLOAT;
					break;
				}
				case Texture::Format::Depth_32F_Stencil_8:
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

			result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &depthSwapChainDesc, &_depthSwapChain[0]);
			if (!OVR_SUCCESS(result))
				return;

			result = ovr_CreateTextureSwapChainDX(_session, _renderer->GetCommandQueue(), &depthSwapChainDesc, &_depthSwapChain[1]);
			if (!OVR_SUCCESS(result))
				return;
		}

		_swapChainColorBuffers[0] = new ID3D12Resource*[_descriptor.bufferCount];
		_swapChainColorBuffers[1] = new ID3D12Resource*[_descriptor.bufferCount];
		_swapChainDepthBuffers[0] = new ID3D12Resource*[_descriptor.bufferCount];
		_swapChainDepthBuffers[1] = new ID3D12Resource*[_descriptor.bufferCount];

		for(int i = 0; i < _descriptor.bufferCount; i++)
		{
			ovr_GetTextureSwapChainBufferDX(_session, _colorSwapChain[0], i, IID_PPV_ARGS(&_swapChainColorBuffers[0][i]));
			ovr_GetTextureSwapChainBufferDX(_session, _colorSwapChain[1], i, IID_PPV_ARGS(&_swapChainColorBuffers[1][i]));
			ovr_GetTextureSwapChainBufferDX(_session, _depthSwapChain[0], i, IID_PPV_ARGS(&_swapChainDepthBuffers[0][i]));
			ovr_GetTextureSwapChainBufferDX(_session, _depthSwapChain[1], i, IID_PPV_ARGS(&_swapChainDepthBuffers[1][i]));
		}

		// Initialize VR structures, filling out description.
		_eyeRenderDesc[0] = ovr_GetRenderDesc(_session, ovrEye_Left, _hmdDescription.DefaultEyeFov[0]);
		_eyeRenderDesc[1] = ovr_GetRenderDesc(_session, ovrEye_Right, _hmdDescription.DefaultEyeFov[1]);
		_hmdToEyeViewPose[0] = _eyeRenderDesc[0].HmdToEyePose;
		_hmdToEyeViewPose[1] = _eyeRenderDesc[1].HmdToEyePose;

		// Initialize our single full screen Fov layer.
		_imageLayer.Header.Type = _depthSwapChain[0]?ovrLayerType_EyeFovDepth:ovrLayerType_EyeFov;
		_imageLayer.Header.Flags = 0;
		_imageLayer.ColorTexture[0] = _colorSwapChain[0];
		_imageLayer.ColorTexture[1] = _colorSwapChain[1];
		_imageLayer.DepthTexture[0] = _depthSwapChain[0];
		_imageLayer.DepthTexture[1] = _depthSwapChain[1];
		_imageLayer.Fov[0] = _eyeRenderDesc[0].Fov;
		_imageLayer.Fov[1] = _eyeRenderDesc[1].Fov;
		_imageLayer.Viewport[0].Pos.x = 0;
		_imageLayer.Viewport[0].Pos.y = 0;
		_imageLayer.Viewport[0].Size.w = recommenedTex0Size.w;
		_imageLayer.Viewport[0].Size.h = recommenedTex0Size.h;
		_imageLayer.Viewport[1].Pos.x = 0;
		_imageLayer.Viewport[1].Pos.y = 0;
		_imageLayer.Viewport[1].Size.w = recommenedTex1Size.w;
		_imageLayer.Viewport[1].Size.h = recommenedTex1Size.h;

		ovr_SetTrackingOriginType(_session, ovrTrackingOrigin_FloorLevel);

		Texture::Descriptor textureDescriptor;
		textureDescriptor.type = Texture::Type::Type2DArray;
		textureDescriptor.width = _size.x;
		textureDescriptor.height = _size.y;
		textureDescriptor.depth = _descriptor.layerCount;
		textureDescriptor.format = _descriptor.colorFormat;
		textureDescriptor.usageHint = Texture::UsageHint::ShaderRead | Texture::UsageHint::RenderTarget;
		textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
		_colorTexture = RN::Texture::WithDescriptor(textureDescriptor)->Downcast<D3D12Texture>();
		_colorTexture->Retain();

		if(_depthSwapChain[0])
		{
			textureDescriptor.format = _descriptor.depthStencilFormat;
			_depthTexture = RN::Texture::WithDescriptor(textureDescriptor)->Downcast<D3D12Texture>();
			_depthTexture->Retain();
		}

		//TODO: Find out why setting buffer count to 1 makes the renderer crash
		//_descriptor.bufferCount = 1; //This is the actual buffer count of this swapchain object as it is only backed by one texture
		_framebuffer = new D3D12Framebuffer(_size, 2, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);
	}

	OculusSwapChain::~OculusSwapChain()
	{
		SafeRelease(_colorTexture);
		SafeRelease(_depthTexture);
		
		if(_colorSwapChain[0])
		{
			ovr_DestroyTextureSwapChain(_session, _colorSwapChain[0]);
			ovr_DestroyTextureSwapChain(_session, _colorSwapChain[1]);
		}
		if(_depthSwapChain[0])
		{
			ovr_DestroyTextureSwapChain(_session, _depthSwapChain[0]);
			ovr_DestroyTextureSwapChain(_session, _depthSwapChain[1]);
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
		ovr_GetTextureSwapChainCurrentIndex(_session, _colorSwapChain[0], &currentIndex);
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
		D3D12_RESOURCE_STATES oldColorSourceState = _colorTexture->GetCurrentState();
		_colorTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);

		D3D12_RESOURCE_STATES oldDepthSourceState;
		if(_depthTexture)
		{
			oldDepthSourceState = _depthTexture->GetCurrentState();
			_depthTexture->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		}

		for(int eye = 0; eye < 2; eye++)
		{
			commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_swapChainColorBuffers[eye][_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));

			CD3DX12_TEXTURE_COPY_LOCATION colorSourceLocation(_colorTexture->GetD3D12Resource(), eye);
			CD3DX12_TEXTURE_COPY_LOCATION colorDestinationLocation(_swapChainColorBuffers[eye][_frameIndex], 0);
			commandList->GetCommandList()->CopyTextureRegion(&colorDestinationLocation, 0, 0, 0, &colorSourceLocation, nullptr);

			commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_swapChainColorBuffers[eye][_frameIndex], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));

			if(_depthTexture)
			{
				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_swapChainDepthBuffers[eye][_frameIndex], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

				CD3DX12_TEXTURE_COPY_LOCATION depthSourceLocation(_depthTexture->GetD3D12Resource(), eye);
				CD3DX12_TEXTURE_COPY_LOCATION depthDestinationLocation(_swapChainDepthBuffers[eye][_frameIndex], 0);
				commandList->GetCommandList()->CopyTextureRegion(&depthDestinationLocation, 0, 0, 0, &depthSourceLocation, nullptr);

				commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_swapChainDepthBuffers[eye][_frameIndex], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
			}
		}

		_colorTexture->TransitionToState(commandList, oldColorSourceState);
		if(_depthTexture)	_depthTexture->TransitionToState(commandList, oldDepthSourceState);
	}

	void OculusSwapChain::PresentBackBuffer()
	{
		// Commit the changes to the texture swap chain
		ovr_CommitTextureSwapChain(_session, _colorSwapChain[0]);
		ovr_CommitTextureSwapChain(_session, _colorSwapChain[1]);

		if(_depthSwapChain)
		{
			ovr_CommitTextureSwapChain(_session, _depthSwapChain[0]);
			ovr_CommitTextureSwapChain(_session, _depthSwapChain[1]);
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
		return _colorTexture->GetD3D12Resource();
	}

	ID3D12Resource *OculusSwapChain::GetD3D12DepthBuffer(int i) const
	{
		if(!_depthTexture) return nullptr;
		return _depthTexture->GetD3D12Resource();
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
