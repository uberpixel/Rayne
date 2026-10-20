//
//  RNCaptureWindowsSwapChain.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureWindowsSwapChain.h"

#if RN_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <RNVulkanDevice.h>
	#include <RNVulkanRenderer.h>

	#include <unknwn.h>
	#include <windows.h>

	#ifndef far
		#define far
		#define RN_CAPTURE_RESTORE_WINDOWS_FAR_MACRO
	#endif
	#include <d3d10_1.h>
	#include <d3d11_1.h>
	#include <dxgi1_6.h>
	#ifdef RN_CAPTURE_RESTORE_WINDOWS_FAR_MACRO
		#undef far
		#undef RN_CAPTURE_RESTORE_WINDOWS_FAR_MACRO
	#endif
	#include <wrl/client.h>

namespace RN
{
	using Microsoft::WRL::ComPtr;

	struct CaptureWindowsSwapChain::Internals
	{
		struct QueuedFrame
		{
			int64 presentationTimeUs = 0;
			int64 durationUs = 0;
			bool isValid = false;
		};

		struct Slot
		{
			ComPtr<ID3D11Texture2D> d3dTexture;
			Texture *vulkanTexture = nullptr;
			QueuedFrame queuedFrame;
		};

		ComPtr<IDXGIFactory1> dxgiFactory;
		ComPtr<IDXGIAdapter1> dxgiAdapter;
		ComPtr<ID3D11Device> d3dDevice;
		ComPtr<ID3D11DeviceContext> d3dContext;
		std::vector<Slot> slots;
		std::deque<QueuedFrame> pendingFrames;
		Lockable frameLock;
		FrameReadyCallback frameReadyCallback;
	};

	static DXGI_FORMAT GetD3DFormatForTextureFormat(Texture::Format format)
	{
		switch(format)
		{
			case Texture::Format::RGB_8_SRGB:
			case Texture::Format::RGBA_8_SRGB:
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			case Texture::Format::BGRA_8_SRGB:
				return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			case Texture::Format::RGB_8:
			case Texture::Format::RGBA_8:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case Texture::Format::BGRA_8:
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			default:
				return DXGI_FORMAT_UNKNOWN;
		}
	}

	static bool CheckSharedTextureFormatSupport(ID3D11Device *device, DXGI_FORMAT format)
	{
		UINT support = 0;
		const HRESULT result = device->CheckFormatSupport(format, &support);
		const UINT requiredSupport = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;

		if(FAILED(result) || (support & requiredSupport) != requiredSupport)
		{
			RNWarning("Recording recording swapchain D3D11 format " << static_cast<int>(format) << " does not support the required shared render target usage: 0x" << std::hex << result << ", support 0x" << support << ", required 0x" << requiredSupport << std::dec);
			return false;
		}

		return true;
	}

	static bool AdapterLUIDMatchesVulkanDevice(const DXGI_ADAPTER_DESC1 &adapterDescription, const uint8 *vulkanLUID)
	{
		static_assert(sizeof(adapterDescription.AdapterLuid) == VK_LUID_SIZE, "DXGI and Vulkan LUID sizes must match");
		return std::memcmp(&adapterDescription.AdapterLuid, vulkanLUID, VK_LUID_SIZE) == 0;
	}

	CaptureWindowsSwapChain::CaptureWindowsSwapChain(VulkanRenderer *renderer, const Vector2 &size, const Window::SwapChainDescriptor &descriptor) :
		_internals(new Internals()),
		_isValid(false)
	{
		_renderer = renderer;
		_device = renderer ? renderer->GetVulkanDevice()->GetDevice() : VK_NULL_HANDLE;
		_size = size;
		_descriptor = descriptor;
		_descriptor.depthStencilFormat = Texture::Format::Invalid;
		_descriptor.layerCount = 1;
		_descriptor.usageHint = Texture::UsageHint::RenderTarget;
		if(_descriptor.bufferCount == 0) _descriptor.bufferCount = 1;
		_frameIndex = _descriptor.bufferCount - 1;
		_semaphoreIndex = 0;

		if(!_renderer)
		{
			RNWarning("Recording recording swapchain requires a Vulkan renderer");
			return;
		}

		VulkanDevice *vulkanDevice = _renderer->GetVulkanDevice();
		if(!vulkanDevice->GetSupportsExternalTextureImport())
		{
			RNWarning("Recording recording swapchain requires VK_KHR_external_memory_win32 support");
			return;
		}

		if(!CreateD3DDeviceForVulkanDevice() || !CreateSharedBuffers())
		{
			return;
		}

		for(size_t i = 0; i < _descriptor.bufferCount; i++)
		{
			_presentSemaphores.push_back(VK_NULL_HANDLE);
			_renderSemaphores.push_back(VK_NULL_HANDLE);
		}

		_frameIndex = 0;
		_framebuffer = CreateSwapChainFramebuffer(_descriptor.layerCount);
		_isValid = true;
	}

	CaptureWindowsSwapChain::~CaptureWindowsSwapChain()
	{
		SafeRelease(_framebuffer);
		_framebuffer = nullptr;

		if(_internals)
		{
			for(Internals::Slot &slot : _internals->slots)
			{
				SafeRelease(slot.vulkanTexture);
			}
			_internals->slots.clear();
		}

		_presentSemaphores.clear();
		_renderSemaphores.clear();

		delete _internals;
		_internals = nullptr;
	}

	bool CaptureWindowsSwapChain::GetIsValid() const
	{
		return _isValid;
	}

	ID3D11Device *CaptureWindowsSwapChain::GetD3DDevice() const
	{
		return _internals ? _internals->d3dDevice.Get() : nullptr;
	}

	ID3D11Texture2D *CaptureWindowsSwapChain::GetD3DTexture(size_t index) const
	{
		if(!_internals || index >= _internals->slots.size()) return nullptr;
		return _internals->slots[index].d3dTexture.Get();
	}

	ID3D11Texture2D *CaptureWindowsSwapChain::GetCurrentD3DTexture() const
	{
		return GetD3DTexture(_frameIndex);
	}

	void CaptureWindowsSwapChain::QueueFrame(int64 presentationTimeUs, int64 durationUs)
	{
		if(!_internals) return;

		LockGuard<Lockable> lock(_internals->frameLock);
		Internals::QueuedFrame frame;
		frame.presentationTimeUs = presentationTimeUs;
		frame.durationUs = durationUs;
		frame.isValid = true;
		_internals->pendingFrames.push_back(frame);
	}

	size_t CaptureWindowsSwapChain::ClearQueuedFrames()
	{
		if(!_internals) return 0;

		LockGuard<Lockable> lock(_internals->frameLock);
		size_t count = _internals->pendingFrames.size();
		_internals->pendingFrames.clear();
		return count;
	}

	void CaptureWindowsSwapChain::SetFrameReadyCallback(FrameReadyCallback callback)
	{
		if(!_internals) return;

		LockGuard<Lockable> lock(_internals->frameLock);
		_internals->frameReadyCallback = callback;
	}

	void CaptureWindowsSwapChain::AcquireBackBuffer()
	{
		if(_descriptor.bufferCount == 0) return;

		_frameIndex = (_frameIndex + 1) % _descriptor.bufferCount;
		_semaphoreIndex = _frameIndex;

		LockGuard<Lockable> lock(_internals->frameLock);
		_internals->slots[_frameIndex].queuedFrame = {};
		if(!_internals->pendingFrames.empty())
		{
			_internals->slots[_frameIndex].queuedFrame = _internals->pendingFrames.front();
			_internals->pendingFrames.pop_front();
		}
	}

	void CaptureWindowsSwapChain::Prepare(VkCommandBuffer commandBuffer)
	{
	}

	void CaptureWindowsSwapChain::Finalize(VkCommandBuffer commandBuffer)
	{
	}

	void CaptureWindowsSwapChain::PresentBackBuffer(VkQueue queue)
	{
		Internals::QueuedFrame frame;
		FrameReadyCallback callback;
		ComPtr<ID3D11Texture2D> texture;
		{
			LockGuard<Lockable> lock(_internals->frameLock);
			if(_frameIndex >= _internals->slots.size()) return;

			frame = _internals->slots[_frameIndex].queuedFrame;
			_internals->slots[_frameIndex].queuedFrame = {};
			callback = _internals->frameReadyCallback;
			texture = _internals->slots[_frameIndex].d3dTexture;
		}

		if(!frame.isValid || !callback || !texture) return;

		_renderer->AddFrameFinishedCallback([texture, callback, frame] {
			callback(texture.Get(), frame.presentationTimeUs, frame.durationUs);
		});
	}

	VkImage CaptureWindowsSwapChain::GetVulkanColorBuffer(int index) const
	{
		if(!_internals || index < 0 || static_cast<size_t>(index) >= _internals->slots.size()) return VK_NULL_HANDLE;
		Texture *texture = _internals->slots[index].vulkanTexture;
		return texture ? reinterpret_cast<VkImage>(texture->GetAPITexture()) : VK_NULL_HANDLE;
	}

	VkImage CaptureWindowsSwapChain::GetVulkanDepthBuffer(int index) const
	{
		return VK_NULL_HANDLE;
	}

	bool CaptureWindowsSwapChain::CreateD3DDeviceForVulkanDevice()
	{
		uint8 vulkanLUID[VK_LUID_SIZE] = {};
		if(!_renderer->GetVulkanDevice()->GetDeviceLUID(vulkanLUID))
		{
			RNWarning("Recording recording swapchain could not query Vulkan device LUID");
			return false;
		}

		HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&_internals->dxgiFactory));
		if(FAILED(result))
		{
			RNWarning("Recording recording swapchain failed to create DXGI factory: 0x" << std::hex << result << std::dec);
			return false;
		}

		for(UINT adapterIndex = 0;; adapterIndex++)
		{
			ComPtr<IDXGIAdapter1> adapter;
			result = _internals->dxgiFactory->EnumAdapters1(adapterIndex, &adapter);
			if(result == DXGI_ERROR_NOT_FOUND) break;
			if(FAILED(result)) continue;

			DXGI_ADAPTER_DESC1 adapterDescription = {};
			adapter->GetDesc1(&adapterDescription);
			if(AdapterLUIDMatchesVulkanDevice(adapterDescription, vulkanLUID))
			{
				_internals->dxgiAdapter = adapter;
				break;
			}
		}

		if(!_internals->dxgiAdapter)
		{
			RNWarning("Recording recording swapchain could not find a DXGI adapter matching the Vulkan device LUID");
			return false;
		}

		UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
	#if RN_BUILD_DEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0};

		result = D3D11CreateDevice(_internals->dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels, 2, D3D11_SDK_VERSION, &_internals->d3dDevice, &featureLevel, &_internals->d3dContext);
		if(result == E_INVALIDARG)
		{
			result = D3D11CreateDevice(_internals->dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, &featureLevels[1], 1, D3D11_SDK_VERSION, &_internals->d3dDevice, &featureLevel, &_internals->d3dContext);
		}
	#if RN_BUILD_DEBUG
		if(FAILED(result) && (flags & D3D11_CREATE_DEVICE_DEBUG))
		{
			flags &= ~D3D11_CREATE_DEVICE_DEBUG;
			result = D3D11CreateDevice(_internals->dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, featureLevels, 2, D3D11_SDK_VERSION, &_internals->d3dDevice, &featureLevel, &_internals->d3dContext);
			if(result == E_INVALIDARG)
			{
				result = D3D11CreateDevice(_internals->dxgiAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, &featureLevels[1], 1, D3D11_SDK_VERSION, &_internals->d3dDevice, &featureLevel, &_internals->d3dContext);
			}
		}
	#endif

		if(FAILED(result))
		{
			RNWarning("Recording recording swapchain failed to create D3D11 device: 0x" << std::hex << result << std::dec);
			return false;
		}

		ComPtr<ID3D10Multithread> d3dMultithread;
		if(SUCCEEDED(_internals->d3dDevice.As(&d3dMultithread)))
		{
			d3dMultithread->SetMultithreadProtected(TRUE);
		}

		return true;
	}

	bool CaptureWindowsSwapChain::CreateSharedBuffers()
	{
		const DXGI_FORMAT d3dFormat = GetD3DFormatForTextureFormat(_descriptor.colorFormat);
		if(d3dFormat == DXGI_FORMAT_UNKNOWN)
		{
			RNWarning("Recording recording swapchain does not support texture format " << static_cast<int>(_descriptor.colorFormat));
			return false;
		}
		if(!CheckSharedTextureFormatSupport(_internals->d3dDevice.Get(), d3dFormat))
		{
			return false;
		}

		const UINT width = static_cast<UINT>(std::max(1.0f, _size.x));
		const UINT height = static_cast<UINT>(std::max(1.0f, _size.y));

		_internals->slots.resize(_descriptor.bufferCount);
		for(size_t i = 0; i < _internals->slots.size(); i++)
		{
			D3D11_TEXTURE2D_DESC textureDescription = {};
			textureDescription.Width = width;
			textureDescription.Height = height;
			textureDescription.MipLevels = 1;
			textureDescription.ArraySize = 1;
			textureDescription.Format = d3dFormat;
			textureDescription.SampleDesc.Count = 1;
			textureDescription.SampleDesc.Quality = 0;
			textureDescription.Usage = D3D11_USAGE_DEFAULT;
			textureDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			textureDescription.CPUAccessFlags = 0;
			textureDescription.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

			HRESULT result = _internals->d3dDevice->CreateTexture2D(&textureDescription, nullptr, &_internals->slots[i].d3dTexture);
			if(FAILED(result))
			{
				RNWarning("Recording recording swapchain failed to create shared D3D11 texture: 0x" << std::hex << result << ", format " << static_cast<int>(textureDescription.Format) << ", bind 0x" << textureDescription.BindFlags << ", misc 0x" << textureDescription.MiscFlags << std::dec << ", size " << textureDescription.Width << "x" << textureDescription.Height);
				return false;
			}

			ComPtr<IDXGIResource1> dxgiResource;
			result = _internals->slots[i].d3dTexture.As(&dxgiResource);
			if(FAILED(result))
			{
				RNWarning("Recording recording swapchain failed to query IDXGIResource1: 0x" << std::hex << result << std::dec);
				return false;
			}

			HANDLE sharedHandle = nullptr;
			result = dxgiResource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &sharedHandle);
			if(FAILED(result) || !sharedHandle)
			{
				RNWarning("Recording recording swapchain failed to create shared D3D11 texture handle: 0x" << std::hex << result << std::dec);
				return false;
			}

			Texture::Descriptor textureDescriptor;
			textureDescriptor.usageHint = Texture::UsageHint::RenderTarget;
			textureDescriptor.accessOptions = GPUResource::AccessOptions::ReadWrite;
			textureDescriptor.width = width;
			textureDescriptor.height = height;
			textureDescriptor.depth = 1;
			textureDescriptor.type = Texture::Type::Type2D;
			textureDescriptor.format = _descriptor.colorFormat;
			textureDescriptor.sampleCount = 1;
			textureDescriptor.mipMaps = 1;

			Texture::ExternalMemoryDescriptor externalMemoryDescriptor;
			externalMemoryDescriptor.handle = reinterpret_cast<uint64>(sharedHandle);
			externalMemoryDescriptor.handleType = Texture::ExternalMemoryHandleType::D3D11Texture;
			externalMemoryDescriptor.dedicatedAllocation = true;

			_internals->slots[i].vulkanTexture = _renderer->CreateTextureWithExternalMemory(textureDescriptor, externalMemoryDescriptor);
			CloseHandle(sharedHandle);
		}

		return true;
	}
} // namespace RN
#endif
