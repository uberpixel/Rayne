//
//  RNVulkanSwapChain.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanSwapChain.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanInternals.h"

namespace RN
{
	RNDefineMeta(VulkanSwapChain, Object)

	static VkFormat SwapChainFormatFromTextureFormat(Texture::Format format)
	{
		switch(format)
		{
		case Texture::Format::RGBA_8_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case Texture::Format::BGRA_8_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case Texture::Format::RGBA_8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::BGRA_8:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case Texture::Format::RGB_10_A_2:
			return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case Texture::Format::BGR_10_A_2:
			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}

	static Texture::Format TextureFormatFromSwapChainFormat(VkFormat format)
	{
		switch(format)
		{
		case VK_FORMAT_R8G8B8A8_SRGB:
			return Texture::Format::RGBA_8_SRGB;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return Texture::Format::BGRA_8_SRGB;
		case VK_FORMAT_R8G8B8A8_UNORM:
			return Texture::Format::RGBA_8;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return Texture::Format::BGRA_8;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			return Texture::Format::RGB_10_A_2;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
			return Texture::Format::BGR_10_A_2;
		default:
			return Texture::Format::Invalid;
		}
	}


#if RN_PLATFORM_WINDOWS
VulkanSwapChain::VulkanSwapChain(const Vector2& size, HWND hwnd, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor) :
		_hwnd(hwnd),
#elif RN_PLATFORM_ANDROID
VulkanSwapChain::VulkanSwapChain(const Vector2& size, ANativeWindow *window, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor) :
		_window(window),
#elif RN_PLATFORM_LINUX
VulkanSwapChain::VulkanSwapChain(const Vector2& size, xcb_window_t window, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor) :
		_window(window),
#else
VulkanSwapChain::VulkanSwapChain(const Vector2& size, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor) :
#endif
		_renderer(renderer),
		_frameIndex(0),
		_semaphoreIndex(0),
		_size(size),
		_descriptor(descriptor),
		_surface(VK_NULL_HANDLE),
		_swapchain(VK_NULL_HANDLE),
		_framebuffer(nullptr),
		_preFullscreenWindowSize(_size),
		_isFullscreen(false)
	{
		_device = _renderer->GetVulkanDevice()->GetDevice();

		CreateSurface();
		CreateSwapChain();
	}

	VulkanSwapChain::VulkanSwapChain() :
		_renderer(nullptr),
		_frameIndex(0),
		_semaphoreIndex(0),
		_size(Vector2()),
		_surface(VK_NULL_HANDLE),
		_swapchain(VK_NULL_HANDLE),
		_framebuffer(nullptr),
		_isFullscreen(false)
	{

	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		SafeRelease(_framebuffer);
	}

	void VulkanSwapChain::CreateSurface()
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();
		VulkanInstance *instance = _renderer->GetVulkanInstance();

#if RN_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hinstance = ::GetModuleHandle(nullptr);
		surfaceInfo.hwnd = _hwnd;

		RNVulkanValidate(vk::CreateWin32SurfaceKHR(instance->GetInstance(), &surfaceInfo, nullptr, &_surface));
#endif
#if RN_PLATFORM_LINUX
		VkXcbSurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.pNext = nullptr;
		surfaceInfo.flags = 0;
		surfaceInfo.connection = Kernel::GetSharedInstance()->GetXCBConnection();
		surfaceInfo.window = _window;

		RNVulkanValidate(vk::CreateXcbSurfaceKHR(instance->GetInstance(), &surfaceInfo, nullptr, &_surface));
#endif
#if RN_PLATFORM_ANDROID
		VkAndroidSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
		createInfo.window = _window;
		VkResult result = vk::CreateAndroidSurfaceKHR(instance->GetInstance(), &createInfo, nullptr, &_surface);
        RNVulkanValidate(result);
#endif

		VkBool32 surfaceSupported;
		vk::GetPhysicalDeviceSurfaceSupportKHR(device->GetPhysicalDevice(), 0, _surface, &surfaceSupported);

		RN_ASSERT(surfaceSupported == VK_TRUE, "VkSurface unsupported!");

		std::vector<VkSurfaceFormatKHR> formats;
		device->GetSurfaceFormats(_surface, formats);

		_format = formats.at(0);
		VkSurfaceFormatKHR fallbackFormat = _format;
		bool isSupportedFormat = false;
		VkFormat requestedFormat = SwapChainFormatFromTextureFormat(_descriptor.colorFormat);
		for(const VkSurfaceFormatKHR &format : formats)
		{
			if(format.format == requestedFormat)
			{
				_format = format;
				isSupportedFormat = true;
				break;
			}
			else if((format.format == VK_FORMAT_R8G8B8A8_SRGB || format.format == VK_FORMAT_B8G8R8A8_SRGB) && (_descriptor.colorFormat == Texture::Format::RGBA_8_SRGB || _descriptor.colorFormat == Texture::Format::BGRA_8_SRGB))
			{
				fallbackFormat = format;
			}
			else if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && (_descriptor.colorFormat == Texture::Format::RGBA_8 || _descriptor.colorFormat == Texture::Format::BGRA_8))
			{
				fallbackFormat = format;
			}
			else if ((format.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 || format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) && (_descriptor.colorFormat == Texture::Format::RGB_10_A_2 || _descriptor.colorFormat == Texture::Format::BGR_10_A_2))
			{
				fallbackFormat = format;
			}
		}

		if(!isSupportedFormat)
		{
			_format = fallbackFormat;
			_descriptor.colorFormat = TextureFormatFromSwapChainFormat(_format.format);
		}

		_extents.width = static_cast<uint32_t>(-1);
		_extents.height = static_cast<uint32_t>(-1);
	}

	void VulkanSwapChain::CreateSwapChain()
	{
		if(_framebuffer)
		{
			_framebuffer->WillUpdateSwapChain();
		}

		VulkanDevice *device = _renderer->GetVulkanDevice();

		VkSurfaceCapabilitiesKHR caps;
		RNVulkanValidate(vk::GetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), _surface, &caps));

		VkExtent2D extent = caps.currentExtent;

		if(extent.width == static_cast<uint32_t>(-1))
		{
			extent.width = static_cast<uint32_t>(_size.x);
			extent.height = static_cast<uint32_t>(_size.y);
		}

		extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, extent.width));
		extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, extent.height));

		if(_extents.width == extent.width && _extents.height == extent.height)
			return;

		uint32_t imageCount = std::max(caps.minImageCount, std::min(caps.maxImageCount, static_cast<uint32_t>(_descriptor.bufferCount)));

		assert(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		assert(caps.supportedTransforms & caps.currentTransform);
		assert(caps.supportedCompositeAlpha & (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR));

		VkCompositeAlphaFlagBitsKHR compositeAlpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		std::vector<VkPresentModeKHR> modes;
		device->GetPresentModes(_surface, modes);

		const bool useVSync = _descriptor.vsync;

		VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR; // FIFO is the only mode universally supported
		for(auto m : modes)
		{
			if(!useVSync && (m == VK_PRESENT_MODE_IMMEDIATE_KHR || m == VK_PRESENT_MODE_MAILBOX_KHR))
			{
				mode = m;
				break;
			}
		}

		VkSwapchainCreateInfoKHR swapchainInfo = {};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainInfo.surface = _surface;
		swapchainInfo.minImageCount = imageCount;
		swapchainInfo.imageFormat = _format.format;
		swapchainInfo.imageColorSpace = _format.colorSpace;
		swapchainInfo.imageExtent = extent;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.preTransform = caps.currentTransform;
		swapchainInfo.compositeAlpha = compositeAlpha;
		swapchainInfo.presentMode = mode;
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.oldSwapchain = _swapchain;
		_swapchain = nullptr;

#if RN_PLATFORM_WINDOWS
		VkSurfaceFullScreenExclusiveInfoEXT exclusiveFullscreenInfo = {};
		exclusiveFullscreenInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
		exclusiveFullscreenInfo.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
		swapchainInfo.pNext = &exclusiveFullscreenInfo;

		/*VkSurfaceFullScreenExclusiveWin32InfoEXT exclusiveFullscreenWin32Info = {};
		exclusiveFullscreenWin32Info.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
		exclusiveFullscreenWin32Info.hmonitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
		exclusiveFullscreenInfo.pNext = &exclusiveFullscreenWin32Info;*/
#endif

		RNVulkanValidate(vk::CreateSwapchainKHR(_device, &swapchainInfo, nullptr, &_swapchain));
		_extents = extent;
		_size.x = extent.width;
		_size.y = extent.height;

		// Destroy the old swapchain
		if(swapchainInfo.oldSwapchain)
		{
			RNVulkanValidate(vk::DeviceWaitIdle(_device));
			vk::DestroySwapchainKHR(_device, swapchainInfo.oldSwapchain, nullptr);
		}

		uint32_t count = _descriptor.bufferCount;
		vk::GetSwapchainImagesKHR(_device, _swapchain, &count, nullptr);
		vk::GetSwapchainImagesKHR(_device, _swapchain, &count, _colorBuffers);
		_descriptor.bufferCount = static_cast<uint8>(count);

		CreateSemaphores();

		if(!_framebuffer)
		{
			_framebuffer = new VulkanFramebuffer(_size, 1, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat, Texture::Format::Invalid);
		}
		else
		{
			_framebuffer->DidUpdateSwapChain(_size, 1, _descriptor.colorFormat, _descriptor.depthStencilFormat, Texture::Format::Invalid);
		}
	};

	void VulkanSwapChain::CreateSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = _presentSemaphores.size(); i < _descriptor.bufferCount; i++)
		{
			VkSemaphore presentSemaphore;
			VkSemaphore renderSemaphore;
			RNVulkanValidate(vk::CreateSemaphore(_renderer->GetVulkanDevice()->GetDevice(), &semaphoreInfo, nullptr, &presentSemaphore));
			RNVulkanValidate(vk::CreateSemaphore(_renderer->GetVulkanDevice()->GetDevice(), &semaphoreInfo, nullptr, &renderSemaphore));
			_presentSemaphores.push_back(presentSemaphore);
			_renderSemaphores.push_back(renderSemaphore);
		}
	}

	void VulkanSwapChain::ResizeSwapchain(const Vector2& size)
	{
		_newSize = size;
	}

	void VulkanSwapChain::SetFullscreen(bool fullscreen)
	{
#if RN_PLATFORM_WINDOWS
		//Those functions appear to be a windows thing!?
		VulkanDevice* device = _renderer->GetVulkanDevice();
		if(fullscreen)
		{
			WINDOWINFO windowInfo;
			windowInfo.cbSize = sizeof(WINDOWINFO);
			GetWindowInfo(_hwnd, &windowInfo);
			_preFullscreenWindowSize.x = std::abs(windowInfo.rcWindow.right - windowInfo.rcWindow.left);
			_preFullscreenWindowSize.y = std::abs(windowInfo.rcWindow.bottom - windowInfo.rcWindow.top);
			_preFullscreenWindowPosition.x = windowInfo.rcWindow.left;
			_preFullscreenWindowPosition.y = windowInfo.rcWindow.top;
			_preFullscreenWindowStyle = windowInfo.dwStyle;

			int width = GetSystemMetrics(SM_CXSCREEN);
			int height = GetSystemMetrics(SM_CYSCREEN);

			MONITORINFO info;
			info.cbSize = sizeof(MONITORINFO);
			HMONITOR monitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
			if(GetMonitorInfo(monitor, &info) != 0)
			{
				width = std::abs(info.rcMonitor.right - info.rcMonitor.left);
				height = std::abs(info.rcMonitor.bottom - info.rcMonitor.top);
			}

			SetWindowLongPtr(_hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
			SetWindowPos(_hwnd, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED);
			ShowCursor(false);

			//CreateSwapChain();

			//RNVulkanValidate(vk::AcquireFullScreenExclusiveModeEXT(device->GetDevice(), _swapchain));
		}
		else
		{
			//RNVulkanValidate(vk::ReleaseFullScreenExclusiveModeEXT(device->GetDevice(), _swapchain));

			SetWindowLongPtr(_hwnd, GWL_STYLE, _preFullscreenWindowStyle);
			SetWindowPos(_hwnd, HWND_TOP, _preFullscreenWindowPosition.x, _preFullscreenWindowPosition.y, _preFullscreenWindowSize.x, _preFullscreenWindowSize.y, SWP_FRAMECHANGED);
			ShowCursor(true);
		}

		_isFullscreen = fullscreen;
#endif
	}

	Vector2 VulkanSwapChain::GetSize() const
	{
		return _size;
	}

	VkImage VulkanSwapChain::GetVulkanColorBuffer(int i) const
	{
		return _colorBuffers[i];
	}

	VkImage VulkanSwapChain::GetVulkanDepthBuffer(int i) const
	{
		return nullptr;
	}

	VkImage VulkanSwapChain::GetVulkanFragmentDensityBuffer(int i, uint32 &width, uint32 &height) const
	{
		return nullptr;
	}

	void VulkanSwapChain::AcquireBackBuffer()
	{
		_semaphoreIndex += 1;
		_semaphoreIndex %= _descriptor.bufferCount;
		VkResult result = vk::AcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _presentSemaphores[_semaphoreIndex], VK_NULL_HANDLE, &_frameIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            CreateSwapChain();
        }
        else
        {
            RNVulkanValidate(result);
        }

		//Update the swap chain and frame buffer size if the window size changed
/*		if(_newSize.GetLength() > 0.001f)
		{
			if(_size.GetSquaredDistance(_newSize) > 0.001f)
			{
				_size = _newSize;

				//Wait for rendering to all currently bound buffers is finished
				UINT completedFenceValue = _fence->GetCompletedValue();
				if(completedFenceValue < _fenceValues[_frameIndex])
				{
					_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
					WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
				}

				//Do the actual resize and notify the framebuffer about it
				CreateSwapChain();

				_frameIndex = _swapChain->GetCurrentBackBufferIndex();
			}

			_newSize = Vector2();
		}*/
	}

	void VulkanSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier postPresentBarrier = {};
		postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		postPresentBarrier.pNext = NULL;
		postPresentBarrier.srcAccessMask = 0;
		postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		postPresentBarrier.image = static_cast<VulkanTexture *>(_framebuffer->GetColorTexture(_frameIndex))->GetVulkanImage();
		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &postPresentBarrier);
	}

	void VulkanSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{
		// Add a present memory barrier to the end of the command buffer
		// This will transform the frame buffer color attachment to a
		// new layout for presenting it to the windowing system integration
		VkImageMemoryBarrier prePresentBarrier = {};
		prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		prePresentBarrier.pNext = NULL;
		prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		prePresentBarrier.dstAccessMask = 0;
		prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		prePresentBarrier.image = static_cast<VulkanTexture *>(_framebuffer->GetColorTexture(_frameIndex))->GetVulkanImage();

		vk::CmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &prePresentBarrier);
	}

	void VulkanSwapChain::PresentBackBuffer(VkQueue queue)
	{
		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &_renderSemaphores[_semaphoreIndex];
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &_swapchain;
		present_info.pImageIndices = &_frameIndex;

		VkResult result = vk::QueuePresentKHR(queue, &present_info);
		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			CreateSwapChain();
		}
		else
		{
			RNVulkanValidate(result);
		}
	}
}
