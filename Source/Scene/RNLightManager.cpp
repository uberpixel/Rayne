//
//  RNLightManager.cpp
//  Rayne
//
//  Copyright 2025 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLightManager.h"
#include "../Rendering/RNRenderer.h"

namespace RN
{
	constexpr uint16 kLightManagerShaderMaxPointLights = 512;
	constexpr uint16 kLightManagerShaderMaxSpotLights = 512;
	constexpr uint32 kLightManagerShaderMaxClusterRecords = 4000;

	RNDefineMeta(LightManager, Object)

	LightManager::LightManager(uint32 x, uint32 y, uint32 z, float zLogFactor, uint16 maxPackedPointLights, uint16 maxPackedSpotLights) :
		_maxLightsPerCluster(255),
		_maxPackedPointLights(std::max<uint16>(1, std::min<uint16>(maxPackedPointLights, kLightManagerShaderMaxPointLights))),
		_maxPackedSpotLights(std::max<uint16>(1, std::min<uint16>(maxPackedSpotLights, kLightManagerShaderMaxSpotLights))),
		_pointLightBuffer(nullptr),
		_spotLightBuffer(nullptr),
		_clusterIndexBuffer(nullptr),
		_clusterRecordsBuffer(nullptr),
		_hasSpotClusterBoundsCache(false)
	{
		SetClusterGridInfo(x, y, z, zLogFactor);
		_grid.zFirstSliceDepth = 3.0f;
	}

	LightManager::~LightManager()
	{
		SafeRelease(_pointLightBuffer);
		SafeRelease(_spotLightBuffer);
		SafeRelease(_clusterIndexBuffer);
		SafeRelease(_clusterRecordsBuffer);
	}

	void LightManager::SetClusterGridInfo(uint32 x, uint32 y, uint32 z, float zLogFactor)
	{
		_grid.clustersX = std::max<uint32>(1, x);
		_grid.clustersY = std::max<uint32>(1, y);
		_grid.clustersZ = std::max<uint32>(1, z);
		_grid.zLogFactor = std::clamp(zLogFactor, 0.0f, 1.0f);
	}

	void LightManager::EnsureBufferCapacity(GPUBuffer *&buffer, size_t capacity)
	{
		if(buffer && buffer->GetLength() >= capacity) return;
		SafeRelease(buffer);
		buffer = Renderer::GetActiveRenderer()->CreateBufferWithLength(capacity, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::WriteOnly, true);
	}

	void LightManager::SetZLogFactor(float zLogFactor)
	{
		_grid.zLogFactor = std::clamp(zLogFactor, 0.0f, 1.0f);
	}

	void LightManager::SetZFirstSliceDepth(float meters)
	{
		_grid.zFirstSliceDepth = std::max(0.0f, meters);
	}

	void LightManager::SetMaxLightsPerCluster(uint16 max)
	{
		_maxLightsPerCluster = std::max<uint16>(1, std::min<uint16>(max, static_cast<uint16>(255)));
	}

	void LightManager::SetMaxPackedLights(uint16 maxPointLights, uint16 maxSpotLights)
	{
		const uint16 clampedPoint = std::max<uint16>(1, std::min<uint16>(maxPointLights, kLightManagerShaderMaxPointLights));
		const uint16 clampedSpot = std::max<uint16>(1, std::min<uint16>(maxSpotLights, kLightManagerShaderMaxSpotLights));
		_maxPackedPointLights = clampedPoint;
		_maxPackedSpotLights = clampedSpot;
	}

	LightManager::DrawSnapshot LightManager::BuildDrawSnapshot(BuildInput &&input)
	{
		RN_PROFILE_SCOPE();
		const ClusterGridInfo &previousGrid = _buildInput.grid;
		const ClusterGridInfo &grid = input.grid;
		if(previousGrid.clustersX != grid.clustersX || previousGrid.clustersY != grid.clustersY || previousGrid.clustersZ != grid.clustersZ ||
		   previousGrid.zLogFactor != grid.zLogFactor || previousGrid.zFirstSliceDepth != grid.zFirstSliceDepth ||
		   !Math::Compare(previousGrid.clipNear, grid.clipNear) || !Math::Compare(previousGrid.clipFar, grid.clipFar))
		{
			_hasSpotClusterBoundsCache = false;
		}

		_buildInput = std::move(input);
		BuildClusters();
		UploadBuffers();
		return GetDrawSnapshot();
	}

	LightManager::BuildInput LightManager::CaptureBuildInput(const Camera *camera, const std::vector<Light *> &lights) const
	{
		BuildInput input;
		input.grid = _grid;
		input.grid.clipNear = camera->GetClipNear();
		input.grid.clipFar = camera->GetReferenceFar();
		input.maxLightsPerCluster = _maxLightsPerCluster;

		const Array *multiview = camera->GetMultiviewCameras();
		const size_t viewCount = (multiview && multiview->GetCount() > 0) ? multiview->GetCount() : 1;
		input.views.reserve(viewCount);
		input.projections.reserve(viewCount);
		input.viewPositions.reserve(viewCount);
		for(size_t i = 0; i < viewCount; ++i)
		{
			const Camera *eye = (multiview && multiview->GetCount() > 0) ? multiview->GetObjectAtIndex<Camera>(i) : camera;
			if(!eye) continue;
			input.views.push_back(eye->GetViewMatrix());
			input.projections.push_back(eye->GetProjectionMatrix());
			input.viewPositions.push_back(eye->GetRenderPosition());
		}

		const PositionType &renderOrigin = camera->GetRenderOrigin();
		input.pointLights.reserve(lights.size());
		input.spotLights.reserve(lights.size());
		input.spotLightCullData.reserve(lights.size());
		for(const Light *light : lights)
		{
			Light::Type type = light->GetType();
			if(type == Light::Type::DirectionalLight) continue; // not clustered

			if(type == Light::Type::SpotLight)
			{
				if(input.spotLights.size() >= _maxPackedSpotLights) continue;
				Vector3 pos(light->GetWorldPosition() - renderOrigin);
				Vector3 dir = light->GetForward();
				SpotLightPacked out;
				out.positionRange = Vector4(pos.x, pos.y, pos.z, light->GetRange());
				out.color = light->GetFinalColor();
				out.dirCos = Vector4(dir.x, dir.y, dir.z, light->GetAngleCos());
				input.spotLights.push_back(out);

				const Sphere cullSphere = light->GetBoundingSphere();
				SpotLightCullData cullData;
				cullData.center = Vector3(cullSphere.position - renderOrigin) + cullSphere.offset;
				cullData.radius = cullSphere.radius;
				cullData.forward = dir;
				cullData.tanHalfAngle = light->GetTanHalfAngle();
				input.spotLightCullData.push_back(cullData);
			}
			else // Point
			{
				if(input.pointLights.size() >= _maxPackedPointLights) continue;
				Vector3 pos(light->GetWorldPosition() - renderOrigin);
				PointLightPacked out;
				out.positionRange = Vector4(pos.x, pos.y, pos.z, light->GetRange());
				out.color = light->GetFinalColor();
				input.pointLights.push_back(out);
			}
		}
		return input;
	}

	float LightManager::ComputeZSlice(float viewZ) const
	{
		const uint32 slices = _buildInput.grid.clustersZ;
		const float n = _buildInput.grid.clipNear;
		const float f = _buildInput.grid.clipFar;
		const float z = std::clamp(viewZ, n, f);

		if(_buildInput.grid.zFirstSliceDepth > 0.0f)
		{
			const float firstEnd = std::min(n + _buildInput.grid.zFirstSliceDepth, f);
			if(z <= firstEnd) return 0.0f;

			const float uLinear = (z - firstEnd) / std::max(1e-6f, (f - firstEnd));
			const float uLog = (log2f(std::max(z / std::max(firstEnd, 1e-6f), 1.0f)) / std::max(log2f(std::max(f / std::max(firstEnd, 1e-6f), 1.0f)), 1e-6f));
			const float u = std::lerp(uLinear, uLog, std::clamp(_buildInput.grid.zLogFactor, 0.0f, 1.0f));
			// Shader: 1 + u * (slices-1)
			const float sliceF = 1.0f + u * float(std::max<int>(1, int(slices) - 1));
			return std::clamp(floorf(sliceF), 0.0f, float(slices - 1));
		}

		const float uLinear = (z - n) / std::max(1e-6f, (f - n));
		const float uLog = (log2f(std::max(z / std::max(n, 1e-6f), 1.0f)) /
							std::max(log2f(std::max(f / std::max(n, 1e-6f), 1.0f)), 1e-6f));
		const float u = std::lerp(uLinear, uLog, std::clamp(_buildInput.grid.zLogFactor, 0.0f, 1.0f));
		// Shader: u * slices
		const float sliceF = u * float(slices);
		return std::clamp(floorf(sliceF), 0.0f, float(slices - 1));
	}

	void LightManager::BuildClusters()
	{
		RN_PROFILE_SCOPE();
		uint32 clusterCount = ComputeClusterCount();
		const auto &views = _buildInput.views;
		const auto &projs = _buildInput.projections;
		const auto &viewPositions = _buildInput.viewPositions;
		const size_t viewCount = views.size();
		std::vector<Matrix> viewProjs;
		viewProjs.reserve(viewCount);
		for(size_t vi = 0; vi < viewCount; ++vi)
		{
			viewProjs.push_back(projs[vi] * views[vi]);
		}
		std::vector<float> projAbsX(viewCount);
		std::vector<float> projAbsY(viewCount);
		for(size_t vi = 0; vi < viewCount; ++vi)
		{
			projAbsX[vi] = std::abs(projs[vi].m[0]);
			projAbsY[vi] = std::abs(projs[vi].m[5]);
		}

		const float zNear = _buildInput.grid.clipNear;
		const float zFar = _buildInput.grid.clipFar;
		uint32 tilesX = _buildInput.grid.clustersX;
		uint32 tilesY = _buildInput.grid.clustersY;
		uint32 tilesZ = _buildInput.grid.clustersZ;
		// Inflate projected bounds by half a tile in NDC to reduce precision/quantization under-coverage.
		const float ndcPadX = 1.0f / float(std::max(1u, tilesX));
		const float ndcPadY = 1.0f / float(std::max(1u, tilesY));
		const float invTilesX = 1.0f / float(std::max(1u, tilesX));
		const float invTilesY = 1.0f / float(std::max(1u, tilesY));
		const std::vector<SpotClusterBound> *clusterConeBoundsByEye = nullptr;
		if(!_buildInput.spotLights.empty())
		{
			bool projectionsMatch = (_cachedSpotBoundsProjections.size() == viewCount);
			if(projectionsMatch)
			{
				for(size_t vi = 0; vi < viewCount; ++vi)
				{
					for(size_t mi = 0; mi < 16; ++mi)
					{
						if(!Math::Compare(_cachedSpotBoundsProjections[vi].m[mi], projs[vi].m[mi]))
						{
							projectionsMatch = false;
							break;
						}
					}
					if(!projectionsMatch) break;
				}
			}
			const bool canUseSpotBoundsCache =
			_hasSpotClusterBoundsCache &&
			projectionsMatch;

			auto solveDepthFromU = [&](float nearDepth, float farDepth, float u) {
				u = std::clamp(u, 0.0f, 1.0f);
				const float logDenominator = std::max(log2f(std::max(farDepth / std::max(nearDepth, 1e-6f), 1.0f)), 1e-6f);
				float low = nearDepth;
				float high = farDepth;
				for(uint32 iteration = 0; iteration < 18; ++iteration)
				{
					float mid = 0.5f * (low + high);
					float uLinear = (mid - nearDepth) / std::max(1e-6f, (farDepth - nearDepth));
					float uLog = log2f(std::max(mid / std::max(nearDepth, 1e-6f), 1.0f)) / logDenominator;
					float mixedU = std::lerp(uLinear, uLog, std::clamp(_buildInput.grid.zLogFactor, 0.0f, 1.0f));
					if(mixedU < u)
						low = mid;
					else
						high = mid;
				}
				return 0.5f * (low + high);
			};

			if(!canUseSpotBoundsCache)
			{
				std::vector<float> zSliceNearDepth(tilesZ);
				std::vector<float> zSliceFarDepth(tilesZ);
				if(_buildInput.grid.zFirstSliceDepth > 0.0f)
				{
					const float firstEnd = std::min(zNear + _buildInput.grid.zFirstSliceDepth, zFar);
					zSliceNearDepth[0] = zNear;
					zSliceFarDepth[0] = firstEnd;

					const float remainingNear = std::max(firstEnd, zNear + 1e-6f);
					if(tilesZ > 1)
					{
						const float remainingSlices = float(std::max(1u, tilesZ - 1u));
						for(uint32 z = 1; z < tilesZ; ++z)
						{
							const float u0 = float(z - 1u) / remainingSlices;
							const float u1 = float(z) / remainingSlices;
							zSliceNearDepth[z] = solveDepthFromU(remainingNear, zFar, u0);
							zSliceFarDepth[z] = solveDepthFromU(remainingNear, zFar, u1);
						}
					}
				}
				else
				{
					const float slicesF = float(std::max(1u, tilesZ));
					for(uint32 z = 0; z < tilesZ; ++z)
					{
						const float u0 = float(z) / slicesF;
						const float u1 = float(z + 1u) / slicesF;
						zSliceNearDepth[z] = solveDepthFromU(zNear, zFar, u0);
						zSliceFarDepth[z] = solveDepthFromU(zNear, zFar, u1);
					}
				}

				const uint32 cornerCols = tilesX + 1u;
				const uint32 cornerRows = tilesY + 1u;
				const size_t cornersPerEye = static_cast<size_t>(cornerCols) * static_cast<size_t>(cornerRows);
				std::vector<Vector3> tileCornerRays(static_cast<size_t>(viewCount) * cornersPerEye);
				auto cornerIndex = [&](size_t eye, uint32 cx, uint32 cy) -> size_t {
					return eye * cornersPerEye + static_cast<size_t>(cy) * static_cast<size_t>(cornerCols) + static_cast<size_t>(cx);
				};

				for(size_t vi = 0; vi < viewCount; ++vi)
				{
					const Matrix invProj = projs[vi].GetInverse();
					for(uint32 cy = 0; cy <= tilesY; ++cy)
					{
						const float ndcY = std::clamp(2.0f * (float(cy) * invTilesY) - 1.0f, -1.0f, 1.0f);
						for(uint32 cx = 0; cx <= tilesX; ++cx)
						{
							const float ndcX = std::clamp(2.0f * (float(cx) * invTilesX) - 1.0f, -1.0f, 1.0f);
							const Vector4 viewPoint4 = invProj * Vector4(ndcX, ndcY, 1.0f, 1.0f);
							const float invW = 1.0f / std::max(std::abs(viewPoint4.w), 1e-6f);
							Vector3 rayVS(viewPoint4.x * invW, viewPoint4.y * invW, viewPoint4.z * invW);
							const float invDepth = 1.0f / std::max(std::abs(rayVS.z), 1e-6f);
							rayVS *= invDepth;
							if(rayVS.z > 0.0f) rayVS *= -1.0f;
							tileCornerRays[cornerIndex(vi, cx, cy)] = rayVS;
						}
					}
				}

				_cachedSpotClusterBoundsByEye.resize(static_cast<size_t>(viewCount) * static_cast<size_t>(clusterCount));
				for(size_t vi = 0; vi < viewCount; ++vi)
				{
					const size_t eyeRayBase = vi * cornersPerEye;
					const size_t eyeClusterBase = vi * static_cast<size_t>(clusterCount);
					for(uint32 z = 0; z < tilesZ; ++z)
					{
						const float clusterDepthNear = zSliceNearDepth[z];
						const float clusterDepthFar = zSliceFarDepth[z];
						const uint32 zBase = z * tilesY * tilesX;
						for(uint32 y = 0; y < tilesY; ++y)
						{
							const uint32 yBase = zBase + y * tilesX;
							for(uint32 x = 0; x < tilesX; ++x)
							{
								const size_t corner00 = eyeRayBase + static_cast<size_t>(y) * static_cast<size_t>(cornerCols) + static_cast<size_t>(x);
								const size_t corner10 = eyeRayBase + static_cast<size_t>(y) * static_cast<size_t>(cornerCols) + static_cast<size_t>(x + 1u);
								const size_t corner01 = eyeRayBase + static_cast<size_t>(y + 1u) * static_cast<size_t>(cornerCols) + static_cast<size_t>(x);
								const size_t corner11 = eyeRayBase + static_cast<size_t>(y + 1u) * static_cast<size_t>(cornerCols) + static_cast<size_t>(x + 1u);

								const Vector3 &ray00 = tileCornerRays[corner00];
								const Vector3 &ray10 = tileCornerRays[corner10];
								const Vector3 &ray01 = tileCornerRays[corner01];
								const Vector3 &ray11 = tileCornerRays[corner11];

								const Vector3 near00 = ray00 * clusterDepthNear;
								const Vector3 near10 = ray10 * clusterDepthNear;
								const Vector3 near01 = ray01 * clusterDepthNear;
								const Vector3 near11 = ray11 * clusterDepthNear;
								const Vector3 far00 = ray00 * clusterDepthFar;
								const Vector3 far10 = ray10 * clusterDepthFar;
								const Vector3 far01 = ray01 * clusterDepthFar;
								const Vector3 far11 = ray11 * clusterDepthFar;

								float clusterMinX = std::min(std::min(near00.x, near10.x), std::min(near01.x, near11.x));
								clusterMinX = std::min(clusterMinX, std::min(std::min(far00.x, far10.x), std::min(far01.x, far11.x)));
								float clusterMaxX = std::max(std::max(near00.x, near10.x), std::max(near01.x, near11.x));
								clusterMaxX = std::max(clusterMaxX, std::max(std::max(far00.x, far10.x), std::max(far01.x, far11.x)));
								float clusterMinYEye = std::min(std::min(near00.y, near10.y), std::min(near01.y, near11.y));
								clusterMinYEye = std::min(clusterMinYEye, std::min(std::min(far00.y, far10.y), std::min(far01.y, far11.y)));
								float clusterMaxYEye = std::max(std::max(near00.y, near10.y), std::max(near01.y, near11.y));
								clusterMaxYEye = std::max(clusterMaxYEye, std::max(std::max(far00.y, far10.y), std::max(far01.y, far11.y)));
								float clusterMinZEye = std::min(std::min(near00.z, near10.z), std::min(near01.z, near11.z));
								clusterMinZEye = std::min(clusterMinZEye, std::min(std::min(far00.z, far10.z), std::min(far01.z, far11.z)));
								float clusterMaxZEye = std::max(std::max(near00.z, near10.z), std::max(near01.z, near11.z));
								clusterMaxZEye = std::max(clusterMaxZEye, std::max(std::max(far00.z, far10.z), std::max(far01.z, far11.z)));

								const Vector3 center((clusterMinX + clusterMaxX) * 0.5f, (clusterMinYEye + clusterMaxYEye) * 0.5f, (clusterMinZEye + clusterMaxZEye) * 0.5f);
								const Vector3 halfExtents((clusterMaxX - clusterMinX) * 0.5f, (clusterMaxYEye - clusterMinYEye) * 0.5f, (clusterMaxZEye - clusterMinZEye) * 0.5f);
								const uint32 clusterIndex = yBase + x;
								_cachedSpotClusterBoundsByEye[eyeClusterBase + static_cast<size_t>(clusterIndex)] = {center, halfExtents.GetLength()};
							}
						}
					}
				}

				_cachedSpotBoundsProjections = projs;
				_hasSpotClusterBoundsCache = true;
			}

			clusterConeBoundsByEye = &_cachedSpotClusterBoundsByEye;
		}

		auto screenToCluster = [&](float ndcX, float ndcY, uint32 &cx, uint32 &cy) {
			const float tileX = (ndcX * 0.5f + 0.5f) * float(tilesX);
			const float tileY = (ndcY * 0.5f + 0.5f) * float(tilesY);
			cx = std::clamp(uint32(tileX), 0u, tilesX - 1u);
			cy = std::clamp(uint32(tileY), 0u, tilesY - 1u);
		};
		struct ClusterSpan
		{
			uint32 zMin, zMax;
			uint32 x0, y0, x1, y1;
		};
		auto computeClusterSpan = [&](const Vector3 &center, float depthRadius, auto &&computeProjectedRadius) -> ClusterSpan {
			float minNdcX = 1.0f;
			float minNdcY = 1.0f;
			float maxNdcX = -1.0f;
			float maxNdcY = -1.0f;
			float minZDepth = zFar;
			float maxZDepth = zNear;
			bool hadProjected = false;
			for(size_t vi = 0; vi < viewCount; ++vi)
			{
				const Vector4 centerVSv = views[vi] * Vector4(center, 1.0f);
				const float depthv = -centerVSv.z;
				const float minZv = std::clamp(depthv - depthRadius, zNear, zFar);
				const float maxZv = std::clamp(depthv + depthRadius, zNear, zFar);
				minZDepth = std::min(minZDepth, minZv);
				maxZDepth = std::max(maxZDepth, maxZv);

				const Vector4 centerCSv = viewProjs[vi] * Vector4(center, 1.0f);
				if(centerCSv.w > 0.0f && depthv > 1e-6f)
				{
					const float centerNdcXv = centerCSv.x / centerCSv.w;
					const float centerNdcYv = centerCSv.y / centerCSv.w;
					float rNdcXv, rNdcYv;
					computeProjectedRadius(vi, depthv, rNdcXv, rNdcYv);
					const float ndcMinXv = std::max(-1.0f, centerNdcXv - rNdcXv);
					const float ndcMaxXv = std::min(1.0f, centerNdcXv + rNdcXv);
					const float ndcMinYv = std::max(-1.0f, centerNdcYv - rNdcYv);
					const float ndcMaxYv = std::min(1.0f, centerNdcYv + rNdcYv);
					minNdcX = std::min(minNdcX, ndcMinXv);
					maxNdcX = std::max(maxNdcX, ndcMaxXv);
					minNdcY = std::min(minNdcY, ndcMinYv);
					maxNdcY = std::max(maxNdcY, ndcMaxYv);
					hadProjected = true;
				}
			}

			if(!hadProjected)
			{
				minNdcX = -1.0f;
				maxNdcX = 1.0f;
				minNdcY = -1.0f;
				maxNdcY = 1.0f;
			}
			minNdcX = std::max(-1.0f, minNdcX - ndcPadX);
			maxNdcX = std::min(1.0f, maxNdcX + ndcPadX);
			minNdcY = std::max(-1.0f, minNdcY - ndcPadY);
			maxNdcY = std::min(1.0f, maxNdcY + ndcPadY);

			ClusterSpan span {};
			span.zMin = uint32(ComputeZSlice(minZDepth));
			span.zMax = uint32(ComputeZSlice(maxZDepth));
			if(span.zMax < span.zMin) std::swap(span.zMin, span.zMax);

			screenToCluster(minNdcX, minNdcY, span.x0, span.y0);
			screenToCluster(maxNdcX, maxNdcY, span.x1, span.y1);
			if(span.x1 < span.x0) std::swap(span.x0, span.x1);
			if(span.y1 < span.y0) std::swap(span.y0, span.y1);
			return span;
		};
		auto computePointClusterSpan = [&](const Vector3 &position, float range) -> ClusterSpan {
			const float lightRadiusSq = range * range;
			bool cameraInsideLight = false;
			float maxInfluenceDepth = 0.0f;
			for(const Vector3 &viewPosition : viewPositions)
			{
				const float distanceToCamera = position.GetDistance(viewPosition);
				maxInfluenceDepth = std::max(maxInfluenceDepth, distanceToCamera + range);
				if(position.GetSquaredDistance(viewPosition) <= lightRadiusSq)
				{
					cameraInsideLight = true;
				}
			}

			if(cameraInsideLight)
			{
				// The tangent-silhouette formula below assumes the eye is outside the sphere.
				// If any eye is inside, the sphere subtends all screen directions.
				ClusterSpan span {};
				span.x0 = 0;
				span.y0 = 0;
				span.x1 = tilesX - 1;
				span.y1 = tilesY - 1;
				span.zMin = 0;
				const float clampedDepth = std::clamp(maxInfluenceDepth, zNear, zFar);
				span.zMax = uint32(ComputeZSlice(clampedDepth));
				return span;
			}

			return computeClusterSpan(position, range, [&](size_t vi, float depthv, float &rNdcXv, float &rNdcYv) {
				const float denom = std::max(depthv * depthv - range * range, 1e-6f);
				const float rSil = range / std::sqrt(denom);
				rNdcXv = projAbsX[vi] * rSil;
				rNdcYv = projAbsY[vi] * rSil;
			});
		};

		// Reuse scratch buffers to avoid per-frame per-cluster allocations.
		const size_t pointClusterCapacity = std::min(static_cast<size_t>(_buildInput.maxLightsPerCluster), _buildInput.pointLights.size());
		const size_t spotClusterCapacity = std::min(static_cast<size_t>(_buildInput.maxLightsPerCluster), _buildInput.spotLights.size());
		_clusterPointScratch.resize(static_cast<size_t>(clusterCount) * pointClusterCapacity);
		_clusterSpotScratch.resize(static_cast<size_t>(clusterCount) * spotClusterCapacity);
		_clusterPointCountsScratch.assign(clusterCount, 0);
		_clusterSpotCountsScratch.assign(clusterCount, 0);
		_clusterOffsetsScratch.resize(clusterCount);
		_clusterLightIndices.clear();

		const uint32 sliceStride = tilesX * tilesY;
		const uint32 rowStride = tilesX;
		for(uint32 li = 0; li < _buildInput.pointLights.size(); ++li)
		{
			const PointLightPacked &pl = _buildInput.pointLights[li];
			const Vector3 position(pl.positionRange.x, pl.positionRange.y, pl.positionRange.z);
			const ClusterSpan span = computePointClusterSpan(position, pl.positionRange.w);
			const uint16 lightIndex = static_cast<uint16>(li);
			for(uint32 z = span.zMin; z < span.zMax + 1u; ++z)
			{
				const uint32 zBase = z * sliceStride;
				for(uint32 y = span.y0; y <= span.y1; ++y)
				{
					uint32 idx = zBase + y * rowStride + span.x0;
					for(uint32 x = span.x0; x <= span.x1; ++x, ++idx)
					{
						uint8 &count = _clusterPointCountsScratch[idx];
						if(count >= _buildInput.maxLightsPerCluster) continue;
						_clusterPointScratch[static_cast<size_t>(idx) * pointClusterCapacity + count] = lightIndex;
						++count;
					}
				}
			}
		}

		struct SpotEyeWork
		{
			Vector3 direction;
			Vector3 position;
			bool directionValid = false;
		};
		std::vector<SpotEyeWork> spotEyeWork(_buildInput.spotLights.empty() ? 0 : viewCount);

		for(uint32 li = 0; li < _buildInput.spotLights.size(); ++li)
		{
			const SpotLightPacked &pl = _buildInput.spotLights[li];
			const Vector3 position(pl.positionRange.x, pl.positionRange.y, pl.positionRange.z);
			const SpotLightCullData &cullData = _buildInput.spotLightCullData[li];
			const ClusterSpan span = computeClusterSpan(cullData.center, cullData.radius, [&](size_t vi, float depthv, float &rNdcXv, float &rNdcYv) {
				const float denom = std::max(depthv * depthv - cullData.radius * cullData.radius, 1e-6f);
				const float rSil = cullData.radius / std::sqrt(denom);
				rNdcXv = projAbsX[vi] * rSil;
				rNdcYv = projAbsY[vi] * rSil;
			});
			const float sideExpandFactor = std::sqrt(1.0f + cullData.tanHalfAngle * cullData.tanHalfAngle);

			for(size_t vi = 0; vi < viewCount; ++vi)
			{
				SpotEyeWork &eyeWork = spotEyeWork[vi];
				const Vector4 directionVS4 = views[vi] * Vector4(cullData.forward, 0.0f);
				Vector3 direction(directionVS4.x, directionVS4.y, directionVS4.z);
				const float dirLenSq = direction.GetSquaredLength();
				eyeWork.directionValid = std::isfinite(dirLenSq) && dirLenSq > 1e-12f;
				if(eyeWork.directionValid)
				{
					direction *= 1.0f / std::sqrt(dirLenSq);
					eyeWork.direction = direction;
				}
				const Vector4 positionVS4 = views[vi] * Vector4(position, 1.0f);
				eyeWork.position = Vector3(positionVS4.x, positionVS4.y, positionVS4.z);
			}
			const uint16 lightIndex = static_cast<uint16>(li);
			for(uint32 z = span.zMin; z < span.zMax + 1u; ++z)
			{
				const uint32 zBase = z * sliceStride;
				for(uint32 y = span.y0; y <= span.y1; ++y)
				{
					uint32 idx = zBase + y * rowStride + span.x0;
					for(uint32 x = span.x0; x <= span.x1; ++x, ++idx)
					{
						bool passesAnyEye = false;
						for(size_t vi = 0; vi < viewCount; ++vi)
						{
							const SpotEyeWork &eyeWork = spotEyeWork[vi];
							if(!eyeWork.directionValid)
							{
								passesAnyEye = true;
								break;
							}

							const SpotClusterBound &clusterBounds = (*clusterConeBoundsByEye)[vi * static_cast<size_t>(clusterCount) + idx];
							const Vector3 toCluster = clusterBounds.center - eyeWork.position;
							const float axial = eyeWork.direction.GetDotProduct(toCluster);
							if(!std::isfinite(axial))
							{
								passesAnyEye = true;
								break;
							}
							if(axial < -clusterBounds.radius || axial > pl.positionRange.w + clusterBounds.radius) continue;

							const float radialSq = (toCluster - eyeWork.direction * axial).GetSquaredLength();
							if(!std::isfinite(radialSq))
							{
								passesAnyEye = true;
								break;
							}

							const float coneDistance = (axial > pl.positionRange.w) ? pl.positionRange.w : std::max(axial, 0.0f);
							const float radialLimit = coneDistance * cullData.tanHalfAngle + clusterBounds.radius * sideExpandFactor;
							const float radialLimitSq = radialLimit * radialLimit;
							if(!std::isfinite(radialLimitSq))
							{
								passesAnyEye = true;
								break;
							}
							if(radialSq <= radialLimitSq)
							{
								passesAnyEye = true;
								break;
							}
						}
						if(!passesAnyEye) continue;

						uint8 &count = _clusterSpotCountsScratch[idx];
						if(count >= _buildInput.maxLightsPerCluster) continue;
						_clusterSpotScratch[static_cast<size_t>(idx) * spotClusterCapacity + count] = lightIndex;
						++count;
					}
				}
			}
		}

		// Build per-cluster counts and offsets
		uint32 totalCount = 0;
		for(uint32 i = 0; i < clusterCount; ++i)
		{
			const uint8 pcount = _clusterPointCountsScratch[i];
			const uint8 scount = _clusterSpotCountsScratch[i];
			_clusterOffsetsScratch[i] = totalCount;
			totalCount += static_cast<uint32>(pcount) + static_cast<uint32>(scount);
		}

		// Flatten indices in cluster order
		_clusterLightIndices.resize(totalCount);
		for(uint32 i = 0, offset = 0; i < clusterCount; ++i)
		{
			const uint8 pcount = _clusterPointCountsScratch[i];
			const uint8 scount = _clusterSpotCountsScratch[i];
			if(pcount)
			{
				const size_t src = static_cast<size_t>(i) * pointClusterCapacity;
				memcpy(_clusterLightIndices.data() + offset, _clusterPointScratch.data() + src, static_cast<size_t>(pcount) * sizeof(uint16));
				offset += pcount;
			}
			if(scount)
			{
				const size_t src = static_cast<size_t>(i) * spotClusterCapacity;
				memcpy(_clusterLightIndices.data() + offset, _clusterSpotScratch.data() + src, static_cast<size_t>(scount) * sizeof(uint16));
				offset += scount;
			}
		}

		// Pack records for groups of 6 clusters: 1 base offset + 6 pairs of counts
		uint32 groupCount = (clusterCount + 5u) / 6u;
		_clusterRecords.resize(groupCount);
		for(uint32 g = 0; g < groupCount; ++g)
		{
			uint32 base = g * 6u;
			ClusterRecord rec {};
			rec.offset = (base < clusterCount) ? _clusterOffsetsScratch[base] : 0u;
			auto packPair = [&](uint32 idxInGroup) -> uint32 {
				uint32 ci = base + idxInGroup;
				uint32 p = (ci < clusterCount) ? _clusterPointCountsScratch[ci] : 0u;
				uint32 s = (ci < clusterCount) ? _clusterSpotCountsScratch[ci] : 0u;
				return (p & 0xffu) | ((s & 0xffu) << 8);
			};
			rec.counts01 = packPair(0) | (packPair(1) << 16);
			rec.counts23 = packPair(2) | (packPair(3) << 16);
			rec.counts45 = packPair(4) | (packPair(5) << 16);
			_clusterRecords[g] = rec;
		}
	}

	void LightManager::UploadBuffers()
	{
		size_t pointBytes = std::max<size_t>(_buildInput.pointLights.size(), kLightManagerShaderMaxPointLights) * sizeof(PointLightPacked);
		size_t spotBytes = std::max<size_t>(_buildInput.spotLights.size(), kLightManagerShaderMaxSpotLights) * sizeof(SpotLightPacked);
		size_t indexBytes = _clusterLightIndices.size() * sizeof(uint16);
		size_t headerBytes = sizeof(ClusterGridInfo);
		size_t recordsBytes = _clusterRecords.size() * sizeof(ClusterRecord);

		// Keep shader-declared light/record sizes and index capacity stable across visibility changes.
		EnsureBufferCapacity(_pointLightBuffer, pointBytes);
		EnsureBufferCapacity(_spotLightBuffer, spotBytes);
		EnsureBufferCapacity(_clusterIndexBuffer, static_cast<size_t>(ComputeClusterCount()) * 2u * _buildInput.maxLightsPerCluster * sizeof(uint16));
		EnsureBufferCapacity(_clusterRecordsBuffer, headerBytes + kLightManagerShaderMaxClusterRecords * sizeof(ClusterRecord));

		auto uploadLights = [](GPUBuffer *buffer, const auto &lights, size_t bufferSize) {
			size_t used = lights.size() * sizeof(lights[0]);
			if(used > 0) memcpy(buffer->GetBuffer(), lights.data(), used);
			buffer->FlushRange(Range(0, bufferSize));
		};
		uploadLights(_pointLightBuffer, _buildInput.pointLights, pointBytes);
		uploadLights(_spotLightBuffer, _buildInput.spotLights, spotBytes);

		if(indexBytes > 0)
		{
			void *dst = _clusterIndexBuffer->GetBuffer();
			memcpy(dst, _clusterLightIndices.data(), indexBytes);
			_clusterIndexBuffer->FlushRange(Range(0, indexBytes));
		}

		if(recordsBytes > 0)
		{
			void *dst = _clusterRecordsBuffer->GetBuffer();
			// Write header
			memcpy(dst, &_buildInput.grid, headerBytes);
			memcpy(static_cast<uint8 *>(dst) + headerBytes, _clusterRecords.data(), recordsBytes);
			_clusterRecordsBuffer->FlushRange(Range(0, headerBytes + recordsBytes));
		}
	}
} // namespace RN
