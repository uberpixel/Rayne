//
//  RND3D12UniformBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12UniformBuffer.h"
#include "RND3D12Renderer.h"
#include "RND3D12GPUBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12UniformBuffer, Object)
	RNDefineMeta(D3D12UniformBufferReference, Object)

	D3D12UniformBuffer::D3D12UniformBuffer(size_t size) :
		_bufferIndex(0),
		_sizeUsed(0),
		_offsetToFreeData(0),
		_totalSize(size)
	{
		D3D12Renderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		GPUBuffer *buffer = realRenderer->CreateBufferWithLength(size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);
		_buffers.push_back(buffer);
		_bufferFrames.push_back(0);
	}

	D3D12UniformBuffer::~D3D12UniformBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *D3D12UniformBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		_bufferIndex = (_bufferIndex + 1) % _buffers.size();

		if(_bufferFrames[_bufferIndex] > completedFrame)
		{
			D3D12Renderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
			GPUBuffer *buffer = realRenderer->CreateBufferWithLength(_totalSize, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);

			_bufferIndex += 1;
			if(_bufferIndex >= _buffers.size())
			{
				_buffers.push_back(buffer);
				_bufferFrames.push_back(currentFrame);
			}
			else
			{
				_buffers.insert(_buffers.begin() + _bufferIndex, buffer);
				_bufferFrames.insert(_bufferFrames.begin() + _bufferIndex, currentFrame);
			}
		}
		else
		{
			_bufferFrames[_bufferIndex] = currentFrame;
		}

		return _buffers[_bufferIndex];

		return nullptr;
	}

	size_t D3D12UniformBuffer::Allocate(size_t size)
	{
		RN::int32 freeSpace = static_cast<RN::int32>(_totalSize) - static_cast<RN::int32>(_offsetToFreeData);
		if(freeSpace < static_cast<RN::int32>(size))
			return -1;

		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_offsetToFreeData += kRNUniformBufferAlignement - (_offsetToFreeData % kRNUniformBufferAlignement);
		_sizeUsed += size;
		return newDataOffset;
	}

	void D3D12UniformBuffer::Free(size_t offset, size_t size)
	{
		_sizeUsed -= size;
		if(_sizeUsed <= 0)
		{
			_offsetToFreeData = 0;
		}
	}

	D3D12UniformBufferReference::D3D12UniformBufferReference() : shaderResourceIndex(0), offset(0), size(0), uniformBuffer(nullptr)
	{

	}

	D3D12UniformBufferReference::~D3D12UniformBufferReference()
	{
		uniformBuffer->Free(offset, size);
	}

	D3D12UniformBufferPool::D3D12UniformBufferPool() :
		_uniformBuffers(new Array()),
		_newReferences(new Array())
	{

	}

	D3D12UniformBufferPool::~D3D12UniformBufferPool()
	{
		_uniformBuffers->Release();
		_newReferences->Release();
	}

	D3D12UniformBufferReference *D3D12UniformBufferPool::GetUniformBufferReference(uint32 size, uint32 index)
	{
		size_t bufferOffset = -1;
		D3D12UniformBuffer *uniformBuffer = nullptr;
		_uniformBuffers->Enumerate<D3D12UniformBuffer>([&](D3D12UniformBuffer *buffer, uint32 index, bool &stop) {
			bufferOffset = buffer->Allocate(size);
			if(bufferOffset != -1)
			{
				uniformBuffer = buffer;
				stop = true;
			}
		});

		D3D12UniformBufferReference *reference = new D3D12UniformBufferReference();
		reference->size = size;
		reference->offset = bufferOffset;
		reference->uniformBuffer = uniformBuffer;
		reference->shaderResourceIndex = index;

		if(uniformBuffer) return reference->Autorelease();

		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}

	void D3D12UniformBufferPool::Update(size_t currentFrame, size_t completedFrame)
	{
		_uniformBuffers->Enumerate<D3D12UniformBuffer>([&](D3D12UniformBuffer *buffer, uint32 index, bool &stop) {
			buffer->Advance(currentFrame, completedFrame);
		});

		size_t requiredSize = 0;
		_newReferences->Enumerate<D3D12UniformBufferReference>([&](D3D12UniformBufferReference *reference, uint32 index, bool &stop) {
			requiredSize += reference->size + kRNUniformBufferAlignement - (reference->size % kRNUniformBufferAlignement);
		});

		if(requiredSize == 0) return;

		requiredSize = std::max(requiredSize, static_cast<size_t>(kRNMinimumUniformBufferSize));
		D3D12UniformBuffer *uniformBuffer = new D3D12UniformBuffer(requiredSize);

		_newReferences->Enumerate<D3D12UniformBufferReference>([&](D3D12UniformBufferReference *reference, uint32 index, bool &stop) {
			reference->uniformBuffer = uniformBuffer;
			reference->offset = uniformBuffer->Allocate(reference->size);
		});

		_uniformBuffers->AddObject(uniformBuffer->Autorelease());
		_newReferences->RemoveAllObjects();
	}

	void D3D12UniformBufferPool::InvalidateAllBuffers()
	{
		_uniformBuffers->Enumerate<D3D12UniformBuffer>([&](D3D12UniformBuffer *buffer, uint32 index, bool &stop) {
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Invalidate();
		});
	}
}
