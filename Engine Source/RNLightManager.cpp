//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLightManager.h"
#include "RNLight.h"
#include "RNRenderer.h"
#include "RNCamera.h"
#include "RNShader.h"
#include "RNTexture.h"
#include "RNOpenGLQueue.h"

#define kRNLightListOffsetCount 0
#define kRNLightListIndices 1
#define kRNLightListPointData 2
#define kRNLightListSpotData 3

namespace RN
{
	RNDeclareMeta(LightManager)
	RNDeclareMeta(ClusteredLightManager)
	
	LightManager::LightManager() :
		camera(nullptr)
	{}
	
	LightManager *LightManager::CreateDefaultLightManager()
	{
		return new ClusteredLightManager();
	}
	
	
	ClusteredLightManager::ClusteredLightManager() :
		_maxLightsDirect(4),
		_maxLightsPerCluster(100),
		_clusterSize(Vector3(32.0f, 32.0f, 5.0f)),
		_lightIndices(nullptr),
		_lightOffsetCount(nullptr),
		_lightIndicesSize(0),
		_lightOffsetCountSize(0),
		_lightPointDataSize(0),
		_lightSpotDataSize(0),
		_spotLightCount(0),
		_pointLightCount(0),
		_directionalLightCount(0)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenTextures(4, _lightTextures);
			gl::GenBuffers(4, _lightBuffers);
			
			// light offsets and counts
			gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListOffsetCount]);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListOffsetCount]);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, _lightBuffers[kRNLightListOffsetCount]);
			
			// Light indices
			gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListIndices]);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListIndices]);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, _lightBuffers[kRNLightListIndices]);
			
			// Point Light Data
			gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListPointData]);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListPointData]);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[kRNLightListPointData]);
			
			// Spot Light Data
			gl::BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListSpotData]);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListSpotData]);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[kRNLightListSpotData]);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		});
	}
	
	ClusteredLightManager::~ClusteredLightManager()
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteBuffers(4, _lightBuffers);
			gl::DeleteTextures(4, _lightTextures);
		}, true);
		
		delete _lightIndices;
		delete _lightOffsetCount;
	}
	
	
	void ClusteredLightManager::UpdateProgram(Renderer *renderer, ShaderProgram *program)
	{
		if(program->lightDirectionalCount != -1)
			gl::Uniform1i(program->lightDirectionalCount, static_cast<GLint>(_directionalLightCount));
		
		if(program->lightDirectionalDirection != -1)
			gl::Uniform3fv(program->lightDirectionalDirection, static_cast<GLint>(_directionalLightCount), (float *)_lightDirectionalDirection.data());
		
		if(program->lightDirectionalColor != -1)
			gl::Uniform4fv(program->lightDirectionalColor, static_cast<GLint>(_directionalLightCount), (float *)_lightDirectionalColor.data());
		
		
		if(program->lightDirectionalMatrix != -1)
		{
			const float *data = reinterpret_cast<const float *>(_lightDirectionalMatrix.data());
			gl::UniformMatrix4fv(program->lightDirectionalMatrix, (GLuint)_lightDirectionalMatrix.size(), GL_FALSE, data);
		}
		
		if(program->lightDirectionalDepth != -1 && _lightDirectionalDepth.size() > 0)
		{
			uint32 textureUnit = renderer->BindTexture(_lightDirectionalDepth.front());
			gl::Uniform1i(program->lightDirectionalDepth, textureUnit);
		}
		
		
		
		if(_pointSpotLightCount > 0 && program->lightClusterSize != -1)
		{
			Rect rect = camera->GetFrame();
			float scaleFactor = renderer->GetScaleFactor();
			
			
			Vector2 lightClusterSize  = Vector2(_clusterSize.x * scaleFactor, _clusterSize.y * scaleFactor);
			Vector2 lightClusterCount = Vector2(ceil(rect.height / _clusterSize.y), ceil(camera->GetClipFar() / _clusterSize.z));
			
			gl::Uniform4f(program->lightClusterSize, lightClusterSize.x, lightClusterSize.y, lightClusterCount.x, lightClusterCount.y);
		}
		
		if(_lightPointDepth.size() > 0)
		{
			const std::vector<GLuint>& lightPointDepthLocations = program->lightPointDepthLocations;
			
			if(lightPointDepthLocations.size() > 0)
			{
				size_t textureCount = std::min(lightPointDepthLocations.size(), _lightPointDepth.size());
				uint32 lastpointdepth = 0;
				
				for(size_t i = 0; i < textureCount; i ++)
				{
					lastpointdepth = renderer->BindTexture(_lightPointDepth[i]);
					gl::Uniform1i(lightPointDepthLocations[i], lastpointdepth);
				}
				
				for(size_t i = textureCount; i < lightPointDepthLocations.size(); i++)
				{
					GLint location = lightPointDepthLocations[i];
					gl::Uniform1i(location, lastpointdepth);
				}
			}
		}
		
		if(_lightSpotDepth.size() > 0)
		{
			const std::vector<GLuint>& lightSpotDepthLocations = program->lightSpotDepthLocations;
			
			if(lightSpotDepthLocations.size() > 0)
			{
				size_t textureCount = std::min(lightSpotDepthLocations.size(), _lightSpotDepth.size());
				uint32 lastspotdepth = 0;
				
				for(size_t i =0; i < textureCount; i ++)
				{
					lastspotdepth = renderer->BindTexture(_lightSpotDepth[i]);
					gl::Uniform1i(lightSpotDepthLocations[i], lastspotdepth);
				}
				
				for(size_t i = textureCount; i < lightSpotDepthLocations.size(); i++)
				{
					GLint location = lightSpotDepthLocations[i];
					gl::Uniform1i(location, lastspotdepth);
				}
			}
			
			if(program->lightSpotMatrix != -1)
			{
				const float *data = reinterpret_cast<const float *>(_lightSpotMatrix.data());
				gl::UniformMatrix4fv(program->lightSpotMatrix, (GLuint)_lightSpotMatrix.size(), GL_FALSE, data);
			}
		}
		
		if(_pointSpotLightCount > 0)
		{
			if(program->lightListIndices != -1)
			{
				uint32 textureUnit = renderer->BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListIndices]);
				gl::Uniform1i(program->lightListIndices, textureUnit);
			}
			
			if(program->lightListOffsetCount != -1)
			{
				uint32 textureUnit = renderer->BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListOffsetCount]);
				gl::Uniform1i(program->lightListOffsetCount, textureUnit);
			}
			
			if(program->lightListDataPoint != -1)
			{
				uint32 textureUnit = renderer->BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListPointData]);
				gl::Uniform1i(program->lightListDataPoint, textureUnit);
			}
			
			if(program->lightListDataSpot != -1)
			{
				uint32 textureUnit = renderer->BindTexture(GL_TEXTURE_BUFFER, _lightTextures[kRNLightListSpotData]);
				gl::Uniform1i(program->lightListDataSpot, textureUnit);
			}
		}
	}
	
	void ClusteredLightManager::AdjustProgramTypes(Shader *shader, uint32 &types)
	{
		if(_lightDirectionalDepth.size() > 0 && shader->SupportsProgramOfType(ShaderProgram::TypeDirectionalShadows))
			types |= ShaderProgram::TypeDirectionalShadows;
		
		if(_lightPointDepth.size() > 0 && shader->SupportsProgramOfType(ShaderProgram::TypePointShadows))
			types |= ShaderProgram::TypePointShadows;
		
		if(_lightSpotDepth.size() > 0 && shader->SupportsProgramOfType(ShaderProgram::TypeSpotShadows))
			types |= ShaderProgram::TypeSpotShadows;
	}
	
	
	void ClusteredLightManager::AddLight(Light *light)
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
				if(light->HasShadow())
					_directionalLights.push_front(light);
				else
					_directionalLights.push_back(light);
				break;
				
			default:
				break;
		}
	}

	
	void ClusteredLightManager::CreateLightLists()
	{
		if(!_pointLights.empty() || !_spotLights.empty())
			CullLights();
		
		CreatePointSpotLightLists();
		CreateDirectionalLightList();
	}
	
	void ClusteredLightManager::CreatePointSpotLightLists()
	{
		_pointSpotLightCount = 0;
		_spotLightCount  = 0;
		_pointLightCount = 0;
		
		// Point lights
		if(!_pointLights.empty())
		{
			Light **lights = _pointLights.data();
			size_t lightCount = _pointLights.size();
			
			_pointSpotLightCount += lightCount;
			_pointLightCount = lightCount;
			
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
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListPointData]);
			if(lightDataSize > _lightPointDataSize)
			{
				gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, nullptr, GL_STATIC_DRAW);
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
				const float shadow = light->HasShadow()?static_cast<float>(i):-1.0f;
				
				if(i < _maxLightsDirect)
				{
					_lightPointPosition.emplace_back(Vector4(position, range));
					_lightPointColor.emplace_back(Vector4(color, shadow));
				}
				
				lightData[i * 2 + 0] = std::move(Vector4(position, range));
				lightData[i * 2 + 1] = std::move(Vector4(color, shadow));
				
				if(light->HasShadow())
				{
					if(light->GetShadowCamera())
					{
						_lightPointDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
			
			gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, nullptr, GL_STATIC_DRAW);
			gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, lightData, GL_STATIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		else
		{
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListPointData]);
			gl::BufferData(GL_TEXTURE_BUFFER, 1, nullptr, GL_STATIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		
		// Spot lights
		if(!_spotLights.empty())
		{
			Light **lights = _spotLights.data();
			size_t lightCount = _spotLights.size();
			
			_pointSpotLightCount += lightCount;
			_spotLightCount = lightCount;
			
			_lightSpotPosition.clear();
			_lightSpotDirection.clear();
			_lightSpotColor.clear();
			_lightSpotMatrix.clear();
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
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListSpotData]);
			if(lightDataSize > _lightSpotDataSize)
			{
				gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, nullptr, GL_STATIC_DRAW);
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
				const Vector3& direction = -light->GetForward();
				const float angle = light->GetAngleCos();
				const float range = light->GetRange();
				const float shadow = light->HasShadow()?static_cast<float>(i):-1.0f;
				
				if(i < _maxLightsDirect)
				{
					_lightSpotPosition.emplace_back(Vector4(position, range));
					_lightSpotDirection.emplace_back(Vector4(direction, angle));
					_lightSpotColor.emplace_back(Vector4(color, shadow));
				}
				
				lightData[i * 3 + 0] = Vector4(position, range);
				lightData[i * 3 + 1] = Vector4(color, shadow);
				lightData[i * 3 + 2] = Vector4(direction, angle);
				
				if(light->HasShadow())
				{
					if(light->GetShadowCamera())
					{
						_lightSpotMatrix.push_back(light->GetShadowMatrices()[0]);
						_lightSpotDepth.push_back(light->GetShadowCamera()->GetStorage()->GetDepthTarget());
					}
				}
			}
			
			gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, nullptr, GL_STATIC_DRAW);
			gl::BufferData(GL_TEXTURE_BUFFER, lightDataSize, lightData, GL_STATIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		else
		{
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListSpotData]);
			gl::BufferData(GL_TEXTURE_BUFFER, 1, nullptr, GL_STATIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		
		_pointLights.clear();
		_spotLights.clear();
	}
	
	
	void ClusteredLightManager::CreateDirectionalLightList()
	{
		_directionalLightCount = _directionalLights.size();
		
		_lightDirectionalDirection.clear();
		_lightDirectionalColor.clear();
		_lightDirectionalMatrix.clear();
		_lightDirectionalDepth.clear();
		
		for(size_t i = 0; i < _directionalLightCount; i ++)
		{
			Light *light = _directionalLights[i];
			const Vector3& color = light->GetResultColor();
			const Vector3& direction = -light->GetForward();
			
			_lightDirectionalDirection.push_back(direction);
			_lightDirectionalColor.emplace_back(Vector4(color, light->HasShadow() ? static_cast<float>(i):-1.0f));
			
			if(light->HasShadow())
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
	}
	
	
	void ClusteredLightManager::CullLights()
	{
		Vector3 cameraForward = camera->GetForward();
		
		Vector3 cameraWorldPosition = camera->GetWorldPosition();
		const Rect& rect = camera->GetFrame();
		
		int tilesWidth   = ceil(rect.width / _clusterSize.x);
		int tilesHeight  = ceil(rect.height / _clusterSize.y);
		int tilesDepth   = ceil(camera->GetClipFar() / _clusterSize.z);
		int clusterCount = tilesWidth * tilesHeight * tilesDepth;
		
		if(_lightIndicesSize < clusterCount * _maxLightsPerCluster)
		{
			_lightIndicesSize = clusterCount * _maxLightsPerCluster;
			_lightIndices = new uint16[_lightIndicesSize];
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListIndices]);
			gl::BufferData(GL_TEXTURE_BUFFER, _lightIndicesSize * sizeof(uint16), nullptr, GL_STATIC_DRAW);
		}
		
		if(_lightOffsetCountSize < clusterCount * 3)
		{
			_lightOffsetCountSize = clusterCount * 3;
			_lightOffsetCount = new int32[_lightOffsetCountSize];
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListOffsetCount]);
			gl::BufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int32), nullptr, GL_STATIC_DRAW);
		}
		
		std::fill(_lightOffsetCount, _lightOffsetCount + _lightOffsetCountSize, 0);
		
		uint16 lightIndex = 0;
		
		//Cull point lights
		for(auto light : _pointLights)
		{
			const Vector3& lightPosition = light->GetWorldPosition();
			float lightRange = light->GetRange();

			Vector4 viewPosition = std::move(camera->GetViewMatrix().Transform(Vector4(lightPosition, 1.0f)));
			viewPosition.w = 1.0f;
			
			float zOffsetMinX = 0.0f;
			float zOffsetMaxX = 0.0f;
			float zOffsetMinY = 0.0f;
			float zOffsetMaxY = 0.0f;
			if(viewPosition.z+camera->GetClipNear() < -lightRange)
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = lightRange;
			}
			else
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = -viewPosition.z-camera->GetClipNear();
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
			
			Vector4 minProjectedX = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(-lightRange, 0.0f, zOffsetMinX, 0.0f)));
			Vector4 maxProjectedX = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(lightRange, 0.0f, zOffsetMaxX, 0.0f)));
			minProjectedX /= Math::FastAbs(minProjectedX.w);
			maxProjectedX /= Math::FastAbs(maxProjectedX.w);
			
			Vector4 minProjectedY = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(0.0f, -lightRange, zOffsetMinY, 0.0f)));
			Vector4 maxProjectedY = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(0.0f, lightRange, zOffsetMaxY, 0.0f)));
			minProjectedY /= Math::FastAbs(minProjectedY.w);
			maxProjectedY /= Math::FastAbs(maxProjectedY.w);
			
			int minX = floor((minProjectedX.x*0.5+0.5)*rect.width / _clusterSize.x);
			int maxX = ceil((maxProjectedX.x*0.5+0.5)*rect.width / _clusterSize.x);
			if(maxX > tilesWidth-1)
				maxX = tilesWidth-1;
			if(minX > tilesWidth-1)
				minX = maxX+1;
			if(minX < 0)
				minX = 0;
			if(maxX < 0)
				maxX = minX-1;
			
			int minY = floor((minProjectedY.y*0.5+0.5)*rect.height / _clusterSize.y);
			int maxY = ceil((maxProjectedY.y*0.5+0.5)*rect.height / _clusterSize.y);
			if(maxY > tilesHeight-1)
				maxY = tilesHeight-1;
			if(minY > tilesHeight-1)
				minY = maxY+1;
			if(minY < 0)
				minY = 0;
			if(maxY < 0)
				maxY = minY-1;
			
			float linearDist = cameraForward.Dot(lightPosition - cameraWorldPosition);
			int minZ = floor((linearDist - lightRange) / _clusterSize.z);
			int maxZ = ceil((linearDist + lightRange) / _clusterSize.z);
			if(minZ < 0)
				minZ = 0;
	
			for(int x = minX; x <= maxX; x++)
			{
				for(int y = minY; y <= maxY; y++)
				{
					for(int z = minZ; z <= maxZ; z++)
					{
						int clusterIndex = x * tilesHeight * tilesDepth + y * tilesDepth + z;
						
						if(_lightOffsetCount[clusterIndex * 3 + 1] < _maxLightsPerCluster)
						{
							_lightIndices[clusterIndex * _maxLightsPerCluster + _lightOffsetCount[clusterIndex * 3 + 1]] = lightIndex;
							_lightOffsetCount[clusterIndex * 3 + 1] ++;
						}
					}
				}
			}
			
			lightIndex ++;
		}
		
		// Cull spot lights
		lightIndex = 0;
		for(auto light : _spotLights)
		{
			const Vector3& lightPosition = light->GetWorldPosition();
			float lightRange = light->GetRange();
			
			Vector4 viewPosition = std::move(camera->GetViewMatrix().Transform(Vector4(lightPosition, 1.0f)));
			viewPosition.w = 1.0f;
			
			float zOffsetMinX = 0.0f;
			float zOffsetMaxX = 0.0f;
			float zOffsetMinY = 0.0f;
			float zOffsetMaxY = 0.0f;
			if(viewPosition.z+camera->GetClipNear() < -lightRange)
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = lightRange;
			}
			else
			{
				zOffsetMinX = zOffsetMaxX = zOffsetMinY = zOffsetMaxY = -viewPosition.z-camera->GetClipNear();
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
			
			Vector4 minProjectedX = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(-lightRange, 0.0f, zOffsetMinX, 0.0f)));
			Vector4 maxProjectedX = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(lightRange, 0.0f, zOffsetMaxX, 0.0f)));
			minProjectedX /= Math::FastAbs(minProjectedX.w);
			maxProjectedX /= Math::FastAbs(maxProjectedX.w);
			
			Vector4 minProjectedY = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(0.0f, -lightRange, zOffsetMinY, 0.0f)));
			Vector4 maxProjectedY = std::move(camera->GetProjectionMatrix().Transform(viewPosition+Vector4(0.0f, lightRange, zOffsetMaxY, 0.0f)));
			minProjectedY /= Math::FastAbs(minProjectedY.w);
			maxProjectedY /= Math::FastAbs(maxProjectedY.w);
			
			int minX = floor((minProjectedX.x*0.5+0.5)*rect.width / _clusterSize.x);
			int maxX = ceil((maxProjectedX.x*0.5+0.5)*rect.width / _clusterSize.x);
			if(maxX > tilesWidth-1)
				maxX = tilesWidth-1;
			if(minX > tilesWidth-1)
				minX = maxX+1;
			if(minX < 0)
				minX = 0;
			if(maxX < 0)
				maxX = minX-1;
			
			int minY = floor((minProjectedY.y*0.5+0.5)*rect.height / _clusterSize.y);
			int maxY = ceil((maxProjectedY.y*0.5+0.5)*rect.height / _clusterSize.y);
			if(maxY > tilesHeight-1)
				maxY = tilesHeight-1;
			if(minY > tilesHeight-1)
				minY = maxY+1;
			if(minY < 0)
				minY = 0;
			if(maxY < 0)
				maxY = minY-1;
			
			float linearDist = cameraForward.Dot(lightPosition - cameraWorldPosition);
			int minZ = floor((linearDist - lightRange) / _clusterSize.z);
			int maxZ = ceil((linearDist + lightRange) / _clusterSize.z);
			if(minZ < 0)
				minZ = 0;
			
			for(int x = minX; x <= maxX; x++)
			{
				for(int y = minY; y <= maxY; y++)
				{
					for(int z = minZ; z <= maxZ; z++)
					{
						int clusterIndex = x * tilesHeight * tilesDepth + y * tilesDepth + z;
					
						if(_lightOffsetCount[clusterIndex * 3 + 1]+_lightOffsetCount[clusterIndex * 3 + 2] < _maxLightsPerCluster)
						{
							_lightIndices[clusterIndex * _maxLightsPerCluster + _lightOffsetCount[clusterIndex * 3 + 1] + _lightOffsetCount[clusterIndex * 3 + 2]] = lightIndex;
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
			std::copy(&_lightIndices[c*_maxLightsPerCluster], &_lightIndices[c*_maxLightsPerCluster+_lightOffsetCount[c*3+1]+_lightOffsetCount[c*3+2]], &_lightIndices[offset]);
			offset += _lightOffsetCount[c*3+1] + _lightOffsetCount[c*3+2];
		}
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListOffsetCount]);
		gl::BufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int32), nullptr, GL_STATIC_DRAW);
		gl::BufferData(GL_TEXTURE_BUFFER, _lightOffsetCountSize * sizeof(int32), _lightOffsetCount, GL_STATIC_DRAW);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[kRNLightListIndices]);
		gl::BufferData(GL_TEXTURE_BUFFER, offset * sizeof(uint16), nullptr, GL_STATIC_DRAW);
		gl::BufferData(GL_TEXTURE_BUFFER, offset * sizeof(uint16), _lightIndices, GL_STATIC_DRAW);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
	}
}
