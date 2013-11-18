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

#define kRNRendererPointLightListIndicesIndex 0
#define kRNRendererPointLightListOffsetIndex  1
#define kRNRendererPointLightListDataIndex    2

#define kRNRendererSpotLightListIndicesIndex 0
#define kRNRendererSpotLightListOffsetIndex  1
#define kRNRendererSpotLightListDataIndex    2

namespace RN
{
	static ThreadPool::SmallObjectsAllocator __LightTilesAlloactor;
	
	LightManager::LightManager()
	{
		_maxLightFastPath = 10;
		
		_lightIndicesBuffer     = nullptr;
		_lightOffsetBuffer      = nullptr;
		_tempLightIndicesBuffer = nullptr;
		
		_lightIndicesBufferSize = 0;
		_lightOffsetBufferSize  = 0;
		
		// Point lights
		_lightPointDataSize = 0;
		
		gl::GenTextures(3, _lightPointTextures);
		gl::GenBuffers(3, _lightPointBuffers);
		
		// light index offsets
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListIndicesIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListIndicesIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightPointBuffers[kRNRendererPointLightListIndicesIndex]);
		
		// Light indices
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListOffsetIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListOffsetIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightPointBuffers[kRNRendererPointLightListOffsetIndex]);
		
		// Light Data
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListDataIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListDataIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightPointBuffers[kRNRendererPointLightListDataIndex]);
		
		// Spot lights
		_lightSpotDataSize = 0;
		
		gl::GenTextures(3, _lightSpotTextures);
		gl::GenBuffers(3, _lightSpotBuffers);
		
		// light index offsets
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListIndicesIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListIndicesIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightSpotBuffers[kRNRendererSpotLightListIndicesIndex]);
		
		// Light indices
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListOffsetIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListOffsetIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightSpotBuffers[kRNRendererSpotLightListOffsetIndex]);
		
		// Light Data
		gl::BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListDataIndex]);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListDataIndex]);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightSpotBuffers[kRNRendererSpotLightListDataIndex]);
	}
	
	LightManager::~LightManager()
	{
		gl::DeleteBuffers(3, _lightPointBuffers);
		gl::DeleteTextures(3, _lightPointTextures);
		
		gl::DeleteBuffers(3, _lightSpotBuffers);
		gl::DeleteTextures(3, _lightSpotTextures);
		
		delete [] _lightIndicesBuffer;
		delete [] _lightOffsetBuffer;
		delete [] _tempLightIndicesBuffer;
	}
	
	void LightManager::AllocateLightBufferStorage(size_t indicesSize, size_t offsetSize)
	{
		if(indicesSize > _lightIndicesBufferSize)
		{
			delete _lightIndicesBuffer;
			delete _tempLightIndicesBuffer;
			
			_lightIndicesBufferSize = indicesSize;
			
			_lightIndicesBuffer     = new int[_lightIndicesBufferSize];
			_tempLightIndicesBuffer = new int[_lightIndicesBufferSize];
		}
		
		if(offsetSize > _lightOffsetBufferSize)
		{
			delete _lightOffsetBuffer;
			
			_lightOffsetBufferSize = offsetSize;
			_lightOffsetBuffer = new int[_lightOffsetBufferSize];
		}
	}
	
	
#define Distance(plane, op, r) { \
float dot = (position.x * plane.normal.x + position.y * plane.normal.y + position.z * plane.normal.z); \
distance = dot - plane.d; \
if(distance op r) \
continue; \
}
	
#define DistanceExpect(plane, op, r, c) { \
float dot = (position.x * plane.normal.x + position.y * plane.normal.y + position.z * plane.normal.z); \
distance = dot - plane.d; \
if(__builtin_expect((distance op r), c)) \
continue; \
}
	
	void LightManager::CullLights(Camera *camera, Light **lights, size_t lightCount, GLuint indicesBuffer, GLuint offsetBuffer)
	{
		Rect rect = camera->GetFrame();
		int tilesWidth  = ceil(rect.width / camera->GetLightTiles().x);
		int tilesHeight = ceil(rect.height / camera->GetLightTiles().y);
		int tilesDepth = 15;//camera->GetLightTiles().z;
		
		size_t i = 0;
		size_t tileCount = tilesWidth * tilesHeight * tilesDepth;
		
		size_t lightindicesSize = tilesWidth * tilesHeight * tilesDepth * lightCount;
		size_t lightindexoffsetSize = tilesWidth * tilesHeight * tilesDepth * 2;
		
		Vector3 corner1 = camera->ToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 corner2 = camera->ToWorld(Vector3(1.0f, -1.0f, 1.0f));
		Vector3 corner3 = camera->ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		
		Vector3 dirx = (corner2-corner1)/rect.width*camera->GetLightTiles().x;
		Vector3 diry = (corner3-corner1)/rect.height*camera->GetLightTiles().y;
		Vector3 camdir = camera->Forward();
		Vector3 dirz = camdir*camera->clipfar/tilesDepth;
		
		const Vector3& camPosition = camera->GetWorldPosition();
		
		std::vector<size_t> indicesCount(tileCount);
		AllocateLightBufferStorage(lightindicesSize, lightindexoffsetSize);
		
		ThreadPool::Batch *batch = ThreadPool::GetSharedInstance()->CreateBatch(__LightTilesAlloactor);
		batch->Reserve(tilesHeight * tilesWidth * tilesDepth);
		
		for(int y = 0; y < tilesHeight; y ++)
		{
			for(int x = 0; x < tilesWidth; x ++)
			{
				size_t index = i;
				i += tilesDepth;
				
				batch->AddTask([&, x, y, index]() {
					
					Plane plleft;
					Plane plright;
					Plane pltop;
					Plane plbottom;
					Plane plfar;
					Plane plnear;
					
					plright.SetPlane(camPosition, corner1+dirx*x+diry*(y+1.0f), corner1+dirx*x+diry*(y-1.0f));
					plleft.SetPlane(camPosition, corner1+dirx*(x+1.0f)+diry*(y+1.0f), corner1+dirx*(x+1.0f)+diry*(y-1.0f));
					plbottom.SetPlane(camPosition, corner1+dirx*(x-1.0f)+diry*(y+1.0f), corner1+dirx*(x+1.0f)+diry*(y+1.0f));
					pltop.SetPlane(camPosition, corner1+dirx*(x-1.0f)+diry*y, corner1+dirx*(x+1.0f)+diry*y);
					
					std::vector<std::pair<Light *, size_t>> culled;
					culled.reserve(lightCount * 0.9f);
					
					float distance;
					
					for(size_t i = 0; i < lightCount; i ++)
					{
						Light *light = lights[i];
						
						const Vector3& position = light->GetWorldPosition();
						const float range = light->GetRange();
						
						DistanceExpect(plright, >, range, true);
						DistanceExpect(plleft, <, -range, true);
						DistanceExpect(plbottom, <, -range, true);
						DistanceExpect(pltop, >, range, true);
						
						culled.emplace_back(std::make_pair(light, i));
					}

					if(!culled.empty())
					{
						size_t i = index;
						
						for(int z = 0; z < tilesDepth; z ++, i++)
						{
							plnear.SetPlane(camPosition + dirz*z, camdir);
							plfar.SetPlane(camPosition + dirz*(z+1.0f), camdir);
							
							size_t lightIndicesCount = 0;
							int *lightPointIndices = _tempLightIndicesBuffer + (i * lightCount);
							
							for(auto& pair : culled)
							{
								Light *light = pair.first;
								
								const Vector3& position = light->GetWorldPosition();
								const float range = light->GetRange();
								
								DistanceExpect(plnear, <, -range, true);
								DistanceExpect(plfar, >, range, true);
								
								lightPointIndices[lightIndicesCount ++] = static_cast<int>(pair.second);
							}
							
							indicesCount[i] = lightIndicesCount;
						}
					}
				});
			}
		}
		
		batch->Commit();
		batch->Wait();
		batch->Release();
		
		size_t lightIndicesCount = 0;
		size_t lightIndexOffsetCount = 0;
		
		for(i=0; i<tileCount; i++)
		{
			size_t tileIndicesCount = indicesCount[i];
			int *tileIndices = _tempLightIndicesBuffer + (i * lightCount);
			
			size_t previous = lightIndicesCount;
			_lightOffsetBuffer[lightIndexOffsetCount ++] = static_cast<int>(previous);
			
			if(tileIndicesCount > 0)
			{
				std::copy(tileIndices, tileIndices + tileIndicesCount, _lightIndicesBuffer + lightIndicesCount);
				lightIndicesCount += tileIndicesCount;
			}
			
			_lightOffsetBuffer[lightIndexOffsetCount ++] = static_cast<int>(lightIndicesCount - previous);
		}
		
		// Indices
		if(lightIndicesCount == 0)
			lightIndicesCount ++;
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, indicesBuffer);
		gl::BufferData(GL_TEXTURE_BUFFER, lightIndicesCount * sizeof(int), 0, GL_DYNAMIC_DRAW);
		gl::BufferData(GL_TEXTURE_BUFFER, lightIndicesCount * sizeof(int), _lightIndicesBuffer, GL_DYNAMIC_DRAW);
		
		// Offsets
		gl::BindBuffer(GL_TEXTURE_BUFFER, offsetBuffer);
		gl::BufferData(GL_TEXTURE_BUFFER, lightIndexOffsetCount * sizeof(int), 0, GL_DYNAMIC_DRAW);
		gl::BufferData(GL_TEXTURE_BUFFER, lightIndexOffsetCount * sizeof(int), _lightOffsetBuffer, GL_DYNAMIC_DRAW);
	}

	int LightManager::CreateLightClusterLists(Camera *camera, Light **lights, size_t pointLightCount, size_t spotLightCount)
	{
		return 0;
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

	int LightManager::CreatePointLightList(Camera *camera)
	{
		Light **lights = _pointLights.data();
		size_t lightCount = _pointLights.size();
		
		_lightPointPosition.clear();
		_lightPointColor.clear();
		_lightPointDepth.clear();
		
		if(lightCount >= _maxLightFastPath)
		{
			lightCount = std::min(camera->GetMaxLightsPerTile(), lightCount);
			
			GLuint indicesBuffer = _lightPointBuffers[kRNRendererPointLightListIndicesIndex];
			GLuint offsetBuffer = _lightPointBuffers[kRNRendererPointLightListOffsetIndex];
			
			CullLights(camera, lights, lightCount, indicesBuffer, offsetBuffer);
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = nullptr;
			size_t lightDataSize = lightCount * 2 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
				lightDataSize = 2 * sizeof(Vector4);
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListDataIndex]);
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
				
				if(i < _maxLightFastPath)
				{
					_lightPointPosition.emplace_back(Vector4(position, light->GetRange()));
					_lightPointColor.emplace_back(Vector4(color, light->Shadow()?static_cast<float>(i):-1.0f));
				}
				
				lightData[i * 2 + 0] = std::move(Vector4(position, light->GetRange()));
				lightData[i * 2 + 1] = std::move(Vector4(color, light->Shadow()?static_cast<float>(i):-1.0f));
				
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
		else
		{
			for(size_t i = 0; i < lightCount; i++)
			{
				Light *light = lights[i];
				_lightPointPosition.emplace_back(Vector4(light->GetWorldPosition(), light->GetRange()));
				
				const Vector3& color = light->GetResultColor();
				_lightPointColor.emplace_back(Vector4(color, light->Shadow()?static_cast<float>(i):-1.0f));
				
				if(light->Shadow())
				{
					if(light->GetShadowCamera())
					{
						_lightPointDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
		}
		
		_pointLights.clear();

		return static_cast<int>(lightCount);
	}
	
	int LightManager::CreateSpotLightList(Camera *camera)
	{
		Light **lights = _spotLights.data();
		size_t lightCount = _spotLights.size();
		
		_lightSpotPosition.clear();
		_lightSpotDirection.clear();
		_lightSpotColor.clear();
		_lightSpotDepth.clear();
		
		if(lightCount >= _maxLightFastPath)
		{
			lightCount = std::min(camera->GetMaxLightsPerTile(), lightCount);
			
			GLuint indicesBuffer = _lightSpotBuffers[kRNRendererSpotLightListIndicesIndex];
			GLuint offsetBuffer = _lightSpotBuffers[kRNRendererSpotLightListOffsetIndex];
			
			CullLights(camera, lights, lightCount, indicesBuffer, offsetBuffer);
			
			// Write the position, range, colour and direction of the lights
			Vector4 *lightData = 0;
			size_t lightDataSize = lightCount * 3 * sizeof(Vector4);
			
			if(lightDataSize == 0)
				lightDataSize = 3 * sizeof(Vector4); // Make sure that we don't end up with an empty buffer
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListDataIndex]);
			if(lightDataSize > _lightSpotDataSize)
			{
				gl::BufferData(GL_TEXTURE_BUFFER, _lightSpotDataSize, 0, GL_DYNAMIC_DRAW);
				gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				
				_lightSpotDataSize = lightDataSize;
			}
			
			if(_lightSpotData.size() < lightCount * 3)
				_lightSpotData.resize(lightCount * 3);
			
			lightData = _lightSpotData.data();
			
			for(size_t i = 0; i < lightCount; i++)
			{
				Light *light = lights[i];
				const Vector3& position  = light->GetWorldPosition();
				const Vector3& color     = light->GetResultColor();
				const Vector3& direction = -light->Forward();
				
				if(i < _maxLightFastPath)
				{
					_lightSpotPosition.emplace_back(Vector4(position, light->GetRange()));
					_lightSpotDirection.emplace_back(Vector4(direction, light->GetAngle()));
					_lightSpotColor.emplace_back(Vector4(color, light->Shadow()?static_cast<float>(i):-1.0f));
				}
				
				lightData[i * 3 + 0] = Vector4(position, light->GetRange());
				lightData[i * 3 + 1] = Vector4(color, 0.0f);
				lightData[i * 3 + 2] = Vector4(direction, light->GetAngle());
				
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
		else
		{
			for(size_t i = 0; i < lightCount; i++)
			{
				Light *light = lights[i];
				_lightSpotPosition.emplace_back(Vector4(light->GetWorldPosition(), light->GetRange()));
				
				const Vector3& color = light->GetResultColor();
				_lightSpotColor.emplace_back(Vector4(color, light->Shadow()?static_cast<float>(i):-1.0f));
				
				const Vector3& direction = -light->Forward();
				_lightSpotDirection.emplace_back(Vector4(direction, light->GetAngle()));
				
				if(light->Shadow())
				{
					if(light->GetShadowCamera())
					{
						_lightSpotDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
		}
		
		_spotLights.clear();
		
		return static_cast<int>(lightCount);
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
	
#undef Distance

}
