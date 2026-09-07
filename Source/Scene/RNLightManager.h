//
//  RNLightManager.h
//  Rayne
//
//  Copyright 2025 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIGHT_MANAGER_H__
#define __RAYNE_LIGHT_MANAGER_H__

#include "../Rendering/RNGPUBuffer.h"
#include "RNCamera.h"
#include "RNLight.h"
#include "RNSceneNode.h"

namespace RN
{
	class LightManager : public Object
	{
	public:
		class DrawSnapshot
		{
		public:
			DrawSnapshot() = default;
			DrawSnapshot(GPUBuffer *pointLightBuffer, GPUBuffer *spotLightBuffer, GPUBuffer *clusterRecordsBuffer, GPUBuffer *clusterIndexBuffer) :
				_pointLightBuffer(pointLightBuffer ? pointLightBuffer->GetActiveBuffer() : nullptr),
				_spotLightBuffer(spotLightBuffer ? spotLightBuffer->GetActiveBuffer() : nullptr),
				_clusterRecordsBuffer(clusterRecordsBuffer ? clusterRecordsBuffer->GetActiveBuffer() : nullptr),
				_clusterIndexBuffer(clusterIndexBuffer ? clusterIndexBuffer->GetActiveBuffer() : nullptr)
			{}

			bool IsValid() const { return _pointLightBuffer.Get() || _spotLightBuffer.Get() || _clusterRecordsBuffer.Get() || _clusterIndexBuffer.Get(); }
			GPUBuffer *GetPointLightBuffer() const { return _pointLightBuffer.Get(); }
			GPUBuffer *GetSpotLightBuffer() const { return _spotLightBuffer.Get(); }
			GPUBuffer *GetClusterRecordsBuffer() const { return _clusterRecordsBuffer.Get(); }
			GPUBuffer *GetClusterIndexBuffer() const { return _clusterIndexBuffer.Get(); }

		private:
			StrongRef<GPUBuffer> _pointLightBuffer;
			StrongRef<GPUBuffer> _spotLightBuffer;
			StrongRef<GPUBuffer> _clusterRecordsBuffer;
			StrongRef<GPUBuffer> _clusterIndexBuffer;
		};

		struct ClusterRecord
		{
			// Packed record for 6 clusters: 4 bytes base offset + 12 bytes counts
			// counts01: p0 | s0 | p1 | s1 (8 bits each, LSB..MSB)
			// counts23: p2 | s2 | p3 | s3
			// counts45: p4 | s4 | p5 | s5
			uint32 offset;
			uint32 counts01;
			uint32 counts23;
			uint32 counts45;
		};

		struct PointLightPacked
		{
			Vector4 positionRange; // xyz = camera-relative position, w = range
			Vector4 color; // xyz = color*intensity, w unused
		};

		struct SpotLightPacked
		{
			Vector4 positionRange; // xyz = camera-relative position, w = range
			Vector4 color; // xyz = color*intensity, w unused
			Vector4 dirCos; // xyz = direction, w = cos(angle)
		};

		struct SpotLightCullData
		{
			Vector3 center;
			float radius;
			Vector3 forward;
			float tanHalfAngle;
		};

		struct ClusterGridInfo
		{
			uint32 clustersX;
			uint32 clustersY;
			uint32 clustersZ;
			uint32 pad0;
			float zFirstSliceDepth; //meters from near; if > 0, slice 0 is fixed depth
			float zLogFactor; //0..1 blend between linear/log slicing
			float clipNear;
			float clipFar;
		};
		struct SpotClusterBound
		{
			Vector3 center;
			float radius;
		};

		struct BuildInput
		{
			ClusterGridInfo grid {};
			uint16 maxLightsPerCluster = 255;
			std::vector<PointLightPacked> pointLights;
			std::vector<SpotLightPacked> spotLights;
			std::vector<SpotLightCullData> spotLightCullData;
			std::vector<Matrix> views;
			std::vector<Matrix> projections;
			std::vector<Vector3> viewPositions;
		};

		RNAPI LightManager(uint32 x, uint32 y, uint32 z, float zLogFactor = 0.7f, uint16 maxPackedPointLights = 256, uint16 maxPackedSpotLights = 256);
		RNAPI ~LightManager() override;

		RNAPI void SetClusterGridInfo(uint32 x, uint32 y, uint32 z, float zLogFactor = 0.5f);
		RNAPI void SetZLogFactor(float zLogFactor);
		RNAPI void SetZFirstSliceDepth(float meters);
		RNAPI const ClusterGridInfo &GetClusterGridInfo() const { return _grid; }

		// Capture scene values on the submission thread; consume them only on the render thread.
		RNAPI BuildInput CaptureBuildInput(const Camera *camera, const std::vector<Light *> &lights) const;
		RNAPI DrawSnapshot BuildDrawSnapshot(BuildInput &&input);

		// Render-thread access to the most recently built buffers and CPU data.
		RNAPI GPUBuffer *GetPointLightBuffer() const { return _pointLightBuffer; }
		RNAPI GPUBuffer *GetSpotLightBuffer() const { return _spotLightBuffer; }
		RNAPI GPUBuffer *GetClusterIndexBuffer() const { return _clusterIndexBuffer; }
		RNAPI GPUBuffer *GetClusterRecordsBuffer() const { return _clusterRecordsBuffer; }
		DrawSnapshot GetDrawSnapshot() const { return DrawSnapshot(_pointLightBuffer, _spotLightBuffer, _clusterRecordsBuffer, _clusterIndexBuffer); }

		// These accessors must also only be used on the render thread.
		RNAPI const std::vector<PointLightPacked> &GetPackedPointLights() const { return _buildInput.pointLights; }
		RNAPI const std::vector<SpotLightPacked> &GetPackedSpotLights() const { return _buildInput.spotLights; }
		RNAPI const std::vector<uint16> &GetClusterLightIndices() const { return _clusterLightIndices; }
		RNAPI const std::vector<ClusterRecord> &GetClusterRecords() const { return _clusterRecords; }

		RNAPI void SetMaxLightsPerCluster(uint16 max);
		uint16 GetMaxLightsPerCluster() const { return _maxLightsPerCluster; }
		RNAPI void SetMaxPackedLights(uint16 maxPointLights, uint16 maxSpotLights);
		uint16 GetMaxPackedPointLights() const { return _maxPackedPointLights; }
		uint16 GetMaxPackedSpotLights() const { return _maxPackedSpotLights; }

	private:
		void BuildClusters();
		void UploadBuffers();
		static void EnsureBufferCapacity(GPUBuffer *&buffer, size_t capacity);

		// Helpers
		uint32 ComputeClusterCount() const { return _buildInput.grid.clustersX * _buildInput.grid.clustersY * _buildInput.grid.clustersZ; }
		float ComputeZSlice(float viewZ) const;

		// Submission-thread configuration. Setters never touch render-thread state.
		ClusterGridInfo _grid {};
		uint16 _maxLightsPerCluster;
		uint16 _maxPackedPointLights;
		uint16 _maxPackedSpotLights;

		// Render-thread state, retained across builds to reuse scratch storage and buffers.
		BuildInput _buildInput;

		std::vector<uint16> _clusterLightIndices; // point indices then spot indices per cluster
		std::vector<ClusterRecord> _clusterRecords;
		std::vector<uint16> _clusterPointScratch;
		std::vector<uint16> _clusterSpotScratch;
		std::vector<uint8> _clusterPointCountsScratch;
		std::vector<uint8> _clusterSpotCountsScratch;
		std::vector<uint32> _clusterOffsetsScratch;

		GPUBuffer *_pointLightBuffer;
		GPUBuffer *_spotLightBuffer;
		GPUBuffer *_clusterIndexBuffer;
		GPUBuffer *_clusterRecordsBuffer;

		bool _hasSpotClusterBoundsCache;
		std::vector<Matrix> _cachedSpotBoundsProjections;
		std::vector<SpotClusterBound> _cachedSpotClusterBoundsByEye;

		__RNDeclareMetaInternal(LightManager)
	};
} // namespace RN

#endif
