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
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		
		// Light indices
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightManagerLightListIndicesIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		
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
			
			_lightSpotPosition.clear();
			_lightSpotDirection.clear();
			_lightSpotColor.clear();
			_lightSpotDepth.clear();
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = nullptr;
			size_t lightDataSize = lightCount * 2 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
				lightDataSize = 2 * sizeof(Vector4);
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListSpotDataIndex]);
			if(lightDataSize > _lightPointDataSize)
			{
				gl::BufferData(GL_TEXTURE_BUFFER, _lightPointDataSize, 0, GL_DYNAMIC_DRAW);
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
			
			gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, lightData, GL_DYNAMIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
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
		
		const Vector3& cameraWorldPosition = camera->GetWorldPosition();
		const Vector3& cameraClusterSize = camera->GetLightTiles();
		
		size_t maxLightsPerTile = camera->GetMaxLightsPerTile();
		const Rect& rect = camera->GetFrame();
		
		int tilesWidth   = ceil(rect.width / cameraClusterSize.x);
		int tilesHeight  = ceil(rect.height / cameraClusterSize.y);
		int tilesDepth   = ceil(camera->clipfar / cameraClusterSize.z); // TODO: clipnear?
		int clusterCount = tilesWidth * tilesHeight * tilesDepth;
		
		if(_lightIndicesSize < clusterCount * maxLightsPerTile)
		{
			_lightIndicesSize = clusterCount * maxLightsPerTile;
			_lightIndices = new int[_lightIndicesSize];
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
			gl::BufferData(GL_TEXTURE_BUFFER, _lightIndicesSize * sizeof(int), nullptr, GL_DYNAMIC_DRAW);
		}
		
		if(_lightOffsetCountSize < clusterCount  * 2)
		{
			_lightOffsetCountSize = clusterCount * 2;
			_lightOffsetCount = new int[_lightOffsetCountSize];
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
			gl::BufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int), nullptr, GL_DYNAMIC_DRAW);
		}
		
		std::fill(_lightOffsetCount, _lightOffsetCount + _lightOffsetCountSize, 0);
		
		int lightIndex = 0;
		
		for(auto light : _pointLights)
		{
			const Vector3& lightPosition = light->GetWorldPosition();
			float lightRange = light->GetRange();

			Vector4 viewPosition = std::move(camera->viewMatrix.Transform(Vector4(lightPosition, 1.0f)));
			viewPosition.w = 1.0;
			
			Vector4 minProjected = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(-lightRange, -lightRange, 0.0, 0.0)));
			Vector4 maxProjected = std::move(camera->projectionMatrix.Transform(viewPosition+Vector4(lightRange, lightRange, 0.0, 0.0)));
			
			minProjected /= Math::FastAbs(minProjected.w);
			maxProjected /= Math::FastAbs(maxProjected.w);
			
			
			int minX = floor((minProjected.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			int maxX = ceil((maxProjected.x*0.5+0.5)*rect.width/cameraClusterSize.x);
			
			if(maxX > tilesWidth-1)
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
			
			if(minZ < 0)
				minZ = 0;
	
			for(int x = minX; x <= maxX; x++)
			{
				for(int y = minY; y <= maxY; y++)
				{
					for(int z = minZ; z <= maxZ; z++)
					{
						int clusterIndex = x * tilesHeight * tilesDepth + y * tilesDepth + z;
						
						if(_lightOffsetCount[clusterIndex * 2 + 1] < maxLightsPerTile)
						{
							_lightIndices[clusterIndex * maxLightsPerTile + _lightOffsetCount[clusterIndex * 2 + 1]] = lightIndex;
							_lightOffsetCount[clusterIndex * 2 + 1] ++;
						}
					}
				}
			}
			
			lightIndex ++;
		}
		
		_lightOffsetCount[0] = 0;
		
		for(int c = 1; c < clusterCount; c++)
			_lightOffsetCount[c*2+0] = _lightOffsetCount[(c-1)*2+0] + _lightOffsetCount[(c-1)*2+1];
		
		
		int offset = 0;
		
		for(int c = 0; c < clusterCount; c++)
		{
			std::copy(&_lightIndices[c*maxLightsPerTile], &_lightIndices[c*maxLightsPerTile+_lightOffsetCount[c*2+1]], &_lightIndices[offset]);
			offset += _lightOffsetCount[c*2+1];
		}
		
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListOffsetCountIndex]);
		gl::BufferSubData(GL_TEXTURE_BUFFER, 0, _lightOffsetCountSize * sizeof(int), _lightOffsetCount);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightManagerLightListIndicesIndex]);
		gl::BufferSubData(GL_TEXTURE_BUFFER, 0, offset * sizeof(int), _lightIndices);
	}
}
