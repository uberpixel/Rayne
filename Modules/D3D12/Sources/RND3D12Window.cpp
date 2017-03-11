//
//  RND3D12Window.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Window.h"
#include "RND3D12Renderer.h"
#include "RND3D12Framebuffer.h"

namespace RN
{
	RNDefineMeta(D3D12Window, Window)

	D3D12Window::D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer) : 
		Window(screen), 
		_renderer(renderer),
		_frameIndex(0)
	{
		for(int i = 0; i < 3; i++)
			_fenceValues[i] = 0;

		HINSTANCE hInstance = ::GetModuleHandle(nullptr);

		static std::once_flag flag;
		std::call_once(flag, [&] {

			WNDCLASSEXW windowClass = { 0 };
			windowClass.cbSize = sizeof(WNDCLASSEX);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = &DefWindowProcW;
			windowClass.hInstance = hInstance;
			windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			windowClass.lpszClassName = L"RND3D12WindowClass";
			RegisterClassExW(&windowClass); 

		});

		const DWORD style =  WS_OVERLAPPEDWINDOW;

		RECT windowRect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
		AdjustWindowRect(&windowRect, style, false);

		Rect frame = screen->GetFrame();

		Vector2 offset = Vector2(frame.width - (windowRect.right - windowRect.left), frame.height - (windowRect.bottom - windowRect.top));
		offset *= 0.5;
		offset.x += frame.x;
		offset.y += frame.y;

		_hwnd = CreateWindowExW(0, L"RND3D12WindowClass", L"", style, offset.x, offset.y, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, this);

		SetForegroundWindow(_hwnd);

		// 
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();

		D3D12_COMMAND_QUEUE_DESC queueDescriptor = { };
		queueDescriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDescriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		device->CreateCommandQueue(&queueDescriptor, IID_PPV_ARGS(&_commandQueue));

		device->CreateFence(_fenceValues[_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		_fenceValues[_frameIndex] ++;
		_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

		ResizeSwapchain(size);
	}

	D3D12Window::~D3D12Window()
	{
		DestroyWindow(_hwnd);
	}

	void D3D12Window::ResizeSwapchain(const Vector2 &size)
	{
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();
		IDXGIFactory4 *factory = _renderer->GetD3D12Descriptor()->GetFactory();

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));

		Vector2 windowSize = GetSize();
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 3;
		swapChainDesc.BufferDesc.Width = windowSize.x;
		swapChainDesc.BufferDesc.Height = windowSize.y;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = _hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = true;

		IDXGISwapChain *__swapChain;
		factory->CreateSwapChain(_commandQueue, &swapChainDesc, &__swapChain);
		_swapChain = static_cast<IDXGISwapChain3 *>(__swapChain);
		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		Framebuffer::Descriptor descriptor;
		descriptor.options = Framebuffer::Options::PrivateStorage;
		descriptor.colorFormat = Texture::Format::RGBA8888;

		_framebuffer = new D3D12Framebuffer(size, descriptor, _swapChain, _renderer);
	}

	void D3D12Window::SetTitle(const String *title)
	{
		char *text = title->GetUTF8String();
		wchar_t *wtext = new wchar_t[strlen(text) + 1];
		mbstowcs(wtext, text, strlen(text) + 1);//Plus null
		LPWSTR ptr = wtext;

		SetWindowTextW(_hwnd, ptr);

		delete[] wtext;
	}

	Screen *D3D12Window::GetScreen()
	{
		HMONITOR monitor = ::MonitorFromWindow(_hwnd, MONITOR_DEFAULTTOPRIMARY);

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

	void D3D12Window::Show()
	{
		ShowWindow(_hwnd, SW_SHOW);
	}

	void D3D12Window::Hide()
	{
		ShowWindow(_hwnd, SW_HIDE);
	}

	Vector2 D3D12Window::GetSize() const
	{
		RECT windowRect;
		GetClientRect(_hwnd, &windowRect);
		return Vector2(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}

	void D3D12Window::AcquireBackBuffer()
	{
		const UINT64 fenceValue = _fenceValues[_frameIndex];
		_commandQueue->Signal(_fence, fenceValue);

		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		_completedFenceValue = _fence->GetCompletedValue();
		if(_completedFenceValue < _fenceValues[_frameIndex])
		{
			_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
			WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
		}

		_fenceValues[_frameIndex] = fenceValue + 1;
	}

	void D3D12Window::PresentBackBuffer()
	{
		_swapChain->Present(1, 0); //Use 0, 0 for no vsync
	}
}
