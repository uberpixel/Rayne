//
//  RNVulkanWindow.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanWindow.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	static std::once_flag __windowClassToken;
	static WNDCLASSEXW __windowClass;

	RNDefineMeta(VulkanWindow, Window);

	VulkanWindow::VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer) :
		Window(screen),
		_renderer(renderer),
		_surface(VK_NULL_HANDLE),
		_swapchain(VK_NULL_HANDLE),
		_activeBackBuffer(nullptr)
	{
		HINSTANCE hInstance = ::GetModuleHandle(nullptr);

		std::call_once(__windowClassToken, [&] {

			__windowClass = { 0 };
			__windowClass.cbSize = sizeof(WNDCLASSEX);
			__windowClass.style = CS_HREDRAW | CS_VREDRAW;
			__windowClass.lpfnWndProc = &DefWindowProcW;
			__windowClass.hInstance = hInstance;
			__windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			__windowClass.lpszClassName = L"RNVulkanWindowClass";

			::RegisterClassExW(&__windowClass);

		});

		// Create the window
		const DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW;

		RECT windowRect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
		::AdjustWindowRect(&windowRect, style, FALSE);

		_hwnd = ::CreateWindowExW(0, L"RNVulkanWindowClass", L"", style, 300, 300, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, this);

		::SetForegroundWindow(_hwnd);

		// Create the back buffers
		VkDevice device = renderer->GetVulkanDevice()->GetDevice();

		for(size_t i = 0; i < kRNVulkanRenderStages; i ++)
		{
			VulkanBackBuffer *buffer = new VulkanBackBuffer(device);
			_backBuffers.push(buffer);
		}

		// Create the swap chain
		InitializeSurface();
		ResizeSwapchain(size);
	}

	VulkanWindow::~VulkanWindow()
	{
		::DestroyWindow(_hwnd);
	}

	void VulkanWindow::InitializeSurface()
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();
		VulkanInstance *instance = _renderer->GetVulkanInstance();

		VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hinstance = ::GetModuleHandle(nullptr);
		surfaceInfo.hwnd = _hwnd;

		RNVulkanValidate(vk::CreateWin32SurfaceKHR(instance->GetInstance(), &surfaceInfo, nullptr, &_surface));

		VkBool32 surfaceSupported;
		vk::GetPhysicalDeviceSurfaceSupportKHR(device->GetPhysicalDevice(), 0, _surface, &surfaceSupported);

		RN_ASSERT(surfaceSupported == VK_TRUE, "VkSurface unsupported!");

		std::vector<VkSurfaceFormatKHR> formats;
		device->GetSurfaceFormats(_surface, formats);

		_format = formats.at(0);

		_extents.width = static_cast<uint32_t>(-1);
		_extents.height = static_cast<uint32_t>(-1);
	}

	void VulkanWindow::ResizeSwapchain(const Vector2 &size)
	{
		VulkanDevice *device = _renderer->GetVulkanDevice();

		VkSurfaceCapabilitiesKHR caps;
		RNVulkanValidate(vk::GetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), _surface, &caps));

		VkExtent2D extent = caps.currentExtent;

		if(extent.width == static_cast<uint32_t>(-1))
		{
			extent.width = static_cast<uint32_t>(size.x);
			extent.height = static_cast<uint32_t>(size.y);
		}

		extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, extent.width));
		extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, extent.height));

		if(_extents.width == extent.width && _extents.height == extent.height)
			return;

		uint32_t imageCount = std::max(caps.minImageCount, std::min(caps.maxImageCount, static_cast<uint32_t>(1)));

		assert(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		assert(caps.supportedTransforms & caps.currentTransform);
		assert(caps.supportedCompositeAlpha & (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR));

		VkCompositeAlphaFlagBitsKHR compositeAlpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		std::vector<VkPresentModeKHR> modes;
		device->GetPresentModes(_surface, modes);

		const bool useVSync = true;

		VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR; // FIFO is the only mode universally supported
		for(auto m : modes)
		{
			if((useVSync && m == VK_PRESENT_MODE_MAILBOX_KHR) || (!useVSync && m == VK_PRESENT_MODE_IMMEDIATE_KHR))
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
		swapchainInfo.preTransform = caps.currentTransform;
		swapchainInfo.compositeAlpha = compositeAlpha;
		swapchainInfo.presentMode = mode;
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.oldSwapchain = _swapchain;

		std::vector<uint32_t> queueFamilies(1, device->GetGameQueue());
		if(device->GetGameQueue() != device->GetPresentQueue())
		{
			queueFamilies.push_back(device->GetPresentQueue());

			swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size());
			swapchainInfo.pQueueFamilyIndices = queueFamilies.data();
		}
		else
		{
			swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		RNVulkanValidate(vk::CreateSwapchainKHR(device->GetDevice(), &swapchainInfo, nullptr, &_swapchain));
		_extents = extent;

		// destroy the old swapchain
		if(swapchainInfo.oldSwapchain != VK_NULL_HANDLE)
		{
			RNVulkanValidate(vk::DeviceWaitIdle(device->GetDevice()));
			vk::DestroySwapchainKHR(device->GetDevice(), swapchainInfo.oldSwapchain, nullptr);
		}
	}

	void VulkanWindow::AcquireBackBuffer()
	{
		_activeBackBuffer = _backBuffers.front();
		_activeBackBuffer->WaitForPresentFence();
		_activeBackBuffer->AcquireNextImage(_swapchain);

		_backBuffers.pop();
	}

	void VulkanWindow::PresentBackBuffer()
	{
		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = _activeBackBuffer->GetRenderSemaphore();
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &_swapchain;
		present_info.pImageIndices = _activeBackBuffer->GetImageIndex();

		RNVulkanValidate(vk::QueuePresentKHR(_renderer->GetPresentQueue(), &present_info));
		RNVulkanValidate(vk::QueueSubmit(_renderer->GetPresentQueue(), 0, nullptr, _activeBackBuffer->GetPresentFence()));

		_backBuffers.push(_activeBackBuffer);
	}


	void VulkanWindow::SetTitle(const String *title)
	{
		char *text = title->GetUTF8String();
		wchar_t *wtext = new wchar_t[strlen(text) + 1];
		mbstowcs(wtext, text, strlen(text) + 1);//Plus null
		LPWSTR ptr = wtext;

		::SetWindowTextW(_hwnd, ptr);

		delete[] wtext;
	}

	Screen *VulkanWindow::GetScreen()
	{
		HMONITOR monitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTOPRIMARY);

		Screen *result = nullptr;
		Array *screens = Screen::GetScreens();

		screens->Enumerate<Screen>([&](Screen *screen, size_t index, bool &stop) {

			if(screen->GetHMONITOR() == monitor)
			{
				result = screen;
				stop = true;
			}

		});

		return result;
	}

	void VulkanWindow::Show()
	{
		::ShowWindow(_hwnd, SW_SHOW);
	}

	void VulkanWindow::Hide()
	{
		::ShowWindow(_hwnd, SW_HIDE);
	}

	Vector2 VulkanWindow::GetSize() const
	{
		RECT windowRect;
		::GetClientRect(_hwnd, &windowRect);
		return Vector2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}
}
