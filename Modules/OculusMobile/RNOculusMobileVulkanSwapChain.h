//
//  RNOculusMobileVulkanSwapChain.h
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_
#define __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_

#include "RNVulkanRenderer.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanSwapChain.h"

#include "VrApi.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "RNOculusMobile.h"

namespace RN
{
	class OculusMobileVulkanSwapChain : public VulkanSwapChain
	{
	public:
		friend class OculusMobileWindow;

		OVRAPI ~OculusMobileVulkanSwapChain();

		VKAPI void AcquireBackBuffer() final;
		VKAPI void Prepare(VkCommandBuffer commandBuffer) final;
		VKAPI void Finalize(VkCommandBuffer commandBuffer) final;
		VKAPI void PresentBackBuffer(VkQueue queue) final;

		VKAPI VkImage GetVulkanColorBuffer(int i) const final;
		VKAPI VkImage GetVulkanDepthBuffer(int i) const final;

		OVRAPI void UpdatePredictedPose();
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; }

	private:
		OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor);
		const String *GetHMDInfoDescription() const;
		OVRAPI void SetProjection(float m22, float m23, float m32);
		ovrMatrix4f GetTanAngleMatrixForEye(uint8 eye);

		void CreateSharedVulkanImage();

		void CreateEGLContext();
		void DestroyEGLContext();

		void CreateGLESFramebuffer();
		void DestroyGLESFramebuffer();

		void UpdateVRMode();

		ovrJava _java;
		ovrTextureSwapChain *_colorSwapChain;
		ovrMobile *_session;
		uint32 _swapChainBufferCount;
		uint32 _actualFrameIndex;
		double _predictedDisplayTime;
		ovrTracking2 _hmdState;
		Vector2 _eyeRenderSize;

		ANativeWindow *_nativeWindow;
		EGLDisplay _eglDisplay;
		EGLConfig _eglConfig;
		EGLSurface _eglTinySurface;
		EGLContext _eglContext;

		GLuint *_glesFramebuffers;
		GLuint _glTexture;

		VkImage _vulkanTexture;

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OculusMobileVulkanSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_ */
