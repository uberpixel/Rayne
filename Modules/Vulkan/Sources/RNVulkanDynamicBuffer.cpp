//
//  RNVulkanDynamicBuffer.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanDynamicBuffer.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanGPUBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanDynamicBuffer, Object)
	RNDefineMeta(VulkanDynamicBufferReference, Object)

	VulkanDynamicBuffer::VulkanDynamicBuffer(Renderer *renderer, size_t size, GPUResource::UsageOptions usageOptions) :
		_bufferIndex(0),
		_sizeUsed(0),
		_offsetToFreeData(0),
		_totalSize(size),
		_sizeReserved(0),
        _usageOptions(usageOptions)
	{
		VulkanRenderer *realRenderer = renderer->Downcast<VulkanRenderer>();
		GPUBuffer *buffer = realRenderer->CreateBufferWithLength(size, usageOptions, GPUResource::AccessOptions::ReadWrite);
		_buffers.push_back(buffer);
		_bufferFrames.push_back(0);
	}

	VulkanDynamicBuffer::~VulkanDynamicBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *VulkanDynamicBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		_bufferIndex = (_bufferIndex + 1) % _buffers.size();

		if(_bufferFrames[_bufferIndex] > completedFrame)
		{
			VulkanRenderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
			GPUBuffer *buffer = realRenderer->CreateBufferWithLength(_totalSize, _usageOptions, GPUResource::AccessOptions::ReadWrite);

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

	void VulkanDynamicBuffer::Reset()
	{
		//Doesn't actually remove any data, just resets the allocation info to start allocating from the start again.
		_sizeUsed = 0;
		_offsetToFreeData = 0;
	}

	size_t VulkanDynamicBuffer::Allocate(size_t size, bool align)
	{
		//Align offset when allocating the next buffer (if it is supposed to be aligned)
		if(align) _offsetToFreeData += kRNDynamicBufferAlignement - (_offsetToFreeData % kRNDynamicBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_offsetToFreeData);
		if(availableSize < static_cast<int>(size))
			return -1;

		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_sizeUsed += size;
		return newDataOffset;
	}

	size_t VulkanDynamicBuffer::Reserve(size_t size)
	{
		size_t alignedSize = size + kRNDynamicBufferAlignement - (size % kRNDynamicBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_sizeReserved);
		if(availableSize < static_cast<int>(alignedSize))
			return -1;

		_sizeReserved += alignedSize;
		return alignedSize;
	}

	void VulkanDynamicBuffer::Unreserve(size_t size)
	{
		_sizeReserved -= size;
	}

	VulkanDynamicBufferReference::VulkanDynamicBufferReference() : shaderResourceIndex(0), offset(0), size(0), reservedSize(0), dynamicBuffer(nullptr)
	{

	}

	VulkanDynamicBufferReference::~VulkanDynamicBufferReference()
	{
		if(dynamicBuffer) dynamicBuffer->Unreserve(reservedSize);
	}

	VulkanDynamicBufferPool::VulkanDynamicBufferPool() :
			_dynamicBuffers(new Array()),
			_newReferences(new Array())
	{

	}

	VulkanDynamicBufferPool::~VulkanDynamicBufferPool()
	{
		_dynamicBuffers->Release();
		_newReferences->Release();
	}

	VulkanDynamicBufferReference *VulkanDynamicBufferPool::GetDynamicBufferReference(uint32 size, uint32 index, GPUResource::UsageOptions usageOptions)
	{
		size_t reservedSize = -1;
		VulkanDynamicBuffer *dynamicBuffer = nullptr;
		_dynamicBuffers->Enumerate<VulkanDynamicBuffer>([&](VulkanDynamicBuffer *buffer, uint32 index, bool &stop){
            if(buffer->_usageOptions != usageOptions) return;

			reservedSize = buffer->Reserve(size);
			if(reservedSize != -1)
			{
                dynamicBuffer = buffer;
				stop = true;
			}
		});

		VulkanDynamicBufferReference *reference = new VulkanDynamicBufferReference();
		reference->size = size;
		reference->reservedSize = reservedSize;
		reference->offset = -1;
		reference->dynamicBuffer = dynamicBuffer;
		reference->shaderResourceIndex = index;
		reference->usageOptions = usageOptions;

		if(dynamicBuffer) return reference->Autorelease();

		_newReferences->AddObject(reference);
		return reference->Autorelease();
	}

	void VulkanDynamicBufferPool::UpdateDynamicBufferReference(VulkanDynamicBufferReference *reference, bool align)
	{
		RN_ASSERT(reference->dynamicBuffer, "Somethings up with the reference not having a uniform buffer assigned");
		size_t bufferOffset = reference->dynamicBuffer->Allocate(reference->size, align);
		RN_ASSERT(bufferOffset != -1, "The uniform buffer does not have enough space to fit the memory required by this reference. This should never happen...");

		reference->offset = bufferOffset;
	}

	void VulkanDynamicBufferPool::Update(Renderer *renderer, size_t currentFrame, size_t completedFrame)
	{
		_dynamicBuffers->Enumerate<VulkanDynamicBuffer>([&](VulkanDynamicBuffer *buffer, uint32 index, bool &stop){
			buffer->Advance(currentFrame, completedFrame);
			buffer->Reset();
		});

		std::map<GPUResource::UsageOptions, size_t> requiredSize;
		_newReferences->Enumerate<VulkanDynamicBufferReference>([&](VulkanDynamicBufferReference *reference, uint32 index, bool &stop){
			if(requiredSize.count(reference->usageOptions) == 0)
			{
				requiredSize[reference->usageOptions] = 0;
			}
			requiredSize[reference->usageOptions] += reference->size + kRNDynamicBufferAlignement - (reference->size % kRNDynamicBufferAlignement);
		});

		if(requiredSize.size() == 0) return;

		for(auto const& typeSize : requiredSize)
		{
			//TODO: limit to a maximum size per buffer
			size_t requiredBufferSize = std::max(typeSize.second, static_cast<size_t>(kRNMinimumDynamicBufferSize));
			VulkanDynamicBuffer *dynamicBuffer = new VulkanDynamicBuffer(renderer, requiredBufferSize, typeSize.first);

			_newReferences->Enumerate<VulkanDynamicBufferReference>([&](VulkanDynamicBufferReference *reference, uint32 index, bool &stop){
				if(reference->usageOptions == typeSize.first)
				{
					reference->dynamicBuffer = dynamicBuffer;
					reference->reservedSize = dynamicBuffer->Reserve(reference->size);
				}
			});

			_dynamicBuffers->AddObject(dynamicBuffer->Autorelease());
		}

		_newReferences->RemoveAllObjects();
	}

	void VulkanDynamicBufferPool::FlushAllBuffers()
	{
		_dynamicBuffers->Enumerate<VulkanDynamicBuffer>([&](VulkanDynamicBuffer *buffer, uint32 index, bool &stop){
			if(buffer->_sizeUsed > 0)
				buffer->GetActiveBuffer()->Flush();
		});
	}
}
