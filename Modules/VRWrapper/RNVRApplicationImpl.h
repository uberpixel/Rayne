//
//  RNVRApplication.cpp
//  Rayne-VR
//
//  Copyright 2020 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNVRApplication.h"

namespace RN
{
//	RNDefineMeta(VRApplication, Application)

	VRApplication::VRApplication() : _vrWindow(nullptr)
	{
		
	}

	VRApplication::~VRApplication()
	{
		SafeRelease(_vrWindow);
	}

	void VRApplication::SetupVR()
	{
		if(_vrWindow)
			return;

	#if RN_PLATFORM_ANDROID
		_vrWindow = new RN::OculusMobileWindow();
	#else
		bool wantsOpenVR = RN::Kernel::GetSharedInstance()->GetArguments().HasArgument("openvr", 0);
	#if RN_PLATFORM_WINDOWS
		bool wantsOculus = RN::Kernel::GetSharedInstance()->GetArguments().HasArgument("oculusvr", 0);

		#ifndef BUILD_FOR_OCULUS
		if(!wantsOpenVR && (!RN::OpenVRWindow::IsSteamVRRunning() || wantsOculus))
		#endif
		{
			if(RN::OculusWindow::GetAvailability() == RN::VRWindow::HMD || wantsOculus)
			{
				_vrWindow = new RN::OculusWindow();
				return;
			}
		}
	#endif

	#ifndef BUILD_FOR_OCULUS
		if(RN::OpenVRWindow::GetAvailability() == RN::VRWindow::HMD || wantsOpenVR)
		{
			_vrWindow = new RN::OpenVRWindow();
			return;
		}
	#endif
	#endif
	}

	RendererDescriptor *VRApplication::GetPreferredRenderer() const
	{
		if(!_vrWindow) return RN::Application::GetPreferredRenderer();

		RN::Array *instanceExtensions = _vrWindow->GetRequiredVulkanInstanceExtensions();
		RN::Dictionary *parameters = nullptr;
		if(instanceExtensions)
		{
			parameters = new RN::Dictionary();
			parameters->SetObjectForKey(instanceExtensions, RNCSTR("instanceextensions"));
		}

	#if RN_PLATFORM_ANDROID
		RN::RendererDescriptor *descriptor = RN::RendererDescriptor::GetRendererWithIdentifier(RNCSTR("net.uberpixel.rendering.vulkan"), parameters);
	#else
		RN::RendererDescriptor *descriptor = RN::RendererDescriptor::GetPreferredRenderer(parameters);
	#endif
		return descriptor;
	}

	RN::RenderingDevice *VRApplication::GetPreferredRenderingDevice(RN::RendererDescriptor *descriptor, const RN::Array *devices) const
	{
		if(!_vrWindow) return RN::Application::GetPreferredRenderingDevice(descriptor, devices);

		RN::RenderingDevice *preferred = nullptr;

		RN::RenderingDevice *vrDevice = _vrWindow->GetOutputDevice(descriptor);
		if(vrDevice)
		{
			devices->Enumerate<RN::RenderingDevice>([&](RN::RenderingDevice *device, size_t index, bool &stop) {
				if(vrDevice->IsEqual(device))
				{
					preferred = device;
					stop = true;
				}
			});
		}

		if(!preferred)
			preferred = RN::Application::GetPreferredRenderingDevice(descriptor, devices);

		RN::Array *deviceExtensions = _vrWindow->GetRequiredVulkanDeviceExtensions(descriptor, preferred);
		preferred->SetExtensions(deviceExtensions);

		return preferred;
	}

	void VRApplication::WillFinishLaunching(RN::Kernel *kernel)
	{
		RN::Application::WillFinishLaunching(kernel);
		
		if(!RN::Kernel::GetSharedInstance()->GetArguments().HasArgument("pancake", '2d'))
		{
			SetupVR();
		}
	}
	
	void VRApplication::DidFinishLaunching(RN::Kernel *kernel)
	{
		RN::Application::DidFinishLaunching(kernel);
		
		if(_vrWindow)
		{
			RN::Renderer *renderer = RN::Renderer::GetActiveRenderer();
			renderer->SetMainWindow(_vrWindow);

			RN::Window::SwapChainDescriptor swapChainDescriptor;
#if RN_PLATFORM_WINDOWS
			if(_vrWindow->IsKindOfClass(RN::OculusWindow::GetMetaClass()))
			{
				swapChainDescriptor.depthStencilFormat = RN::Texture::Format::Depth_32F;
				swapChainDescriptor.colorFormat = RN::Texture::Format::RGBA_16F;
			}
#endif
			_vrWindow->StartRendering(swapChainDescriptor);
		}
/*		else
		{
			RN::Window::SwapChainDescriptor swapchainDescriptor(RN::Texture::Format::RGBA_16F, RN::Texture::Format::Depth_32F);
			swapchainDescriptor.vsync = false;
			
			RN::Vector2 windowSize(1024, 768);
			RN::Dictionary *resolutionDictionary = RN::Settings::GetSharedInstance()->GetEntryForKey<RN::Dictionary>(RNCSTR("RNResolution"));
			if(resolutionDictionary)
			{
				RN::Number *widthNumber = resolutionDictionary->GetObjectForKey<RN::Number>(RNCSTR("width"));
				RN::Number *heightNumber = resolutionDictionary->GetObjectForKey<RN::Number>(RNCSTR("height"));
				if(widthNumber) windowSize.x = widthNumber->GetInt32Value();
				if(heightNumber) windowSize.y = heightNumber->GetInt32Value();
			}
			
			RN::Window *previewWindow = RN::Renderer::GetActiveRenderer()->CreateAWindow(windowSize, RN::Screen::GetMainScreen(), swapchainDescriptor);
			previewWindow->Show();
		}*/
	}
}
