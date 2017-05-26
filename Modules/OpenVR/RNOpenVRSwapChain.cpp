//
//  RNOpenVRSwapChain.cpp
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenVRSwapChain.h"
#include "RND3D12Internals.h"

namespace RN
{
	RNDefineMeta(OpenVRSwapChain, D3D12SwapChain)

	const uint32 OpenVRSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, but it isn't enough)

	OpenVRSwapChain::OpenVRSwapChain(const Window::SwapChainDescriptor &descriptor) : _isFirstRender(true)
	{
		vr::EVRInitError eError = vr::VRInitError_None;
		_hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);

		if (eError != vr::VRInitError_None)
		{
			_hmd = nullptr;
			RNDebug("OpenVR: Unable to init VR runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError));
			return;
		}

		RNInfo(GetHMDInfoDescription());

		_renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		_descriptor = descriptor;

		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_hmd->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		_size = Vector2(recommendedWidth * 2 + kEyePadding, recommendedHeight);

		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(Texture::Format::RGBA8888SRGB, _size.x, _size.y, false);
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);

		_descriptor.bufferCount = 1;
		_frameIndex = 0;
		_framebuffer = new D3D12Framebuffer(_size, this, _renderer, Texture::Format::RGBA8888SRGB, Texture::Format::Depth24Stencil8);

		//TODO: Update every frame, maybe move to window
		vr::HmdMatrix34_t leftEyeMatrix = _hmd->GetEyeToHeadTransform(vr::Eye_Left);
		vr::HmdMatrix34_t rightEyeMatrix = _hmd->GetEyeToHeadTransform(vr::Eye_Right);
		_hmdToEyeViewOffset[0].x = leftEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[0].y = leftEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[0].z = leftEyeMatrix.m[2][3];
		_hmdToEyeViewOffset[1].x = rightEyeMatrix.m[0][3];
		_hmdToEyeViewOffset[1].y = rightEyeMatrix.m[1][3];
		_hmdToEyeViewOffset[1].z = rightEyeMatrix.m[2][3];
	}

	OpenVRSwapChain::~OpenVRSwapChain()
	{
		SafeRelease(_targetTexture);
		if(_hmd) vr::VR_Shutdown();
	}

	const String *OpenVRSwapChain::GetHMDInfoDescription() const
	{
		if (!_hmd)
			return RNCSTR("No HMD found.");

		//TODO: Implement similar to the oculus module
		String *description = new String("Using HMD: ");
/*		description->Append(_hmdDescription.ProductName);
		description->Append(", Vendor: ");
		description->Append(_hmdDescription.Manufacturer);
		description->Append(", Firmware: %i.%i", _hmdDescription.FirmwareMajor, _hmdDescription.FirmwareMinor);*/

		return description;
	}


	void OpenVRSwapChain::AcquireBackBuffer()
	{
/*		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_hmd->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		Vector2 newSize(recommendedWidth * 2 + kEyePadding, recommendedHeight);

		if(newSize.GetSquaredDistance(_size) > 0.001f)
		{
			_size = newSize;
			//_framebuffer->WillUpdateSwapChain(); //As all it does is free the swap chain d3d buffer resources, it would free the targetTexture resource and should't be called in this case...
			_targetTexture->Release();
			Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(Texture::Format::RGBA8888SRGB, _size.x, _size.y, false);
			textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
			_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);
			_framebuffer->DidUpdateSwapChain(_size, Texture::Format::RGBA8888SRGB, Texture::Format::Depth24Stencil8);
		}*/
	}

	void OpenVRSwapChain::Prepare(D3D12CommandList *commandList)
	{
		if (_isFirstRender)
			return;

		_targetTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	void OpenVRSwapChain::Finalize(D3D12CommandList *commandList)
	{
		_targetTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		_isFirstRender = false;
	}

	void OpenVRSwapChain::PresentBackBuffer()
	{
		vr::D3D12TextureData_t d3d12EyeTexture;
		d3d12EyeTexture.m_pResource = _targetTexture->Downcast<D3D12Texture>()->GetD3D12Resource();
		d3d12EyeTexture.m_pCommandQueue = _renderer->GetCommandQueue();
		d3d12EyeTexture.m_nNodeMask = 0;
		vr::Texture_t eyeTexture = { (void *)&d3d12EyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };

		vr::VRTextureBounds_t bounds;
		bounds.vMin = 0.0f;
		bounds.vMax = 1.0f;

		bounds.uMin = 0.0f;
		bounds.uMax = 0.5f - kEyePadding * 0.5f / _size.x;
		vr::VRCompositor()->Submit(vr::Eye_Left, &eyeTexture, &bounds, vr::Submit_Default);

		bounds.uMin = 0.5f + kEyePadding * 0.5f / _size.x;
		bounds.uMax = 1.0f;
		vr::VRCompositor()->Submit(vr::Eye_Right, &eyeTexture, &bounds, vr::Submit_Default);
	}

	ID3D12Resource *OpenVRSwapChain::GetD3D12Buffer(int i) const
	{
		return _targetTexture->Downcast<D3D12Texture>()->GetD3D12Resource();
	}

	void OpenVRSwapChain::UpdatePredictedPose()
	{
		vr::VRCompositor()->WaitGetPoses(_frameDevicePose, vr::k_unMaxTrackedDeviceCount, _predictedDevicePose, vr::k_unMaxTrackedDeviceCount);
	}
}
