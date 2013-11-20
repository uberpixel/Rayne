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
		_lightIndicesTemp = nullptr;
		_lightOffsetCount = nullptr;
		
		_lightIndicesSize = 0;
		_lightIndicesTempSize = 0;
		_lightOffsetCountSize = 0;
		
		// Point and Spot lights
		_lightPointDataSize = 0;
		_lightSpotDataSize = 0;
		
		glGenTextures(4, _lightTextures);
		glGenBuffers(4, _lightBuffers);
		
		// light offsets and counts
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListOffsetCountIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		
		// Light indices
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListIndicesIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		
		// Point Light Data
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListPointDataIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListPointDataIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[kRNLightManagerLightListPointDataIndex]);
		
		// Spot Light Data
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListSpotDataIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
	}
	
	LightManager::~LightManager()
	{
		glDeleteBuffers(4, _lightBuffers);
		glDeleteTextures(4, _lightTextures);
		
		if(_lightIndices != nullptr)
			delete _lightIndices;
		if(_lightIndicesTemp != nullptr)
			delete _lightIndicesTemp;
		if(_lightOffsetCount != nullptr)
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
	

	bool LightManager::CreateLightLists(Camera *camera)
	{
		CullLights(camera);
		
		//Point lights
		{
			Light **lights = _pointLights.data();
			size_t lightCount = _pointLights.size();
			
			_lightPointPosition.clear();
			_lightPointColor.clear();
			_lightPointDepth.clear();
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = nullptr;
			size_t lightDataSize = lightCount * 2 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
				lightDataSize = 2 * sizeof(Vector4);
			
			glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListPointDataIndex]);
			if(lightDataSize > _lightPointDataSize)
			{
				glBufferData(GL_TEXTURE_BUFFER, _lightPointDataSize, 0, GL_DYNAMIC_DRAW);
				glBufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				
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
			
			glBufferData(GL_TEXTURE_BUFFER, lightDataSize, lightData, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		
		//Spot lights
		{
			Light **lights = _spotLights.data();
			size_t lightCount = _spotLights.size();
			
			_lightSpotPosition.clear();
			_lightSpotDirection.clear();
			_lightSpotColor.clear();
			_lightSpotDepth.clear();
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = nullptr;
			size_t lightDataSize = lightCount * 2 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
				lightDataSize = 2 * sizeof(Vector4);
			
			glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
			if(lightDataSize > _lightPointDataSize)
			{
				glBufferData(GL_TEXTURE_BUFFER, _lightPointDataSize, 0, GL_DYNAMIC_DRAW);
				glBufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				
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
						_lightPointDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
			
			glBufferData(GL_TEXTURE_BUFFER, lightDataSize, lightData, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		
		_pointLights.clear();
		_spotLights.clear();
		
		return true;
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
		Vector3 cameraWorldPosition = camera->GetWorldPosition();
		
		//Vector3 corner1 = camera->ToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		//Vector3 corner2 = camera->ToWorld(Vector3(1.0f, -1.0f, 1.0f));
		//Vector3 corner3 = camera->ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		
		Vector3 cameraClusterSize = camera->GetLightTiles();
		Rect rect = camera->GetFrame();
		int tilesWidth  = ceil(rect.width / cameraClusterSize.x);
		int tilesHeight = ceil(rect.height / cameraClusterSize.y);
		int tilesDepth = ceil(camera->clipfar / cameraClusterSize.z); //TODO: clipnear?
		int clusterCount = tilesWidth * tilesHeight * tilesDepth;
		
		//Vector3 dirX = (corner2 - corner1) / tilesWidth;
		//Vector3 dirY = (corner3 - corner1) / tilesHeight;
		//Vector3 dirZ = cameraForward * camera->clipfar / tilesDepth;
		
		size_t maxLightsPerTile = camera->GetMaxLightsPerTile();
		
		if(_lightIndicesTempSize < tilesWidth * tilesHeight * tilesDepth * maxLightsPerTile)
		{
			_lightIndicesTempSize = tilesWidth * tilesHeight * tilesDepth * maxLightsPerTile;
			_lightIndicesTemp = new int[_lightIndicesTempSize];
		}
		
		if(_lightOffsetCountSize < tilesWidth * tilesHeight * tilesDepth * 2)
		{
			_lightOffsetCountSize = tilesWidth * tilesHeight * tilesDepth * 2;
			_lightOffsetCount = new int[_lightOffsetCountSize];
		}
		
		for(int i = 0; i < _lightOffsetCountSize; i++)
		{
			_lightOffsetCount[i] = 0;
		}
		
		int lightIndex = 0;
		for(auto light : _pointLights)
		{
			Vector3 lightPosition = light->GetWorldPosition();
			float lightRange = light->GetRange();

			Vector4 viewPosition = camera->viewMatrix.Transform(Vector4(lightPosition, 1.0f));
			viewPosition.w = 1.0;
			
			Vector4 minProjected = camera->projectionMatrix.Transform(viewPosition+Vector4(-lightRange, -lightRange, 0.0, 0.0));
			minProjected /= minProjected.w;
			Vector4 maxProjected = camera->projectionMatrix.Transform(viewPosition+Vector4(lightRange, lightRange, 0.0, 0.0));
			maxProjected /= maxProjected.w;
			
			int minX = floor((minProjected.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			int maxX = ceil((maxProjected.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			
			if(maxX >= tilesWidth-1)
				maxX = tilesWidth-1;
			if(minX > tilesWidth-1)
				minX = maxX+1;
			if(minX < 0)
				minX = 0;
			if(maxX < 0)
				maxX = minX-1;
			
			
			int minY = floor((minProjected.y*0.5+0.5)*rect.height/cameraClusterSize.y);
			int maxY = ceil((maxProjected.y*0.5+0.5)*rect.height/cameraClusterSize.y);
			
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
	
			for(int x = minX; x <= maxX; x++)
			{
				for(int y = minY; y <= maxY; y++)
				{
					for(int z = minZ; z <= maxZ; z++)
					{
						int clusterIndex = x * tilesHeight * tilesDepth + y * tilesDepth + z;
						
						if(_lightOffsetCount[clusterIndex * 2 + 1] < maxLightsPerTile)
						{
							_lightIndicesTemp[clusterIndex * maxLightsPerTile + _lightOffsetCount[clusterIndex * 2 + 1]] = lightIndex;
							_lightOffsetCount[clusterIndex * 2 + 1] += 1;
						}
					}
				}
			}
			
			lightIndex++;
		}
		
		if(_lightIndicesSize < clusterCount * maxLightsPerTile)
		{
			_lightIndicesSize = clusterCount * maxLightsPerTile;
			_lightIndices = new int[_lightIndicesSize];
		}
		
		_lightOffsetCount[0] = 0;
		for(int c = 1; c < clusterCount; c++)
		{
			_lightOffsetCount[c*2+0] = _lightOffsetCount[(c-1)*2+0]+_lightOffsetCount[(c-1)*2+1];
		}
		
		for(int c = 0; c < clusterCount; c++)
		{
			std::copy(&_lightIndicesTemp[c*maxLightsPerTile], &_lightIndicesTemp[c*maxLightsPerTile+_lightOffsetCount[c*2+1]], &_lightIndices[_lightOffsetCount[c*2+0]]);
		}
		
		// Offsets
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		glBufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int), 0, GL_DYNAMIC_DRAW);
		glBufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int), _lightOffsetCount, GL_DYNAMIC_DRAW);
		
		// Indices
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		glBufferData(GL_TEXTURE_BUFFER, _lightIndicesSize * sizeof(int), 0, GL_DYNAMIC_DRAW);
		glBufferData(GL_TEXTURE_BUFFER, _lightIndicesSize * sizeof(int), _lightIndices, GL_DYNAMIC_DRAW);
	}
}
