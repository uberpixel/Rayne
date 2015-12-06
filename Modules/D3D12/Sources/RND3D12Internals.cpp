//
//  RND3D12Internals.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Internals.h"
#include "RND3D12Texture.h"
#include "RND3D12Window.h"

namespace RN
{
	void D3D12RendererInternals::InitializeRenderingPipeline(D3D12Window *window)
	{
		ZeroMemory(fenceValues, sizeof(fenceValues));
		rtvDescriptorSize = 0;

		IDXGIFactory4 *factory;
		CreateDXGIFactory1(IID_PPV_ARGS(&factory));

		IDXGIAdapter1 *hardwareAdapter;
		GetHardwareAdapter(factory, &hardwareAdapter);

		D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
		RN_ASSERT(device, "Needs a valid device");

		// Describe and create the command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

		// Describe and create the swap chain.
		Vector2 windowSize = window->GetSize();
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = 3;
		swapChainDesc.BufferDesc.Width = windowSize.x;
		swapChainDesc.BufferDesc.Height = windowSize.y;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = window->_hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = TRUE;

		IDXGISwapChain *__swapChain;
		factory->CreateSwapChain(commandQueue, &swapChainDesc, &__swapChain);
		swapChain = static_cast<IDXGISwapChain3*>(__swapChain);
		frameIndex = swapChain->GetCurrentBackBufferIndex();

		// Create descriptor heaps.
		{
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = 3;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
			rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap));
		}

		// Create a command allocator for each frame.
		for(UINT n = 0; n < 3; n++)
		{
			device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n]));
		}





		// Create an empty root signature.
		{
			CD3DX12_DESCRIPTOR_RANGE ranges[1];
			CD3DX12_ROOT_PARAMETER rootParameters[1];

			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ID3DBlob *signature;
			ID3DBlob *error;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
			device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		}

		// Create the command list.
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex], nullptr, IID_PPV_ARGS(&commandList));

		CreateFramebuffers(window->GetSize());

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Create synchronization objects and wait until assets have been uploaded to the GPU.
		{
			device->CreateFence(fenceValues[frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
			fenceValues[frameIndex]++;

			// Create an event handle to use for frame synchronization.
			fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if(fenceEvent == nullptr)
			{
//				ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
			}

			// Wait for the command list to execute; we are reusing the same command 
			// list in our main loop but for now, we just want to wait for setup to 
			// complete before continuing.
			WaitForGpu();
		}
	}

	// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
	// If no such adapter can be found, *ppAdapter will be set to nullptr.
	void D3D12RendererInternals::GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter)
	{
		IDXGIAdapter1* pAdapter = nullptr;
		*ppAdapter = nullptr;

		for(UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);

			if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if(SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*ppAdapter = pAdapter;
	}

	void D3D12RendererInternals::CreateFramebuffers(const Vector2 &size)
	{
		viewport.Width = static_cast<float>(size.x);
		viewport.Height = static_cast<float>(size.y);
		viewport.MaxDepth = 1.0f;

		scissorRect.right = static_cast<LONG>(size.x);
		scissorRect.bottom = static_cast<LONG>(size.y);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for(UINT n = 0; n < 3; n++)
		{
			swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]));
			device->CreateRenderTargetView(renderTargets[n], nullptr, rtvHandle);
			rtvHandle.Offset(1, rtvDescriptorSize);
		}
	}

	// Wait for pending GPU work to complete.
	void D3D12RendererInternals::WaitForGpu()
	{
		// Schedule a Signal command in the queue.
		commandQueue->Signal(fence, fenceValues[frameIndex]);

		// Wait until the fence has been processed.
		fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent);
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

		// Increment the fence value for the current frame.
		fenceValues[frameIndex]++;
	}

	// Prepare to render the next frame.
	void D3D12RendererInternals::MoveToNextFrame()
	{
		// Schedule a Signal command in the queue.
		const UINT64 currentFenceValue = fenceValues[frameIndex];
		commandQueue->Signal(fence, currentFenceValue);

		// Update the frame index.
		frameIndex = swapChain->GetCurrentBackBufferIndex();

		// If the next frame is not ready to be rendered yet, wait until it is ready.
		if(fence->GetCompletedValue() < fenceValues[frameIndex])
		{
			fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent);
			WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		fenceValues[frameIndex] = currentFenceValue + 1;
	}

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		return DefWindowProcW(window, message, wparam, lparam);
	}
}