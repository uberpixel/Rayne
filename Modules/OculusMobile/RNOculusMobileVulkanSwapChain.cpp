//
//  RNOculusMobileVulkanSwapChain.cpp
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusMobileVulkanSwapChain.h"
#include "RNVulkanInternals.h"

#include <sys/prctl.h>					// for prctl( PR_SET_NAME )
#include <android/log.h>
#include <android/window.h>				// for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h>	// for native window JNI
#include <android_native_app_glue.h>

#include "VrApi_Helpers.h"
#include "VrApi_SystemUtils.h"

#define GL_HANDLE_TYPE_OPAQUE_FD_EXT 0x9586
typedef void (GL_APIENTRY* PFNGLCREATEMEMORYOBJECTSEXTPROC) (GLsizei n, GLuint *memoryObjects);
typedef void (GL_APIENTRY* PFNGLIMPORTMEMORYFDEXTPROC) (GLuint memory, GLuint64 size, GLenum handleType, GLint fd);
typedef void (GL_APIENTRY* PFNGLTEXSTORAGEMEM2DEXTPROC) (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);

namespace RN
{
	RNDefineMeta(OculusMobileVulkanSwapChain, VulkanSwapChain)

	const uint32 OculusMobileVulkanSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, doesn't appear to be enough though...)

	OculusMobileVulkanSwapChain::OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor) : _session(nullptr), _actualFrameIndex(0), _predictedDisplayTime(0.0), _nativeWindow(nullptr), _eglDisplay(0)
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

		// Note that AttachCurrentThread will reset the thread name.
		prctl(PR_SET_NAME, (long)"Rayne::Main", 0, 0, 0);

		const ovrInitParms initParms = vrapi_DefaultInitParms(&_java);
		int32_t initResult = vrapi_Initialize(&initParms);
		if(initResult != VRAPI_INITIALIZE_SUCCESS)
		{
			return;
		}

		RNInfo(GetHMDInfoDescription());

		_eyeRenderSize.x = vrapi_GetSystemPropertyInt(&_java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
		_eyeRenderSize.y = vrapi_GetSystemPropertyInt(&_java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);

		_size.x = _eyeRenderSize.x * 2 + kEyePadding;
		_size.y = _eyeRenderSize.y;

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

		NotificationManager::GetSharedInstance()->AddSubscriber(kRNAndroidWindowDidChange, [this](Notification *notification) {
        			if(notification->GetName()->IsEqual(kRNAndroidWindowDidChange))
        			{
        				_nativeWindow = nullptr;
        				UpdateVRMode();
        			}
        		}, this);

		//Create and set egl context
		CreateEGLContext();

		//Create opengl swap chain with 3 buffers, but only expose the vulkan texture to renderer as swapchain with only 1 buffer
		_descriptor.bufferCount = 3;
		_colorSwapChain = vrapi_CreateTextureSwapChain2(VRAPI_TEXTURE_TYPE_2D, textureFormat, _size.x, _size.y, 1, _descriptor.bufferCount);
		_swapChainBufferCount = vrapi_GetTextureSwapChainLength(_colorSwapChain);
		_descriptor.bufferCount = 1;

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for(size_t i = 0; i < _swapChainBufferCount; i++)
		{
			VkSemaphore presentSemaphore;
			VkSemaphore renderSemaphore;
			RNVulkanValidate(vk::CreateSemaphore(_device, &semaphoreInfo, nullptr, &presentSemaphore));
			RNVulkanValidate(vk::CreateSemaphore(_device, &semaphoreInfo, nullptr, &renderSemaphore));
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}

		CreateSharedVulkanImage();
		_framebuffer = new VulkanFramebuffer(_size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);

		CreateGLESFramebuffer();
	}

	OculusMobileVulkanSwapChain::~OculusMobileVulkanSwapChain()
	{
		NotificationManager::GetSharedInstance()->RemoveSubscriber(kRNAndroidWindowDidChange, this);

		if(_session)
		{
			vrapi_LeaveVrMode(_session);
		}

		DestroyGLESFramebuffer();
		DestroyEGLContext();

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

	void OculusMobileVulkanSwapChain::UpdateVRMode()
    {
    	if(!_nativeWindow)
    	{
    		if(!_session)
    		{
    			android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
                _nativeWindow = app->window;

    			ovrModeParms params = vrapi_DefaultModeParms(&_java);
				params.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
				params.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
				params.Display = (size_t)_eglDisplay;
				params.WindowSurface = (size_t)_nativeWindow;
				params.ShareContext = (size_t)_eglContext;
				_session = vrapi_EnterVrMode(&params);

    			// If entering VR mode failed then the ANativeWindow was not valid.
    			if(!_session)
    			{
    				RNDebug(RNCSTR("Invalid ANativeWindow!"));
    				_nativeWindow = nullptr;
    			}

    			// Set performance parameters once we have entered VR mode and have a valid ovrMobile.
    			if(_session)
    			{
//    				vrapi_SetClockLevels(app->Ovr, app->CpuLevel, app->GpuLevel);
 //   				vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, app->MainThreadTid);
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
		_semaphoreIndex %= _swapChainBufferCount;

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
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _glesFramebuffers[_semaphoreIndex]);

		glViewport(0, 0, _size.x, _size.y);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_SCISSOR_TEST);
		glScissor(0, 0, _eyeRenderSize.x, _size.y);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		glScissor(_eyeRenderSize.x + kEyePadding, 0, _eyeRenderSize.x, _size.y);
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);

        glFlush();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
/*				gameLayer.Textures[eye].TextureRect.x = (eye * (_eyeRenderSize.x + kEyePadding))/_size.x;
				gameLayer.Textures[eye].TextureRect.y = 0.0f;
				gameLayer.Textures[eye].TextureRect.width = _eyeRenderSize.x/_size.x;
				gameLayer.Textures[eye].TextureRect.height = 1.0f;*/
			}
			gameLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

			const ovrLayerHeader2 * layers[] = { &gameLayer.Header };

			ovrSubmitFrameDescription2 frameDescription = {};
			frameDescription.Flags = 0;
			frameDescription.SwapInterval = 1;
			frameDescription.FrameIndex = _actualFrameIndex;
	//		frameDescription.CompletionFence = completionFence;
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
		return _vulkanTexture;

//		unsigned int textureHandle = vrapi_GetTextureSwapChainHandle(_colorSwapChain, i);
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
    		{ 0.0f, 0.5f * projection->M[1][1], 0.5f * projection->M[1][2] - 0.5f, 0.0f },
    		{ 0.0f, 0.0f, -1.0f, 0.0f },
    		// Store the values to convert a clip-Z to a linear depth in the unused matrix elements.
    		{ projection->M[2][2], projection->M[2][3], projection->M[3][2], 1.0f }
    	} };
    	return tanAngleMatrix;
    }

	static const char *EglErrorString(const EGLint error)
    {
    	switch(error)
    	{
    		case EGL_SUCCESS:				return "EGL_SUCCESS";
    		case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
    		case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
    		case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
    		case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
    		case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
    		case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
    		case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
    		case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
    		case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
    		case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
    		case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
    		case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
    		case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
    		case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
    		default:						return "unknown";
    	}
    }

	void OculusMobileVulkanSwapChain::CreateEGLContext()
    {
    	if(_eglDisplay != 0)
    	{
    		return;
    	}

    	_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    	eglInitialize(_eglDisplay, nullptr, nullptr);

    	// Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    	// flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    	// settings, and that is completely wasted for our warp target.
    	const int MAX_CONFIGS = 1024;
    	EGLConfig configs[MAX_CONFIGS];
    	EGLint numConfigs = 0;
    	if(eglGetConfigs(_eglDisplay, configs, MAX_CONFIGS, &numConfigs ) == EGL_FALSE)
    	{
    		RNDebug(RNCSTR("eglGetConfigs() failed: ") << EglErrorString(eglGetError()));
    		return;
    	}
    	const EGLint configAttribs[] =
    	{
    		EGL_RED_SIZE,		8,
    		EGL_GREEN_SIZE,		8,
    		EGL_BLUE_SIZE,		8,
    		EGL_ALPHA_SIZE,		8, // need alpha for the multi-pass timewarp compositor
    		EGL_DEPTH_SIZE,		0,
    		EGL_STENCIL_SIZE,	0,
    		EGL_SAMPLES,		0,
    		EGL_NONE
    	};
    	_eglConfig = 0;
    	for(int i = 0; i < numConfigs; i++)
    	{
    		EGLint value = 0;

    		eglGetConfigAttrib(_eglDisplay, configs[i], EGL_RENDERABLE_TYPE, &value);
    		if((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
    		{
    			continue;
    		}

    		// The pbuffer config also needs to be compatible with normal window rendering
    		// so it can share textures with the window context.
    		eglGetConfigAttrib(_eglDisplay, configs[i], EGL_SURFACE_TYPE, &value);
    		if((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT))
    		{
    			continue;
    		}

    		int	j = 0;
    		for(; configAttribs[j] != EGL_NONE; j += 2)
    		{
    			eglGetConfigAttrib(_eglDisplay, configs[i], configAttribs[j], &value);
    			if(value != configAttribs[j + 1])
    			{
    				break;
    			}
    		}
    		if(configAttribs[j] == EGL_NONE)
    		{
    			_eglConfig = configs[i];
    			break;
    		}
    	}
    	if(_eglConfig == 0)
    	{
    		RNDebug(RNCSTR("eglChooseConfig() failed: ") << EglErrorString(eglGetError()));
    		return;
    	}

    	EGLint contextAttribs[] =
    	{
    		EGL_CONTEXT_CLIENT_VERSION, 3,
    		EGL_NONE
    	};
    	_eglContext = eglCreateContext(_eglDisplay, _eglConfig, EGL_NO_CONTEXT, contextAttribs);
    	if(_eglContext == EGL_NO_CONTEXT)
    	{
    		RNDebug(RNCSTR("eglCreateContext() failed: ") << EglErrorString(eglGetError()));
    		return;
    	}

    	const EGLint surfaceAttribs[] =
    	{
    		EGL_WIDTH, 16,
    		EGL_HEIGHT, 16,
    		EGL_NONE
    	};
    	_eglTinySurface = eglCreatePbufferSurface(_eglDisplay, _eglConfig, surfaceAttribs);
    	if(_eglTinySurface == EGL_NO_SURFACE)
    	{
    		RNDebug(RNCSTR("eglCreatePbufferSurface() failed: ") << EglErrorString(eglGetError()));
    		eglDestroyContext(_eglDisplay, _eglContext);
    		_eglContext = EGL_NO_CONTEXT;
    		return;
    	}

    	if(eglMakeCurrent(_eglDisplay, _eglTinySurface, _eglTinySurface, _eglContext) == EGL_FALSE)
    	{
    		RNDebug(RNCSTR("eglMakeCurrent() failed: %s") << EglErrorString(eglGetError()));
    		eglDestroySurface(_eglDisplay, _eglTinySurface);
    		eglDestroyContext(_eglDisplay, _eglContext);
    		_eglContext = EGL_NO_CONTEXT;
    		return;
    	}
    }

    void OculusMobileVulkanSwapChain::DestroyEGLContext()
    {
    	if(_eglDisplay != 0)
    	{
    		eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    	}
    	if(_eglContext != EGL_NO_CONTEXT)
    	{
    		eglDestroyContext(_eglDisplay, _eglContext);
    		_eglContext = EGL_NO_CONTEXT;
    	}
    	if(_eglTinySurface != EGL_NO_SURFACE)
    	{
    		eglDestroySurface(_eglDisplay, _eglTinySurface);
    		_eglTinySurface = EGL_NO_SURFACE;
    	}
    	if(_eglDisplay != 0)
    	{
    		eglTerminate(_eglDisplay);
    		_eglDisplay = 0;
    	}
    }

#ifdef CHECK_GL_ERRORS
    static const char *GlErrorString(GLenum error)
    {
    	switch(error)
    	{
    		case GL_NO_ERROR:						return "GL_NO_ERROR";
    		case GL_INVALID_ENUM:					return "GL_INVALID_ENUM";
    		case GL_INVALID_VALUE:					return "GL_INVALID_VALUE";
    		case GL_INVALID_OPERATION:				return "GL_INVALID_OPERATION";
    		case GL_INVALID_FRAMEBUFFER_OPERATION:	return "GL_INVALID_FRAMEBUFFER_OPERATION";
    		case GL_OUT_OF_MEMORY:					return "GL_OUT_OF_MEMORY";
    		default: return "unknown";
    	}
    }

    static void GLCheckErrors(int line)
    {
    	for(int i = 0; i < 10; i++)
    	{
    		const GLenum error = glGetError();
    		if(error == GL_NO_ERROR)
    		{
    			break;
    		}
    		RNDebug(RNCSTR("GL error: ") << GlErrorString(error));
    	}
    }

    #define GL(func) func; GLCheckErrors( __LINE__ );
#else // CHECK_GL_ERRORS
    #define GL(func) func;
#endif // CHECK_GL_ERRORS

	static const char *GlFrameBufferStatusString(GLenum status)
	{
		switch(status)
		{
			case GL_FRAMEBUFFER_UNDEFINED:						return "GL_FRAMEBUFFER_UNDEFINED";
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			case GL_FRAMEBUFFER_UNSUPPORTED:					return "GL_FRAMEBUFFER_UNSUPPORTED";
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
			default:											return "unknown";
		}
	}

    void OculusMobileVulkanSwapChain::CreateGLESFramebuffer()
    {
/*    	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
    		(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress( "glRenderbufferStorageMultisampleEXT" );
    	PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
    		(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress( "glFramebufferTexture2DMultisampleEXT" );

    	PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
    		(PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC) eglGetProcAddress( "glFramebufferTextureMultiviewOVR" );
    	PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
    		(PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC) eglGetProcAddress( "glFramebufferTextureMultisampleMultiviewOVR" );
    */

		_glesFramebuffers = new GLuint[_swapChainBufferCount];

    	for(int i = 0; i < _swapChainBufferCount; i++)
    	{
    		// Create the color buffer texture.
    		const GLuint colorTexture = vrapi_GetTextureSwapChainHandle(_colorSwapChain, i);
    		GLenum colorTextureTarget = GL_TEXTURE_2D;
    		GL(glBindTexture(colorTextureTarget, colorTexture));
/*    		if(glExtensions.EXT_texture_border_clamp)
    		{
    			GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ) );
    			GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ) );
    			GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    			GL( glTexParameterfv( colorTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor ) );
    		}
    		else
    		{*/
    			// Just clamp to edge. However, this requires manually clearing the border
    			// around the layer to clear the edge texels.
    			GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    			GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
//    		}
    		GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    		GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    		GL(glBindTexture(colorTextureTarget, 0));

			// Create the frame buffer.
			GL(glGenFramebuffers(1, &_glesFramebuffers[i]));
			GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _glesFramebuffers[i]));
			GL(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
			GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
			GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
			if(renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
			{
				RNDebug(RNCSTR("Incomplete frame buffer object: ") << GlFrameBufferStatusString(renderFramebufferStatus));
				return;
			}
    	}
    }

    void OculusMobileVulkanSwapChain::DestroyGLESFramebuffer()
    {
    	GL(glDeleteFramebuffers(_swapChainBufferCount, _glesFramebuffers));
		delete[] _glesFramebuffers;
    }

    void OculusMobileVulkanSwapChain::CreateSharedVulkanImage()
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();

		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VulkanTexture::VulkanImageFormatFromTextureFormat(_descriptor.colorFormat);
		imageInfo.extent = { static_cast<uint32>(_size.x/4), static_cast<uint32>(_size.y/4), 1 };
		imageInfo.arrayLayers = 1;
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;//VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = static_cast<VkSampleCountFlagBits>(1);
		imageInfo.usage = /*VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.flags = 0;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.mipLevels = 1;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

		RNVulkanValidate(vk::CreateImage(device->GetDevice(), &imageInfo, _renderer->GetAllocatorCallback(), &_vulkanTexture));


		VkMemoryRequirements requirements;
		vk::GetImageMemoryRequirements(device->GetDevice(), _vulkanTexture, &requirements);

		VkExportMemoryAllocateInfo exportAllocInfo;
		exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
		exportAllocInfo.pNext = nullptr;
		exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = &exportAllocInfo;
		allocateInfo.allocationSize = requirements.size;
		device->GetMemoryWithType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocateInfo.memoryTypeIndex);

		VkDeviceMemory memory;
		RNVulkanValidate(vk::AllocateMemory(device->GetDevice(), &allocateInfo, _renderer->GetAllocatorCallback(), &memory));
		RNVulkanValidate(vk::BindImageMemory(device->GetDevice(), _vulkanTexture, memory, 0));

		VkMemoryGetFdInfoKHR memoryHandleInfo;
		memoryHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
		memoryHandleInfo.pNext = nullptr;
		memoryHandleInfo.memory = memory;
		memoryHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

		GLint handle = -1;
		PFN_vkGetMemoryFdKHR GetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vk::GetDeviceProcAddr(device->GetDevice(), "vkGetMemoryFdKHR"));
		RNVulkanValidate(GetMemoryFdKHR(device->GetDevice(), &memoryHandleInfo, &handle));

		VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
		commandBuffer->Begin();
		VulkanTexture::SetImageLayout(commandBuffer->GetCommandBuffer(), _vulkanTexture, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VulkanTexture::BarrierIntent::RenderTarget);
		commandBuffer->End();
		_renderer->SubmitCommandBuffer(commandBuffer);

		if(handle >= 0)
		{
			PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT = reinterpret_cast<PFNGLCREATEMEMORYOBJECTSEXTPROC>(eglGetProcAddress("glCreateMemoryObjectsEXT"));
			PFNGLIMPORTMEMORYFDEXTPROC glImportMemoryFdEXT = reinterpret_cast<PFNGLIMPORTMEMORYFDEXTPROC>(eglGetProcAddress("glImportMemoryFdEXT"));
			PFNGLTEXSTORAGEMEM2DEXTPROC glTexStorageMem2DEXT = reinterpret_cast<PFNGLTEXSTORAGEMEM2DEXTPROC>(eglGetProcAddress("glTexStorageMem2DEXT"));

			GLuint mem;
			GL(glCreateMemoryObjectsEXT(1, &mem));
			GL(glImportMemoryFdEXT(mem, requirements.size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, handle));

			GL(glGenTextures(GL_TEXTURE_2D, &_glTexture));
			GL(glTexStorageMem2DEXT(_glTexture, 1, GL_RGBA8, _size.x, _size.y, mem, 0));
		}
	}
}
