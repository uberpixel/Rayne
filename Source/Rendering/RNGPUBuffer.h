//
//  RNGPUBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_GPUBUFFER_H_
#define __RAYNE_GPUBUFFER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNGPUResource.h"

namespace RN
{
	struct DrawIndirectArguments
	{
		uint32 vertexCount;
		uint32 instanceCount;
		uint32 firstVertex;
		uint32 firstInstance;
	};

	struct DrawIndexedIndirectArguments
	{
		uint32 indexCount;
		uint32 instanceCount;
		uint32 firstIndex;
		int32 vertexOffset;
		uint32 firstInstance;
	};

	class GPUBuffer : public GPUResource
	{
	public:
		RNAPI ~GPUBuffer();
		RNAPI virtual void *GetBuffer() = 0;
		RNAPI virtual void UnmapBuffer() = 0;
		RNAPI virtual void Invalidate();
		RNAPI virtual void InvalidateRange(const Range &range) = 0;
		RNAPI virtual void Flush();
		// Discard allows bytes outside the flushed range to be lost.
		RNAPI virtual void FlushRange(const Range &range, bool discard = false) = 0;
		RNAPI virtual size_t GetLength() const = 0;
		RNAPI virtual GPUBuffer *GetActiveBuffer() const;

		virtual StrongRef<GPUBuffer> GetSnapshotReference() { return {}; }
		// Strong references keep allocations alive. Reservations prevent overwrites.
		// Do not capture snapshots while writing buffers. Reservations can be released on another thread.
		virtual void ReserveForSnapshot() noexcept {}
		virtual void ReleaseSnapshotReservation() noexcept {}

		class SnapshotReferences
		{
		public:
			SnapshotReferences() = default;
			SnapshotReferences(const SnapshotReferences &) = delete;
			SnapshotReferences &operator=(const SnapshotReferences &) = delete;
			~SnapshotReferences()
			{
				for(auto &entry : _buffers) entry.second->ReleaseSnapshotReservation();
			}

			GPUBuffer *CaptureBuffer(GPUBuffer *buffer)
			{
				if(!buffer) return nullptr;
				GPUBuffer *activeBuffer = buffer->GetActiveBuffer();
				if(_buffers.count(activeBuffer)) return activeBuffer;
				StrongRef<GPUBuffer> bufferReference = activeBuffer->GetSnapshotReference();
				if(!bufferReference.Get()) return buffer;
				// Reserve after insertion so allocation failure cannot leak a reservation.
				auto entry = _buffers.emplace(activeBuffer, std::move(bufferReference));
				if(entry.second) entry.first->second->ReserveForSnapshot();
				return activeBuffer;
			}

		private:
			std::unordered_map<GPUBuffer *, StrongRef<GPUBuffer>> _buffers;
		};

	protected:
		RNAPI GPUBuffer();

		__RNDeclareMetaInternal(GPUBuffer)
	};
} // namespace RN


#endif /* __RAYNE_GPUBUFFER_H_ */
