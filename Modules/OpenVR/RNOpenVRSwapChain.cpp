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

	OpenVRSwapChain::OpenVRSwapChain(const Window::SwapChainDescriptor &descriptor)
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
		_size = Vector2(recommendedWidth * 2, recommendedHeight);

		Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(Texture::Format::RGBA8888SRGB, recommendedWidth * 2, recommendedHeight, false);
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
		_targetTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);

		textureDescriptor.width = recommendedWidth;
		textureDescriptor.height = recommendedHeight;
		_leftEyeTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);
		_rightEyeTexture = _renderer->CreateTextureWithDescriptor(textureDescriptor);

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
		SafeRelease(_leftEyeTexture);
		SafeRelease(_rightEyeTexture);
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
		
	}

	void OpenVRSwapChain::Prepare(D3D12CommandList *commandList)
	{

	}

	void OpenVRSwapChain::Finalize(D3D12CommandList *commandList)
	{
		_targetTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		_leftEyeTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
		_rightEyeTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

		CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(_targetTexture->Downcast<D3D12Texture>()->GetD3D12Resource(), 0);
		CD3DX12_TEXTURE_COPY_LOCATION leftEyeLocation(_leftEyeTexture->Downcast<D3D12Texture>()->GetD3D12Resource(), 0);
		CD3DX12_TEXTURE_COPY_LOCATION rightEyeLocation(_rightEyeTexture->Downcast<D3D12Texture>()->GetD3D12Resource(), 0);

		D3D12_BOX eyeBox;
		eyeBox.top = 0;
		eyeBox.bottom = _size.y;
		eyeBox.left = 0;
		eyeBox.right = _size.x / 2;
		eyeBox.back = 1;
		eyeBox.front = 0;
		commandList->GetCommandList()->CopyTextureRegion(&leftEyeLocation, 0, 0, 0, &sourceLocation, &eyeBox);

		eyeBox.left = _size.x / 2;
		eyeBox.right = _size.x;
		commandList->GetCommandList()->CopyTextureRegion(&rightEyeLocation, 0, 0, 0, &sourceLocation, &eyeBox);

		_targetTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_leftEyeTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		_rightEyeTexture->Downcast<D3D12Texture>()->TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void OpenVRSwapChain::PresentBackBuffer()
	{
		vr::D3D12TextureData_t d3d12LeftTexture;
		d3d12LeftTexture.m_pResource = _leftEyeTexture->Downcast<D3D12Texture>()->GetD3D12Resource();
		d3d12LeftTexture.m_pCommandQueue = _renderer->GetCommandQueue();
		d3d12LeftTexture.m_nNodeMask = 0;
		vr::Texture_t leftEyeTexture = { (void *)&d3d12LeftTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };

		vr::D3D12TextureData_t d3d12RightTexture;
		d3d12RightTexture.m_pResource = _rightEyeTexture->Downcast<D3D12Texture>()->GetD3D12Resource();
		d3d12RightTexture.m_pCommandQueue = _renderer->GetCommandQueue();
		d3d12RightTexture.m_nNodeMask = 0;
		vr::Texture_t rightEyeTexture = { (void *)&d3d12RightTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };

		vr::VRTextureBounds_t bounds;
		bounds.uMin = 0.0f;
		bounds.uMax = 1.0f;
		bounds.vMin = 0.0f;
		bounds.vMax = 1.0f;
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture, &bounds, vr::Submit_Default);
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture, &bounds, vr::Submit_Default);
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
