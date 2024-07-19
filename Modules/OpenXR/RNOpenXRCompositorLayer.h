//
//  RNOpenXRCompositorLayer.h
//  Rayne-VR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENXRCOMPOSITORLAYER_H_
#define __RAYNE_OPENXRCOMPOSITORLAYER_H_

#include "RNOpenXR.h"
#include "RNVRCompositorLayer.h"

namespace RN
{
	class OpenXRSwapChain;
	class OpenXRWindow;
	class OpenXRCompositorLayerInternals;

	class OpenXRCompositorLayer : public VRCompositorLayer
	{
	public:
		friend OpenXRWindow;
		friend class OpenXRVulkanSwapChain;

		OXRAPI ~OpenXRCompositorLayer();

		OXRAPI void SetActive(bool active) final;
		OXRAPI void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) final;

		OXRAPI Vector2 GetSize() const final;
		OXRAPI size_t GetImageCount() const final;

		OXRAPI Framebuffer *GetFramebuffer() const final;
		OXRAPI Framebuffer *GetFramebuffer(uint8 image) const final;

	protected:
		OXRAPI OpenXRCompositorLayer(Type type, const Window::SwapChainDescriptor &descriptor, Vector2 resolution, bool supportFoveation, OpenXRWindow *window);

	private:
		void SetSessionActive(bool active);
		void UpdateForCurrentFrame(const OpenXRWindow *window);

		OpenXRCompositorLayerInternals *_internals;

		OpenXRSwapChain *_swapChain;
		bool _isActive;
		bool _isSessionActive;
		bool _shouldDisplay;

		RNDeclareMetaAPI(OpenXRCompositorLayer, OXRAPI)
	};
}


#endif /* __RAYNE_OPENXRCOMPOSITORLAYER_H_ */
