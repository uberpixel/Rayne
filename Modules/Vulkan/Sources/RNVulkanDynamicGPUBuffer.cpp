//
//  RNVulkanDynamicBuffer.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanDynamicGPUBuffer.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanGPUBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanDynamicGPUBuffer, VulkanGPUBuffer)
	RNDefineMeta(VulkanDynamicBufferReference, Object)

	VulkanDynamicGPUBuffer::VulkanDynamicGPUBuffer(Renderer *renderer, size_t size, GPUResource::UsageOptions usageOptions, bool isOwnedByPool) :
		_bufferIndex(0),
		_hostBufferIndex(0),
		_isOwnedByPool(isOwnedByPool),
		_sizeUsed(0),
		_offsetToFreeData(0),
		_totalSize(size),
		_sizeReserved(0),
		_usageOptions(usageOptions)
	{
		VulkanRenderer *realRenderer = renderer->Downcast<VulkanRenderer>();
		_buffers.push_back(new VulkanStaticGPUBuffer(realRenderer, nullptr, size, usageOptions, GPUResource::AccessOptions::WriteOnly, !isOwnedByPool));
		if(isOwnedByPool) _bufferFrames.push_back(0);
	}

	VulkanDynamicGPUBuffer::~VulkanDynamicGPUBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	size_t VulkanDynamicGPUBuffer::AlignUpTo(size_t value, size_t alignment)
	{
		size_t remainder = value % alignment;
		return (remainder == 0) ? value : (value + alignment - remainder);
	}

	void *VulkanDynamicGPUBuffer::GetBuffer()
	{
		if(!_isOwnedByPool && !_hasPendingWrite)
		{
			_hostBufferIndex = _bufferIndex;
			if(_buffers[_hostBufferIndex]->HasSnapshotReservations())
			{
				for(_hostBufferIndex = 0; _hostBufferIndex < _buffers.size(); _hostBufferIndex++)
				{
					if(!_buffers[_hostBufferIndex]->HasSnapshotReservations()) break;
				}
				if(_hostBufferIndex == _buffers.size())
				{
					VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
					_buffers.push_back(new VulkanStaticGPUBuffer(renderer, nullptr, _totalSize, _usageOptions, GPUResource::AccessOptions::WriteOnly, true));
				}
			}
			_hasPendingWrite = true;
		}
		return _buffers[_hostBufferIndex]->GetBuffer();
	}

	void VulkanDynamicGPUBuffer::FlushRange(const Range &range, bool discard)
	{
		RN_ASSERT(range.origin <= _totalSize && range.length <= _totalSize - range.origin, "Invalid streamed buffer flush range");
		if(!_isOwnedByPool && _hasPendingWrite && _hostBufferIndex != _bufferIndex && !discard)
		{
			size_t end = range.origin + range.length;
			if(range.origin || end < _totalSize)
			{
				uint8 *destination = static_cast<uint8 *>(_buffers[_hostBufferIndex]->GetBuffer());
				const uint8 *source = static_cast<const uint8 *>(_buffers[_bufferIndex]->GetBuffer());
				if(range.origin) std::memcpy(destination, source, range.origin);
				if(end < _totalSize) std::memcpy(destination + end, source + end, _totalSize - end);
			}
			_buffers[_hostBufferIndex]->Flush();
		}
		else
		{
			_buffers[_hostBufferIndex]->FlushRange(range);
		}
		_bufferIndex = _hostBufferIndex;
		_hasPendingWrite = false;
	}

	void VulkanDynamicGPUBuffer::FlushInternal() //Used internally by the dynamic buffer pool for flushing without advancing
	{
		_buffers[_bufferIndex]->Flush();
	}

	void VulkanDynamicGPUBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		RN_ASSERT(_isOwnedByPool, "Only pooled buffers use frame-based rotation");
		_hostBufferIndex = (_hostBufferIndex + 1) % _buffers.size();
		if(completedFrame == static_cast<size_t>(-1) || _bufferFrames[_hostBufferIndex] > completedFrame)
		{
			VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
			VulkanStaticGPUBuffer *buffer = new VulkanStaticGPUBuffer(renderer, nullptr, _totalSize, _usageOptions, GPUResource::AccessOptions::WriteOnly);
			_hostBufferIndex++;
			if(_hostBufferIndex >= _buffers.size())
			{
				_buffers.push_back(buffer);
				_bufferFrames.push_back(currentFrame);
			}
			else
			{
				_buffers.insert(_buffers.begin() + _hostBufferIndex, buffer);
				_bufferFrames.insert(_bufferFrames.begin() + _hostBufferIndex, currentFrame);
			}
		}
		else
		{
			_bufferFrames[_hostBufferIndex] = currentFrame;
		}
		_bufferIndex = _hostBufferIndex;
	}

	void VulkanDynamicGPUBuffer::Reset()
	{
		//Doesn't actually remove any data, just resets the allocation info to start allocating from the start again.
		_sizeUsed = 0;
		_offsetToFreeData = 0;
	}

	size_t VulkanDynamicGPUBuffer::Allocate(size_t size, bool align)
	{
		//Align offset when allocating the next buffer (if it is supposed to be aligned)
		if(align) _offsetToFreeData = AlignUpTo(_offsetToFreeData, kRNDynamicBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_offsetToFreeData);
		if(availableSize < static_cast<int>(size))
			return -1;

		size_t newDataOffset = _offsetToFreeData;
		_offsetToFreeData += size;
		_sizeUsed += size;
		return newDataOffset;
	}

	size_t VulkanDynamicGPUBuffer::Reserve(size_t size)
	{
		size_t alignedSize = AlignUpTo(size, kRNDynamicBufferAlignement);

		int availableSize = static_cast<int>(_totalSize) - static_cast<int>(_sizeReserved);
		if(availableSize < static_cast<int>(alignedSize))
			return -1;

		_sizeReserved += alignedSize;
		return alignedSize;
	}

	void VulkanDynamicGPUBuffer::Unreserve(size_t size)
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
		VulkanDynamicGPUBuffer *dynamicBuffer = nullptr;
		_dynamicBuffers->Enumerate<VulkanDynamicGPUBuffer>([&](VulkanDynamicGPUBuffer *buffer, uint32 index, bool &stop){
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
		RN_DEBUG_ASSERT(reference->dynamicBuffer, "Somethings up with the reference not having a uniform buffer assigned");
		size_t bufferOffset = reference->dynamicBuffer->Allocate(reference->size, align);
		RN_DEBUG_ASSERT(bufferOffset != -1, "The uniform buffer does not have enough space to fit the memory required by this reference. This should never happen...");

		reference->offset = bufferOffset;
	}

	void VulkanDynamicBufferPool::Update(Renderer *renderer, size_t currentFrame, size_t completedFrame)
	{
		_dynamicBuffers->Enumerate<VulkanDynamicGPUBuffer>([&](VulkanDynamicGPUBuffer *buffer, uint32 index, bool &stop){
			buffer->Advance(currentFrame, completedFrame);
			buffer->Reset();
		});

		std::map<GPUResource::UsageOptions, size_t> requiredSize;
		_newReferences->Enumerate<VulkanDynamicBufferReference>([&](VulkanDynamicBufferReference *reference, uint32 index, bool &stop){
			if(requiredSize.count(reference->usageOptions) == 0)
			{
				requiredSize[reference->usageOptions] = 0;
			}
			requiredSize[reference->usageOptions] += VulkanDynamicGPUBuffer::AlignUpTo(reference->size, kRNDynamicBufferAlignement);
		});

		if(requiredSize.size() == 0) return;

		for(auto const& typeSize : requiredSize)
		{
			//TODO: limit to a maximum size per buffer
			size_t requiredBufferSize = std::max(typeSize.second, static_cast<size_t>(kRNMinimumDynamicBufferSize));
			VulkanDynamicGPUBuffer *dynamicBuffer = new VulkanDynamicGPUBuffer(renderer, requiredBufferSize, typeSize.first, true);

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
		_dynamicBuffers->Enumerate<VulkanDynamicGPUBuffer>([&](VulkanDynamicGPUBuffer *buffer, uint32 index, bool &stop){
			if(buffer->_sizeUsed > 0)
				buffer->FlushInternal();
		});
	}
}
