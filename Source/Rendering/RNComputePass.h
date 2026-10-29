//
//  RNComputePass.h
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_COMPUTEPASS_H__
#define __RAYNE_COMPUTEPASS_H__

#include "../Base/RNBase.h"
#include "RNFramePass.h"
#include "RNGPUBuffer.h"
#include "RNShader.h"
#include "RNTexture.h"

namespace RN
{
	class ComputePass : public FramePass
	{
	public:
		struct DispatchSize
		{
			uint32 x = 1;
			uint32 y = 1;
			uint32 z = 1;
		};

		struct DispatchOffset
		{
			uint32 x = 0;
			uint32 y = 0;
			uint32 z = 0;
		};

		struct DispatchRegion
		{
			DispatchSize groupCount;
			DispatchOffset groupOffset;
		};

		class DispatchSnapshot
		{
		public:
			void CaptureResourceBuffers(GPUBuffer::SnapshotReferences &bufferReferences)
			{
				for(auto &entry : _resourceBuffers) entry.second = bufferReferences.CaptureBuffer(entry.second.Get());
			}

			Shader *GetShader() const { return _shader.Get(); }
			RNAPI const DispatchSize &GetGroupCount() const;
			RNAPI const DispatchOffset &GetGroupOffset() const;
			const std::vector<DispatchRegion> &GetDispatchRegions() const { return _dispatchRegions; }
			RNAPI GPUBuffer *GetResourceBuffer(size_t nameHash) const;
			RNAPI Texture *GetResourceTexture(size_t nameHash) const;
			RNAPI const std::vector<uint8> *GetUniform(size_t nameHash) const;

		private:
			friend class ComputePass;

			void Reset();

			StrongRef<Shader> _shader;
			std::vector<DispatchRegion> _dispatchRegions;
			std::unordered_map<size_t, StrongRef<GPUBuffer>> _resourceBuffers;
			std::unordered_map<size_t, StrongRef<Texture>> _resourceTextures;
			std::unordered_map<size_t, std::vector<uint8>> _uniforms;
		};

		RNAPI ComputePass(Shader *shader = nullptr);
		RNAPI ~ComputePass() override;

		RNAPI void SetShader(Shader *shader);
		RNAPI void SetGroupCount(uint32 x, uint32 y = 1, uint32 z = 1);
		RNAPI void SetGroupOffset(uint32 x, uint32 y = 0, uint32 z = 0);
		RNAPI void AddDispatchRegion(uint32 groupCountX, uint32 groupCountY = 1, uint32 groupCountZ = 1, uint32 groupOffsetX = 0, uint32 groupOffsetY = 0, uint32 groupOffsetZ = 0);
		RNAPI void ClearDispatchRegions();
		RNAPI void SetResourceBuffer(const String *name, GPUBuffer *buffer);
		RNAPI void SetResourceTexture(const String *name, Texture *texture);
		RNAPI void SetUniform(const String *name, const void *data, size_t size);

		Shader *GetShader() const { return _shader; }
		RNAPI const DispatchSize &GetGroupCount() const;
		RNAPI const DispatchOffset &GetGroupOffset() const;
		const std::vector<DispatchRegion> &GetDispatchRegions() const { return _dispatchRegions; }
		RNAPI void GetDispatchSnapshot(DispatchSnapshot &snapshot) const;

	private:
		DispatchRegion &GetPrimaryDispatchRegion();
		bool HasOnlyDefaultDispatchRegion() const;

		Shader *_shader;
		std::vector<DispatchRegion> _dispatchRegions;
		std::unordered_map<size_t, StrongRef<GPUBuffer>> _resourceBuffers;
		std::unordered_map<size_t, StrongRef<Texture>> _resourceTextures;
		std::unordered_map<size_t, std::vector<uint8>> _uniforms;

		__RNDeclareMetaInternal(ComputePass)
	};
} // namespace RN

#endif /* __RAYNE_COMPUTEPASS_H__ */
