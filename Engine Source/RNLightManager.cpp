//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNLightManager.h"
#include "RNLight.h"
#include "RNThreadPool.h"

#define kRNLightManagerLightListOffsetCountIndex 0
#define kRNLightManagerLightListIndicesIndex 1
#define kRNLightManagerLightListPointDataIndex 2
#define kRNLightManagerLightListSpotDataIndex 3

namespace RN
{
	LightManager::LightManager()
	{
		_maxLightFastPath = 10;
		
		_lightIndices = nullptr;
		_lightOffsetCount = nullptr;
		
		_lightIndicesSize = 0;
		_lightOffsetCountSize = 0;
		
		// Point and Spot lights
		_lightPointDataSize = 0;
		_lightSpotDataSize = 0;
		
		gl::GenTextures(4, _lightTextures);
		gl::GenBuffers(4, _lightBuffers);
		
		// light offsets and counts
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListOffsetCountIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		
		// Light indices
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListIndicesIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		
		// Point Light Data
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListPointDataIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListPointDataIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[kRNLightManagerLightListPointDataIndex]);
		
		// Spot Light Data
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListSpotDataIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
	}
	
	LightManager::~LightManager()
	{
		gl::DeleteBuffers(4, _lightBuffers);
		gl::DeleteTextures(4, _lightTextures);
		
		delete _lightIndices;
		delete _lightOffsetCount;
	}
	
	
	void LightManager::AddLight(Light *light)
	{
		switch(light->GetType())
		{
			case Light::Type::PointLight:
				_pointLights.push_back(light);
				break;
				
			case Light::Type::SpotLight:
				_spotLights.push_back(light);
				break;
				
			case Light::Type::DirectionalLight:
				if(light->Shadow())
					_directionalLights.push_front(light);
				else
					_directionalLights.push_back(light);
				break;
				
			default:
				break;
		}
	}
	

	int LightManager::CreatePointSpotLightLists(Camera *camera)
	{
		size_t pointSpotCount = 0;
		
		CullLights(camera);
		
		//Point lights
		{
			Light **lights = _pointLights.data();
			size_t lightCount = _pointLights.size();
			pointSpotCount += lightCount;
			
			_lightPointPosition.clear();
			_lightPointColor.clear();
			_lightPointDepth.clear();
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = nullptr;
			size_t lightDataSize = lightCount * 2 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
			{
				lightDataSize = 2 * sizeof(Vector4);
				
				if(_lightPointData.size() == 0)
					_lightPointData.resize(2);
			}
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListPointDataIndex]);
			if(lightDataSize > _lightPointDataSize)
			{
				gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				_lightPointDataSize = lightDataSize;
			}
			
			if(_lightPointData.size() < lightCount * 2)
				_lightPointData.resize(lightCount * 2);
			
			lightData = _lightPointData.data();
			
			for(size_t i = 0; i < lightCount; i ++)
			{
				Light *light = lights[i];
				const Vector3& position = light->GetWorldPosition();
				const Vector3& color    = light->GetResultColor();
				const float range = light->GetRange();
				const float shadow = light->Shadow()?static_cast<float>(i):-1.0f;
				
				if(i < _maxLightFastPath)
				{
					_lightPointPosition.emplace_back(Vector4(position, range));
					_lightPointColor.emplace_back(Vector4(color, shadow));
				}
				
				lightData[i * 2 + 0] = std::move(Vector4(position, range));
				lightData[i * 2 + 1] = std::move(Vector4(color, shadow));
				
				if(light->Shadow())
				{
					if(light->GetShadowCamera())
					{
						_lightPointDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
			
			gl::BufferSubData(GL_TEXTURE_BUFFER, 0, lightDataSize, lightData);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		
		//Spot lights
		{
			Light **lights = _spotLights.data();
			size_t lightCount = _spotLights.size();
			pointSpotCount += lightCount;
			
			_lightSpotPosition.clear();
			_lightSpotDirection.clear();
			_lightSpotColor.clear();
			_lightSpotDepth.clear();
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = nullptr;
			size_t lightDataSize = lightCount * 4 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
			{
				lightDataSize = 3 * sizeof(Vector4);
				
				if(_lightSpotData.size() == 0)
					_lightSpotData.resize(3);
			}
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
			if(lightDataSize > _lightSpotDataSize)
			{
				gl::BufferData(GL_TEXTURE_BUFFER, _lightSpotDataSize, 0, GL_DYNAMIC_DRAW);
				gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				
				_lightSpotDataSize = lightDataSize;
			}
			
			if(_lightSpotData.size() < lightCount * 3)
				_lightSpotData.resize(lightCount * 3);
			
			lightData = _lightSpotData.data();
			
			for(size_t i = 0; i < lightCount; i ++)
			{
				Light *light = lights[i];
				const Vector3& position = light->GetWorldPosition();
				const Vector3& color    = light->GetResultColor();
				const Vector3& direction = -light->Forward();
				const float angle = light->GetAngle();
				const float range = light->GetRange();
				const float shadow = light->Shadow()?static_cast<float>(i):-1.0f;
				
				if(i < _maxLightFastPath)
				{
					_lightSpotPosition.emplace_back(Vector4(position, range));
					_lightSpotDirection.emplace_back(Vector4(direction, angle));
					_lightSpotColor.emplace_back(Vector4(color, shadow));
				}
				
				lightData[i * 3 + 0] = Vector4(position, range);
				lightData[i * 3 + 1] = Vector4(color, shadow);
				lightData[i * 3 + 2] = Vector4(direction, angle);
				
				if(light->Shadow())
				{
					if(light->GetShadowCamera())
					{
						_lightSpotDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
			
			gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, lightData, GL_DYNAMIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		
		_pointLights.clear();
		_spotLights.clear();
		
		return static_cast<int>(pointSpotCount);
	}
	
	
	int LightManager::CreateDirectionalLightList(Camera *camera)
	{
		size_t lightCount = _directionalLights.size();
		
		_lightDirectionalDirection.clear();
		_lightDirectionalColor.clear();
		_lightDirectionalMatrix.clear();
		_lightDirectionalDepth.clear();
		
		for(size_t i=0; i<lightCount; i++)
		{
			Light *light = _directionalLights[i];
			const Vector3& color = light->GetResultColor();
			const Vector3& direction = -light->Forward();
			
			_lightDirectionalDirection.push_back(direction);
			_lightDirectionalColor.emplace_back(Vector4(color, light->Shadow() ? static_cast<float>(i):-1.0f));
			
			if(light->Shadow())
			{
				const std::vector<Matrix> &matrices = light->GetShadowMatrices();
				
				if(matrices.size() > 0)
				{
					for(int i = 0; i < 4; i++)
					{
						_lightDirectionalMatrix.push_back(matrices[i]);
					}
					
					if(light->GetShadowCamera())
					{
						_lightDirectionalDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
					else
					{
						_lightDirectionalDepth.push_back(light->GetShadowCameras()->GetFirstObject<Camera>()->GetStorage()->GetDepthTarget());
					}
				}
			}
		}
		
		_directionalLights.clear();
		
		return static_cast<int>(lightCount);
	}
	
	
	void LightManager::CullLights(Camera *camera)
	{
		Vector3 cameraForward = camera->Forward();
		
		const Vector3& cameraWorldPosition = camera->GetWorldPosition();
		const Vector3& cameraClusterSize = camera->GetLightClusters();
		
		size_t maxLightsPerTile = camera->GetMaxLightsPerTile();
		const Rect& rect = camera->GetFrame();
		
		int tilesWidth   = ceil(rect.width / cameraClusterSize.x);
		int tilesHeight  = ceil(rect.height / cameraClusterSize.y);
		int tilesDepth   = ceil(camera->clipfar / cameraClusterSize.z);
		int clusterCount = tilesWidth * tilesHeight * tilesDepth;
		
		if(_lightIndicesSize < clusterCount * maxLightsPerTile)
		{
			_lightIndicesSize = clusterCount * maxLightsPerTile;
			_lightIndices = new uint16[_lightIndicesSize];
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
			gl::BufferData(GL_TEXTURE_BUFFER, _lightIndicesSize * sizeof(uint16), nullptr, GL_DYNAMIC_DRAW);
		}
		
		if(_lightOffsetCountSize < clusterCount * 3)
		{
			_lightOffsetCountSize = clusterCount * 3;
			_lightOffsetCount = new int32[_lightOffsetCountSize];
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
			gl::BufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int32), nullptr, GL_DYNAMIC_DRAW);
		}
		
		std::fill(_lightOffsetCount, _lightOffsetCount + _lightOffsetCountSize, 0);
		
		uint16 lightIndex = 0;
		
		//Cull point lights
		for(auto light : _pointLights)
		{
			const Vector3& lightPosition = light->GetWorldPosition();
			float lightRange = light->GetRange();

			Vector4 viewPosition = std::move(camera->viewMatrix.Transform(Vector4(lightPosition, 1.0f)));
			viewPosition.w = 1.0f;
			
			float zOffsetMinX = 0.0f;
			float zOffsetMaxX = 0.0f;
			float zOffsetMinY = 0.0f;
			float zOffsetMaxY = 0.0f;
			if(viewPosition.z+camera->clipnear < -lightRange)
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = lightRange;
			}
			else
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = -viewPosition.z-camera->clipnear;
			}
			if(viewPosition.x > lightRange)
			{
				zOffsetMinX *= -1.0f;
			}
			if(viewPosition.x < -lightRange)
			{
				zOffsetMaxX *= -1.0f;
			}
			if(viewPosition.y > lightRange)
			{
				zOffsetMinY *= -1.0f;
			}
			if(viewPosition.y < -lightRange)
			{
				zOffsetMaxY *= -1.0f;
			}
			
			Vector4 minProjectedX = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(-lightRange, 0.0f, zOffsetMinX, 0.0f)));
			Vector4 maxProjectedX = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(lightRange, 0.0f, zOffsetMaxX, 0.0f)));
			minProjectedX /= Math::FastAbs(minProjectedX.w);
			maxProjectedX /= Math::FastAbs(maxProjectedX.w);
			
			Vector4 minProjectedY = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(0.0f, -lightRange, zOffsetMinY, 0.0f)));
			Vector4 maxProjectedY = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(0.0f, lightRange, zOffsetMaxY, 0.0f)));
			minProjectedY /= Math::FastAbs(minProjectedY.w);
			maxProjectedY /= Math::FastAbs(maxProjectedY.w);
			
			int minX = floor((minProjectedX.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			int maxX = ceil((maxProjectedX.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			if(maxX > tilesWidth-1)
				maxX = tilesWidth-1;
			if(minX > tilesWidth-1)
				minX = maxX+1;
			if(minX < 0)
				minX = 0;
			if(maxX < 0)
				maxX = minX-1;
			
			int minY = floor((minProjectedY.y*0.5+0.5)*rect.height/cameraClusterSize.y);
			int maxY = ceil((maxProjectedY.y*0.5+0.5)*rect.height/cameraClusterSize.y);
			if(maxY > tilesHeight-1)
				maxY = tilesHeight-1;
			if(minY > tilesHeight-1)
				minY = maxY+1;
			if(minY < 0)
				minY = 0;
			if(maxY < 0)
				maxY = minY-1;
			
			float linearDist = cameraForward.Dot(lightPosition - cameraWorldPosition);
			int minZ = floor((linearDist - lightRange) / cameraClusterSize.z);
			int maxZ = ceil((linearDist + lightRange) / cameraClusterSize.z);
			if(minZ < 0)
				minZ = 0;
	
			for(int x = minX; x <= maxX; x++)
			{
				for(int y = minY; y <= maxY; y++)
				{
					for(int z = minZ; z <= maxZ; z++)
					{
						int clusterIndex = x * tilesHeight * tilesDepth + y * tilesDepth + z;
						
						if(_lightOffsetCount[clusterIndex * 3 + 1] < maxLightsPerTile)
						{
							_lightIndices[clusterIndex * maxLightsPerTile + _lightOffsetCount[clusterIndex * 3 + 1]] = lightIndex;
							_lightOffsetCount[clusterIndex * 3 + 1] ++;
						}
					}
				}
			}
			
			lightIndex ++;
		}
		
		//Cull spot lights
		lightIndex = 0;
		for(auto light : _spotLights)
		{
			const Vector3& lightPosition = light->GetWorldPosition();
			float lightRange = light->GetRange();
			
			Vector4 viewPosition = std::move(camera->viewMatrix.Transform(Vector4(lightPosition, 1.0f)));
			viewPosition.w = 1.0f;
			
			float zOffsetMinX = 0.0f;
			float zOffsetMaxX = 0.0f;
			float zOffsetMinY = 0.0f;
			float zOffsetMaxY = 0.0f;
			if(viewPosition.z+camera->clipnear < -lightRange)
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = lightRange;
			}
			else
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = -viewPosition.z-camera->clipnear;
			}
			if(viewPosition.x > lightRange)
			{
				zOffsetMinX *= -1.0f;
			}
			if(viewPosition.x < -lightRange)
			{
				zOffsetMaxX *= -1.0f;
			}
			if(viewPosition.y > lightRange)
			{
				zOffsetMinY *= -1.0f;
			}
			if(viewPosition.y < -lightRange)
			{
				zOffsetMaxY *= -1.0f;
			}
			
			Vector4 minProjectedX = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(-lightRange, 0.0f, zOffsetMinX, 0.0f)));
			Vector4 maxProjectedX = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(lightRange, 0.0f, zOffsetMaxX, 0.0f)));
			minProjectedX /= Math::FastAbs(minProjectedX.w);
			maxProjectedX /= Math::FastAbs(maxProjectedX.w);
			
			Vector4 minProjectedY = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(0.0f, -lightRange, zOffsetMinY, 0.0f)));
			Vector4 maxProjectedY = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(0.0f, lightRange, zOffsetMaxY, 0.0f)));
			minProjectedY /= Math::FastAbs(minProjectedY.w);
			maxProjectedY /= Math::FastAbs(maxProjectedY.w);
			
			int minX = floor((minProjectedX.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			int maxX = ceil((maxProjectedX.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			if(maxX > tilesWidth-1)
				maxX = tilesWidth-1;
			if(minX > tilesWidth-1)
				minX = maxX+1;
			if(minX < 0)
				minX = 0;
			if(maxX < 0)
				maxX = minX-1;
			
			int minY = floor((minProjectedY.y*0.5+0.5)*rect.height/cameraClusterSize.y);
			int maxY = ceil((maxProjectedY.y*0.5+0.5)*rect.height/cameraClusterSize.y);
			if(maxY > tilesHeight-1)
				maxY = tilesHeight-1;
			if(minY > tilesHeight-1)
				minY = maxY+1;
			if(minY < 0)
				minY = 0;
			if(maxY < 0)
				maxY = minY-1;
			
			float linearDist = cameraForward.Dot(lightPosition - cameraWorldPosition);
			int minZ = floor((linearDist - lightRange) / cameraClusterSize.z);
			int maxZ = ceil((linearDist + lightRange) / cameraClusterSize.z);
			if(minZ < 0)
				minZ = 0;
			
			for(int x = minX; x <= maxX; x++)
			{
				for(int y = minY; y <= maxY; y++)
				{
					for(int z = minZ; z <= maxZ; z++)
					{
						int clusterIndex = x * tilesHeight * tilesDepth + y * tilesDepth + z;
						
						if(_lightOffsetCount[clusterIndex * 3 + 1]+_lightOffsetCount[clusterIndex * 3 + 2] < maxLightsPerTile)
						{
							_lightIndices[clusterIndex * maxLightsPerTile + _lightOffsetCount[clusterIndex * 3 + 1] + _lightOffsetCount[clusterIndex * 3 + 2]] = lightIndex;
							_lightOffsetCount[clusterIndex * 3 + 2] ++;
						}
					}
				}
			}
			
			lightIndex ++;
		}
	
		_lightOffsetCount[0] = 0;
		for(int c = 1; c < clusterCount; c++)
		{
			_lightOffsetCount[c*3+0] = _lightOffsetCount[(c-1)*3+0]+_lightOffsetCount[(c-1)*3+1] + _lightOffsetCount[(c-1)*3+2];
		}
		
		int offset = 0;
		for(int c = 0; c < clusterCount; c++)
		{
			std::copy(&_lightIndices[c*maxLightsPerTile], &_lightIndices[c*maxLightsPerTile+_lightOffsetCount[c*3+1]+_lightOffsetCount[c*3+2]], &_lightIndices[offset]);
			offset += _lightOffsetCount[c*3+1] + _lightOffsetCount[c*3+2];
		}
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		gl::BufferSubData(GL_TEXTURE_BUFFER, 0, _lightOffsetCountSize * sizeof(int32), _lightOffsetCount);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		gl::BufferSubData(GL_TEXTURE_BUFFER, 0, offset * sizeof(uint16), _lightIndices);
	}
}
