//
//  RNCaptureWindowsSwapChain.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_WINDOWS_SWAP_CHAIN_H_
#define __RN_CAPTURE_WINDOWS_SWAP_CHAIN_H_

#include <Rayne.h>

#if RN_PLATFORM_WINDOWS
	#include <RNVulkanSwapChain.h>

struct ID3D11Device;
struct ID3D11Texture2D;

namespace RN
{
	class CaptureWindowsSwapChain final : public VulkanSwapChain
	{
	public:
		using FrameReadyCallback = std::function<void(ID3D11Texture2D *texture, int64 presentationTimeUs, int64 durationUs)>;

		CaptureWindowsSwapChain(VulkanRenderer *renderer, const Vector2 &size, const Window::SwapChainDescriptor &descriptor);
		~CaptureWindowsSwapChain() override;

		bool GetIsValid() const;

		ID3D11Device *GetD3DDevice() const;
		ID3D11Texture2D *GetD3DTexture(size_t index) const;
		ID3D11Texture2D *GetCurrentD3DTexture() const;

		void QueueFrame(int64 presentationTimeUs, int64 durationUs);
		size_t ClearQueuedFrames();
		void SetFrameReadyCallback(FrameReadyCallback callback);

		void AcquireBackBuffer() final;
		void Prepare(VkCommandBuffer commandBuffer) final;
		void Finalize(VkCommandBuffer commandBuffer) final;
		void PresentBackBuffer(VkQueue queue) final;

		VkImage GetVulkanColorBuffer(int index) const final;
		VkImage GetVulkanDepthBuffer(int index) const final;

	private:
		struct Internals;

		bool CreateD3DDeviceForVulkanDevice();
		bool CreateSharedBuffers();

		Internals *_internals;
		bool _isValid;
	};
} // namespace RN
#endif

#endif /* defined(__RN_CAPTURE_WINDOWS_SWAP_CHAIN_H_) */
