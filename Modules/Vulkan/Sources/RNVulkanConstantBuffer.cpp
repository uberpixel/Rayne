//
//  RNVulkanConstantBuffer.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanConstantBuffer.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanGPUBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanConstantBuffer, Object)
	RNDefineMeta(VulkanConstantBufferReference, Object)

	VulkanConstantBuffer::VulkanConstantBuffer(Renderer *renderer, size_t size) :
		_bufferIndex(0),
		_sizeUsed(0),
		_offsetToFreeData(0),
		_totalSize(size),
		_sizeReserved(0)
	{
		VulkanRenderer *realRenderer = renderer->Downcast<VulkanRenderer>();
		GPUBuffer *buffer = realRenderer->CreateBufferWithLength(size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);
		_buffers.push_back(buffer);
		_bufferFrames.push_back(0);
	}

	VulkanConstantBuffer::~VulkanConstantBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *VulkanConstantBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		_bufferIndex = (_bufferIndex + 1) % _buffers.size();

		if(_bufferFrames[_bufferIndex] > completedFrame)
		{
			VulkanRenderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
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
	}

	void VulkanConstantBuffer::Reset()
	{
		//Doesn't actually remove any data, just resets the allocation info to start allocating from the start again.
		_sizeUsed = 0;
		_offsetToFreeData = 0;
	}

	size_t VulkanConstantBuffer::Allocate(size_t size, bool align)
	{
		//Align offset when allocating the next buffer (if it is supposed to be aligned)
		if(align) _offsetToFreeData += kRNConstantBufferAlignement - (_offsetToFreeData % kRNConstantBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_offsetToFreeData);
		if(availableSize < static_cast<int>(size))
			return -1;

		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_sizeUsed += size;
		return newDataOffset;
	}

	size_t VulkanConstantBuffer::Reserve(size_t size)
	{
		size_t alignedSize = size + kRNConstantBufferAlignement - (size % kRNConstantBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_sizeReserved);
		if(availableSize < static_cast<int>(alignedSize))
			return -1;

		_sizeReserved += alignedSize;
		return alignedSize;
	}

	void VulkanConstantBuffer::Unreserve(size_t size)
	{
		_sizeReserved -= size;
	}

	VulkanConstantBufferReference::VulkanConstantBufferReference() : shaderResourceIndex(0), offset(0), size(0), reservedSize(0), constantBuffer(nullptr)
	{

	}

	VulkanConstantBufferReference::~VulkanConstantBufferReference()
	{
		if(constantBuffer) constantBuffer->Unreserve(reservedSize);
	}

	VulkanConstantBufferPool::VulkanConstantBufferPool() :
		_constantBuffers(new Array()),
		_newReferences(new Array())
	{

	}

	VulkanConstantBufferPool::~VulkanConstantBufferPool()
	{
		_constantBuffers->Release();
		_newReferences->Release();
	}

	VulkanConstantBufferReference *VulkanConstantBufferPool::GetConstantBufferReference(uint32 size, uint32 index)
	{
		size_t reservedSize = -1;
		VulkanConstantBuffer *uniformBuffer = nullptr;
		_constantBuffers->Enumerate<VulkanConstantBuffer>([&](VulkanConstantBuffer *buffer, uint32 index, bool &stop){
			reservedSize = buffer->Reserve(size);
			if(reservedSize != -1)
			{
				uniformBuffer = buffer;
				stop = true;
			}
		});

		VulkanConstantBufferReference *reference = new VulkanConstantBufferReference();
		reference->size = size;
		reference->reservedSize = reservedSize;
		reference->offset = -1;
		reference->constantBuffer = uniformBuffer;
		reference->shaderResourceIndex = index;

		if(uniformBuffer) return reference->Autorelease();

		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}

	void VulkanConstantBufferPool::UpdateConstantBufferReference(VulkanConstantBufferReference *reference, bool align)
	{
		RN_ASSERT(reference->constantBuffer, "Somethings up with the reference not having a uniform buffer assigned");
		size_t bufferOffset = reference->constantBuffer->Allocate(reference->size, align);
		RN_ASSERT(bufferOffset != -1, "The uniform buffer does not have enough space to fit the memory required by this reference. This should never happen...");

		reference->offset = bufferOffset;
	}

	void VulkanConstantBufferPool::Update(Renderer *renderer, size_t currentFrame, size_t completedFrame)
	{
		_constantBuffers->Enumerate<VulkanConstantBuffer>([&](VulkanConstantBuffer *buffer, uint32 index, bool &stop){
			buffer->Advance(currentFrame, completedFrame);
			buffer->Reset();
		});

		size_t requiredSize = 0;
		_newReferences->Enumerate<VulkanConstantBufferReference>([&](VulkanConstantBufferReference *reference, uint32 index, bool &stop){
			requiredSize += reference->size + kRNConstantBufferAlignement - (reference->size % kRNConstantBufferAlignement);
		});

		if(requiredSize == 0) return;

		//TODO: limit to a maximum size per buffer
		requiredSize = std::max(requiredSize, static_cast<size_t>(kRNMinimumConstantBufferSize));
		VulkanConstantBuffer *constantBuffer = new VulkanConstantBuffer(renderer, requiredSize);

		_newReferences->Enumerate<VulkanConstantBufferReference>([&](VulkanConstantBufferReference *reference, uint32 index, bool &stop){
			reference->constantBuffer = constantBuffer;
			reference->reservedSize = constantBuffer->Reserve(reference->size);
		});

		_constantBuffers->AddObject(constantBuffer->Autorelease());
		_newReferences->RemoveAllObjects();
	}

	void VulkanConstantBufferPool::FlushAllBuffers()
	{
		_constantBuffers->Enumerate<VulkanConstantBuffer>([&](VulkanConstantBuffer *buffer, uint32 index, bool &stop){
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Flush();
		});
	}
}
