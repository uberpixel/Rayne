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
		_totalSize(size),
		_sizeReserved(0)
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

	void D3D12UniformBuffer::Reset()
	{
		//Doesn't actually remove any data, just resets the allocation info to start allocating from the start again.
		_sizeUsed = 0;
		_offsetToFreeData = 0;
	}

	size_t D3D12UniformBuffer::Allocate(size_t size, bool align)
	{
		//Align offset when allocating the next buffer (if it is supposed to be aligned)
		if(align) _offsetToFreeData += kRNUniformBufferAlignement - (_offsetToFreeData % kRNUniformBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_offsetToFreeData);
		if (availableSize < static_cast<int>(size))
			return -1;

		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_sizeUsed += size;
		return newDataOffset;
	}

	size_t D3D12UniformBuffer::Reserve(size_t size)
	{
		size_t alignedSize = size + kRNUniformBufferAlignement - (size % kRNUniformBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_sizeReserved);
		if (availableSize < static_cast<int>(alignedSize))
			return -1;

		_sizeReserved += alignedSize;
		return alignedSize;
	}

	void D3D12UniformBuffer::Unreserve(size_t size)
	{
		_sizeReserved -= size;
	}

	D3D12UniformBufferReference::D3D12UniformBufferReference() : shaderResourceIndex(0), offset(0), size(0), reservedSize(0), uniformBuffer(nullptr)
	{

	}

	D3D12UniformBufferReference::~D3D12UniformBufferReference()
	{
		if(uniformBuffer) uniformBuffer->Unreserve(reservedSize);
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
		size_t reservedSize = -1;
		D3D12UniformBuffer *uniformBuffer = nullptr;
		_uniformBuffers->Enumerate<D3D12UniformBuffer>([&](D3D12UniformBuffer *buffer, uint32 index, bool &stop) {
			reservedSize = buffer->Reserve(size);
			if(reservedSize != -1)
			{
				uniformBuffer = buffer;
				stop = true;
			}
		});

		D3D12UniformBufferReference *reference = new D3D12UniformBufferReference();
		reference->size = size;
		reference->reservedSize = reservedSize;
		reference->offset = -1;
		reference->uniformBuffer = uniformBuffer;
		reference->shaderResourceIndex = index;

		if(uniformBuffer) return reference->Autorelease();

		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}

	void D3D12UniformBufferPool::UpdateUniformBufferReference(D3D12UniformBufferReference *reference, bool align)
	{
		RN_ASSERT(reference->uniformBuffer, "Somethings up with the reference not having a uniform buffer assigned");
		size_t bufferOffset = reference->uniformBuffer->Allocate(reference->size, align);
		RN_ASSERT(bufferOffset != -1, "The uniform buffer does not have enough space to fit the memory required by this reference. This should never happen...");

		reference->offset = bufferOffset;
	}

	void D3D12UniformBufferPool::Update(size_t currentFrame, size_t completedFrame)
	{
		_uniformBuffers->Enumerate<D3D12UniformBuffer>([&](D3D12UniformBuffer *buffer, uint32 index, bool &stop) {
			buffer->Advance(currentFrame, completedFrame);
			buffer->Reset();
		});

		size_t requiredSize = 0;
		_newReferences->Enumerate<D3D12UniformBufferReference>([&](D3D12UniformBufferReference *reference, uint32 index, bool &stop) {
			requiredSize += reference->size + kRNUniformBufferAlignement - (reference->size % kRNUniformBufferAlignement);
		});

		if(requiredSize == 0) return;

		//TODO: limit to a maximum size per buffer
		requiredSize = std::max(requiredSize, static_cast<size_t>(kRNMinimumUniformBufferSize));
		D3D12UniformBuffer *uniformBuffer = new D3D12UniformBuffer(requiredSize);

		_newReferences->Enumerate<D3D12UniformBufferReference>([&](D3D12UniformBufferReference *reference, uint32 index, bool &stop) {
			reference->uniformBuffer = uniformBuffer;
			reference->reservedSize = uniformBuffer->Reserve(reference->size);
		});

		_uniformBuffers->AddObject(uniformBuffer->Autorelease());
		_newReferences->RemoveAllObjects();
	}

	void D3D12UniformBufferPool::FlushAllBuffers()
	{
		_uniformBuffers->Enumerate<D3D12UniformBuffer>([&](D3D12UniformBuffer *buffer, uint32 index, bool &stop) {
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Flush();
		});
	}
}
