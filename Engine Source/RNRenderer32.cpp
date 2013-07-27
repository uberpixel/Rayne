//
//  RNRenderer32.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer32.h"
#include "RNThreadPool.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"

#define kRNRendererPointLightListIndicesIndex 0
#define kRNRendererPointLightListOffsetIndex  1
#define kRNRendererPointLightListDataIndex    2

#define kRNRendererSpotLightListIndicesIndex 0
#define kRNRendererSpotLightListOffsetIndex  1
#define kRNRendererSpotLightListDataIndex    2

namespace RN
{
	Renderer32::Renderer32()
	{
		// Setup framebuffer copy stuff
		_copyVertices[0] = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);
		_copyVertices[1] = Vector4(1.0f, -1.0f,  1.0f, 0.0f);
		
		_copyVertices[2] = Vector4(-1.0f, 1.0f,  0.0f, 1.0f);
		_copyVertices[3] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		
		_lightIndicesBuffer     = 0;
		_lightOffsetBuffer      = 0;
		_tempLightIndicesBuffer = 0;
		
		_lightIndicesBufferSize = 0;
		_lightOffsetBufferSize  = 0;
		
		glGenVertexArrays(1, &_copyVAO);
		glBindVertexArray(_copyVAO);
		
		glGenBuffers(1, &_copyVBO);
		
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STATIC_DRAW);
		
		glBindVertexArray(0);
		
		// Point lights
		_lightPointDataSize = 0;
		
		glGenTextures(3, _lightPointTextures);
		glGenBuffers(3, _lightPointBuffers);
		
		// light index offsets
		glBindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListIndicesIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListIndicesIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightPointBuffers[kRNRendererPointLightListIndicesIndex]);
		
		// Light indices
		glBindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListOffsetIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListOffsetIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightPointBuffers[kRNRendererPointLightListOffsetIndex]);
		
		// Light Data
		glBindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListDataIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListDataIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightPointBuffers[kRNRendererPointLightListDataIndex]);
		
		// Spot lights
		_lightSpotDataSize = 0;
		
		glGenTextures(3, _lightSpotTextures);
		glGenBuffers(3, _lightSpotBuffers);
		
		// light index offsets
		glBindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListIndicesIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListIndicesIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightSpotBuffers[kRNRendererSpotLightListIndicesIndex]);
		
		// Light indices
		glBindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListOffsetIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListOffsetIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightSpotBuffers[kRNRendererSpotLightListOffsetIndex]);
		
		// Light Data
		glBindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListDataIndex]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListDataIndex]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightSpotBuffers[kRNRendererSpotLightListDataIndex]);
		
		_maxLightFastPath = 10;
		
		RN_CHECKOPENGL();
	}
	
	Renderer32::~Renderer32()
	{
	}
	
	
	
	void Renderer32::AllocateLightBufferStorage(size_t indicesSize, size_t offsetSize)
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
	
	// ---------------------
	// MARK: -
	// MARK: Light
	// ---------------------
	
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
	
	void Renderer32::CullLights(Camera *camera, Light **lights, size_t lightCount, GLuint indicesBuffer, GLuint offsetBuffer)
	{
		Rect rect = camera->Frame();
		int tilesWidth  = ceil(rect.width / camera->LightTiles().x);
		int tilesHeight = ceil(rect.height / camera->LightTiles().y);
		
		size_t i = 0;
		size_t tileCount = tilesWidth * tilesHeight;
		
		size_t lightindicesSize = tilesWidth * tilesHeight * lightCount;
		size_t lightindexoffsetSize = tilesWidth * tilesHeight * 2;
		
		if(lightCount == 0)
		{
			AllocateLightBufferStorage(1, lightindexoffsetSize);
			
			std::fill(_lightIndicesBuffer, _lightIndicesBuffer + 1, 0);
			std::fill(_lightOffsetBuffer, _lightOffsetBuffer + lightindexoffsetSize, 0);
			
			// Indicies
			glBindBuffer(GL_TEXTURE_BUFFER, indicesBuffer);
			glBufferData(GL_TEXTURE_BUFFER, 1 * sizeof(int), 0, GL_DYNAMIC_DRAW);
			glBufferData(GL_TEXTURE_BUFFER, 1 * sizeof(int), _lightIndicesBuffer, GL_DYNAMIC_DRAW);
			
			// Offsets
			glBindBuffer(GL_TEXTURE_BUFFER, offsetBuffer);
			glBufferData(GL_TEXTURE_BUFFER, lightindexoffsetSize * sizeof(int), 0, GL_DYNAMIC_DRAW);
			glBufferData(GL_TEXTURE_BUFFER, lightindexoffsetSize * sizeof(int), _lightOffsetBuffer, GL_DYNAMIC_DRAW);
			
			return;
		}
		
		Vector3 corner1 = camera->ToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 corner2 = camera->ToWorld(Vector3(1.0f, -1.0f, 1.0f));
		Vector3 corner3 = camera->ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		
		Vector3 dirx = (corner2-corner1)/rect.width*camera->LightTiles().x;
		Vector3 diry = (corner3-corner1)/rect.height*camera->LightTiles().y;
		
		const Vector3& camPosition = camera->WorldPosition();
		float *depthArray = camera->DepthArray();
		
		Vector3 camdir = camera->Forward();
		
		std::vector<size_t> indicesCount(tileCount);
		AllocateLightBufferStorage(lightindicesSize, lightindexoffsetSize);
		
		ThreadPool::Batch *batch = ThreadPool::SharedInstance()->OpenBatch();
		batch->Reserve(tilesHeight * tilesWidth);
		
		for(int y=0; y<tilesHeight; y++)
		{
			for(int x=0; x<tilesWidth; x++)
			{
				size_t index = i ++;
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
					
					plnear.SetPlane(camPosition + camdir * depthArray[index * 2 + 0], camdir);
					plfar.SetPlane(camPosition + camdir * depthArray[index * 2 + 1], camdir);
					
					size_t lightIndicesCount = 0;
					int *lightPointIndices = _tempLightIndicesBuffer + (index * lightCount);
					
					for(size_t i=0; i<lightCount; i++)
					{
						Light *light = lights[i];
						
						const Vector3& position = light->WorldPosition();
						const float range = light->Range();
						float distance, dr, dl, dt, db;
						DistanceExpect(plright, >, range, true);
						dr = distance;
						DistanceExpect(plleft, <, -range, true);
						dl = distance;
						DistanceExpect(plbottom, <, -range, true);
						db = distance;
						DistanceExpect(pltop, >, range, true);
						dt = distance;
						
						float sqrange = range*range;
						
						if(dr > 0.0f && db < 0.0f && dr*dr+db*db > sqrange)
							continue;
						if(dr > 0.0f && dt > 0.0f && dr*dr+dt*dt > sqrange)
							continue;
						if(dl < 0.0f && db < 0.0f && dl*dl+db*db > sqrange)
							continue;
						if(dl < 0.0f && dt > 0.0f && dl*dl+dt*dt > sqrange)
							continue;
						
						Distance(plnear, >, range);
						Distance(plfar, <, -range);
						
						lightPointIndices[lightIndicesCount ++] = static_cast<int>(i);
					}
					
					indicesCount[index] = lightIndicesCount;
				});
			}
		}
		
		batch->Commit();
		batch->Wait();
		
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
		
		glBindBuffer(GL_TEXTURE_BUFFER, indicesBuffer);
		glBufferData(GL_TEXTURE_BUFFER, lightIndicesCount * sizeof(int), 0, GL_DYNAMIC_DRAW);
		glBufferData(GL_TEXTURE_BUFFER, lightIndicesCount * sizeof(int), _lightIndicesBuffer, GL_DYNAMIC_DRAW);
		
		// Offsets
		glBindBuffer(GL_TEXTURE_BUFFER, offsetBuffer);
		glBufferData(GL_TEXTURE_BUFFER, lightIndexOffsetCount * sizeof(int), 0, GL_DYNAMIC_DRAW);
		glBufferData(GL_TEXTURE_BUFFER, lightIndexOffsetCount * sizeof(int), _lightOffsetBuffer, GL_DYNAMIC_DRAW);
	}
	
	int Renderer32::CreatePointLightList(Camera *camera)
	{
		Light **lights = _pointLights.data();
		size_t lightCount = _pointLights.size();
		
		lightCount = MIN(camera->MaxLightsPerTile(), lightCount);
		
		_lightPointPosition.clear();
		_lightPointColor.clear();
		
		if(camera->DepthTiles())
		{
			GLuint indicesBuffer = _lightPointBuffers[kRNRendererPointLightListIndicesIndex];
			GLuint offsetBuffer = _lightPointBuffers[kRNRendererPointLightListOffsetIndex];
			
			CullLights(camera, lights, lightCount, indicesBuffer, offsetBuffer);
			
			// Write the position, range and colour of the lights
			Vector4 *lightData = 0;
			size_t lightDataSize = lightCount * 2 * sizeof(Vector4);
			
			if(lightDataSize == 0) // Makes sure that we don't end up with an empty buffer
				lightDataSize = 2 * sizeof(Vector4);
			
			glBindBuffer(GL_TEXTURE_BUFFER, _lightPointBuffers[kRNRendererPointLightListDataIndex]);
			if(lightDataSize > _lightPointDataSize)
			{
				glBufferData(GL_TEXTURE_BUFFER, _lightPointDataSize, 0, GL_DYNAMIC_DRAW);
				glBufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				
				_lightPointDataSize = lightDataSize;
			}
			
			lightData = (Vector4 *)glMapBufferRange(GL_TEXTURE_BUFFER, 0, _lightPointDataSize, GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT);
			
			for(size_t i=0; i<lightCount; i++)
			{
				Light *light = lights[i];
				const Vector3& position = light->WorldPosition();
				const Vector3& color = light->ResultColor();
				
				if(i < _maxLightFastPath)
				{
					_lightPointPosition.emplace_back(Vector4(position, light->Range()));
					_lightPointColor.emplace_back(Vector4(color, 0.0f));
				}
				
				lightData[i * 2 + 0] = Vector4(position, light->Range());
				lightData[i * 2 + 1] = Vector4(color, 0.0f);
			}
			
			glUnmapBuffer(GL_TEXTURE_BUFFER);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		else
		{
			lightCount = _pointLights.size();
			for(size_t i = 0; i < lightCount; i++)
			{
				_lightPointPosition.emplace_back(Vector4(_pointLights[i]->Position(), _pointLights[i]->Range()));
				_lightPointColor.emplace_back(Vector4(_pointLights[i]->Color().r, _pointLights[i]->Color().g, _pointLights[i]->Color().b, 0.0f));
			}
		}
		
		return static_cast<int>(lightCount);
	}
	
	int Renderer32::CreateSpotLightList(Camera *camera)
	{
		Light **lights = _spotLights.data();
		size_t lightCount = _spotLights.size();
		
		lightCount = MIN(camera->MaxLightsPerTile(), lightCount);
		
		_lightSpotPosition.clear();
		_lightSpotDirection.clear();
		_lightSpotColor.clear();
		
		if(camera->DepthTiles())
		{
			GLuint indicesBuffer = _lightSpotBuffers[kRNRendererSpotLightListIndicesIndex];
			GLuint offsetBuffer = _lightSpotBuffers[kRNRendererSpotLightListOffsetIndex];
			
			CullLights(camera, lights, lightCount, indicesBuffer, offsetBuffer);
			
			// Write the position, range, colour and direction of the lights
			Vector4 *lightData = 0;
			size_t lightDataSize = lightCount * 3 * sizeof(Vector4);
			
			if(lightDataSize == 0)
				lightDataSize = 3 * sizeof(Vector4); // Make sure that we don't end up with an empty buffer
			
			glBindBuffer(GL_TEXTURE_BUFFER, _lightSpotBuffers[kRNRendererSpotLightListDataIndex]);
			if(lightDataSize > _lightSpotDataSize)
			{
				glBufferData(GL_TEXTURE_BUFFER, _lightSpotDataSize, 0, GL_DYNAMIC_DRAW);
				glBufferData(GL_TEXTURE_BUFFER, lightDataSize, 0, GL_DYNAMIC_DRAW);
				
				_lightSpotDataSize = lightDataSize;
			}
			
			lightData = (Vector4 *)glMapBufferRange(GL_TEXTURE_BUFFER, 0, _lightSpotDataSize, GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT);
			
			for(size_t i=0; i<lightCount; i++)
			{
				Light *light = lights[i];
				const Vector3& position  = light->WorldPosition();
				const Vector3& color     = light->ResultColor();
				const Vector3& direction = light->Forward();
				
				if(i < _maxLightFastPath)
				{
					_lightSpotPosition.emplace_back(Vector4(position, light->Range()));
					_lightSpotDirection.emplace_back(Vector4(direction, light->Angle()));
					_lightSpotColor.emplace_back(Vector4(color, 0.0f));
				}
				
				lightData[i * 3 + 0] = Vector4(position, light->Range());
				lightData[i * 3 + 1] = Vector4(color, 0.0f);
				lightData[i * 3 + 2] = Vector4(direction, light->Angle());
			}
			
			glUnmapBuffer(GL_TEXTURE_BUFFER);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		else
		{
			return 0;
		}
		
		return static_cast<int>(lightCount);
	}
	
	int Renderer32::CreateDirectionalLightList(Camera *camera)
	{
		Light **lights = _directionalLights.data();
		size_t lightCount = _directionalLights.size();
		
		_lightDirectionalDirection.clear();
		_lightDirectionalColor.clear();
		
		for(size_t i=0; i<lightCount; i++)
		{
			Light *light = lights[i];
			const Vector3& color = light->ResultColor();
			const Vector3& direction = light->Forward();
			
			_lightDirectionalDirection.push_back(direction);
			_lightDirectionalColor.emplace_back(Vector4(color, light->Shadow() ? 1.0f : 0.0f));
			
			if(light->Shadow())
			{
				if(camera == light->ShadowCamera() || light->ShadowCameras()->ContainsObject(camera))
				{
					_lightDirectionalMatrix.clear();
					_lightDirectionalDepth.clear();
					
					for(int i = 0; i < 4; i++)
					{
						_lightDirectionalMatrix.push_back(light->ShadowMatrices()[i]);
					}
					
					if(light->ShadowCamera())
					{
						_lightDirectionalDepth.push_back(light->ShadowCamera()->Storage()->DepthTarget());
					}
					else
					{
						_lightDirectionalDepth.push_back(light->ShadowCameras()->FirstObject<Camera>()->Storage()->DepthTarget());
					}
				}
			}
		}
		
		return static_cast<int>(lightCount);
	}
	
#undef Distance
	
	// ---------------------
	// MARK: -
	// MARK: Camera rendering
	// ---------------------
	
	void Renderer32::FlushCamera(Camera *camera, Shader *drawShader)
	{
		Renderer::FlushCamera(camera, drawShader);
		
		BindVAO(_copyVAO);
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		
		glEnableVertexAttribArray(_currentProgram->attPosition);
		glVertexAttribPointer(_currentProgram->attPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(_currentProgram->attTexcoord0);
		glVertexAttribPointer(_currentProgram->attTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		const Rect& frame = camera->Frame();
		
		glViewport(frame.x * _scaleFactor, frame.y * _scaleFactor, frame.width * _scaleFactor, frame.height * _scaleFactor);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		glDisableVertexAttribArray(_currentProgram->attPosition);
		glDisableVertexAttribArray(_currentProgram->attTexcoord0);
	}
	
	void Renderer32::DrawCameraStage(Camera *camera, Camera *stage)
	{
		Renderer::DrawCameraStage(camera, stage);
		
		BindVAO(_copyVAO);
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		
		glEnableVertexAttribArray(_currentProgram->attPosition);
		glVertexAttribPointer(_currentProgram->attPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(_currentProgram->attTexcoord0);
		glVertexAttribPointer(_currentProgram->attTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		glDisableVertexAttribArray(_currentProgram->attPosition);
		glDisableVertexAttribArray(_currentProgram->attTexcoord0);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Rendering
	// ---------------------
	
	void Renderer32::DrawCamera(Camera *camera, Camera *source, uint32 skyCubeMeshes)
	{
		Renderer::DrawCamera(camera, source, skyCubeMeshes);
		
		bool changedCamera = true;
		bool changedShader;
		bool changedMaterial;
		
		SetDepthWriteEnabled(true);
		SetScissorEnabled(false);
		
		camera->Bind();
		camera->PrepareForRendering();
		
		if(_currentMaterial)
			SetDepthWriteEnabled(_currentMaterial->depthwrite);
		
		bool wantsFog = _currentCamera->usefog;
		bool wantsClipPlane = _currentCamera->useclipplane;
		
		Matrix identityMatrix;
		
		if(!source)
		{
			Material *surfaceMaterial = camera->Material();

			// Create the light lists for the camera
			int lightPointCount = CreatePointLightList(camera);
			int lightSpotCount  = CreateSpotLightList(camera);
			int lightDirectionalCount = CreateDirectionalLightList(camera);
			
			_renderedLights += lightPointCount + lightSpotCount + lightDirectionalCount;
			
			// Update the shader
			const Matrix& projectionMatrix = camera->projectionMatrix;
			const Matrix& inverseProjectionMatrix = camera->inverseProjectionMatrix;
			
			const Matrix& viewMatrix = camera->viewMatrix;
			const Matrix& inverseViewMatrix = camera->inverseViewMatrix;
			
			Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
			Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
			
			size_t objectsCount = _frame.size();
			size_t i = (camera->CameraFlags() & Camera::FlagNoSky) ? skyCubeMeshes : 0;
			
			for(; i<objectsCount; i++)
			{
				RenderingObject& object = _frame[i];
				if(object.prepare)
					object.prepare(this, object);
				
				SetScissorEnabled(object.scissorTest);
				
				if(_scissorTest)
					SetScissorRect(object.scissorRect);
				
				Mesh     *mesh = object.mesh;
				Material *material = object.material;
				Shader   *shader = surfaceMaterial ? surfaceMaterial->Shader() : material->Shader();
				
				Matrix& transform = object.transform ? *object.transform : identityMatrix;
				Matrix inverseTransform = transform.Inverse();
				
				// Check if we can use instancing here
				bool wantsInstancing = (object.type == RenderingObject::Type::Instanced);
				if(RN_EXPECT_FALSE(wantsInstancing))
				{
					if(!shader->SupportsProgramOfType(ShaderProgram::TypeInstanced))
						continue;
				}
				
				// Grab the correct shader program
				uint32 programTypes = 0;
				ShaderProgram *program = 0;
				
				bool wantsDiscard = material->discard;
				if(surfaceMaterial && !(material->override & Material::OverrideDiscard))
					wantsDiscard = surfaceMaterial->discard;
				
				if(object.skeleton && shader->SupportsProgramOfType(ShaderProgram::TypeAnimated))
					programTypes |= ShaderProgram::TypeAnimated;
				
				bool wantsLighting = material->lighting;
				if(surfaceMaterial)
					wantsLighting = surfaceMaterial->lighting;

				if(wantsLighting && shader->SupportsProgramOfType(ShaderProgram::TypeLighting))
				{
					programTypes |= ShaderProgram::TypeLighting;
					
					if(_lightDirectionalDepth.size() > 0)
						programTypes |= ShaderProgram::TypeDirectionalShadows;
				}
				
				if(wantsFog && shader->SupportsProgramOfType(ShaderProgram::TypeFog))
					programTypes |= ShaderProgram::TypeFog;
				
				if(wantsClipPlane && shader->SupportsProgramOfType(ShaderProgram::TypeClipPlane))
					programTypes |= ShaderProgram::TypeClipPlane;
				
				if(wantsInstancing)
					programTypes |= ShaderProgram::TypeInstanced;
				
				if(wantsDiscard && shader->SupportsProgramOfType(ShaderProgram::TypeDiscard))
					programTypes |= ShaderProgram::TypeDiscard;
				
				// Set lighting defines
				std::vector<ShaderDefine> defines;
				if(lightPointCount > 0)
					defines.emplace_back(ShaderDefine("RN_POINT_LIGHTS", MIN(lightPointCount, _maxLightFastPath)));
				if(lightSpotCount > 0)
					defines.emplace_back(ShaderDefine("RN_SPOT_LIGHTS", MIN(lightSpotCount, _maxLightFastPath)));
				if(lightDirectionalCount > 0)
					defines.emplace_back(ShaderDefine("RN_DIRECTIONAL_LIGHTS", lightDirectionalCount));
				
				if(lightPointCount < _maxLightFastPath)
					defines.emplace_back(ShaderDefine("RN_POINT_LIGHTS_FASTPATH", ""));
				if(lightSpotCount < _maxLightFastPath)
					defines.emplace_back(ShaderDefine("RN_SPOT_LIGHTS_FASTPATH", ""));
				
				if(surfaceMaterial)
				{
					program = shader->ProgramWithLookup(surfaceMaterial->Lookup() + material->Lookup() + ShaderLookup(programTypes) + ShaderLookup(defines));
				}
				else
				{
					program = shader->ProgramWithLookup(material->Lookup() + ShaderLookup(programTypes) + ShaderLookup(defines));
				}
				
				RN_ASSERT(program, "");
				
				changedShader   = (_currentProgram != program);
				changedMaterial = (_currentMaterial != material);
				
				BindMaterial(material, program);
				
				// Update the shader data
				if(changedShader || changedCamera)
				{
					UpdateShaderData();
					changedCamera = false;
					changedShader = true;
				}
				
				if(changedShader)
				{
					// Light data
					glUniform1i(program->lightPointCount, lightPointCount);
					glUniform4fv(program->lightPointPosition, lightPointCount, (float*)_lightPointPosition.data());
					glUniform4fv(program->lightPointColor, lightPointCount, (float*)_lightPointColor.data());
					
					glUniform1i(program->lightSpotCount, lightSpotCount);
					glUniform4fv(program->lightSpotPosition, lightSpotCount, (float*)_lightSpotPosition.data());
					glUniform4fv(program->lightSpotDirection, lightSpotCount, (float*)_lightSpotDirection.data());
					glUniform4fv(program->lightSpotColor, lightSpotCount, (float*)_lightSpotColor.data());
					
					glUniform1i(program->lightDirectionalCount, lightDirectionalCount);
					glUniform3fv(program->lightDirectionalDirection, lightDirectionalCount, (float*)_lightDirectionalDirection.data());
					glUniform4fv(program->lightDirectionalColor, lightDirectionalCount, (float*)_lightDirectionalColor.data());
					
					float *data = reinterpret_cast<float *>(_lightDirectionalMatrix.data());
					glUniformMatrix4fv(program->lightDirectionalMatrix, (GLuint)_lightDirectionalMatrix.size(), GL_FALSE, data);
					
					if(camera->DepthTiles() != 0)
					{
						if(program->lightTileSize != -1)
						{
							Rect rect = camera->Frame();
							int tilesWidth  = ceil(rect.width / camera->LightTiles().x);
							int tilesHeight = ceil(rect.height / camera->LightTiles().y);
							
							Vector2 lightTilesSize = camera->LightTiles() * _scaleFactor;
							Vector2 lightTilesCount = Vector2(tilesWidth, tilesHeight);
							
							glUniform4f(program->lightTileSize, lightTilesSize.x, lightTilesSize.y, lightTilesCount.x, lightTilesCount.y);
						}
					}
					
					if(program->discardThreshold != -1)
					{
						float threshold = material->discardThreshold;
						
						if(surfaceMaterial && !(material->override & Material::OverrideDiscardThreshold))
							threshold = surfaceMaterial->discardThreshold;
						
						glUniform1f(program->discardThreshold, threshold);
					}
				}
				
				if(changedShader || changedMaterial)
				{
					if(program->lightDirectionalDepth != -1 && _lightDirectionalDepth.size() > 0)
					{
						uint32 textureUnit = BindTexture(_lightDirectionalDepth.front());
						glUniform1i(program->lightDirectionalDepth, textureUnit);
					}
					
					if(camera->DepthTiles())
					{
						// Point lights
						if(program->lightPointList != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListIndicesIndex]);
							glUniform1i(program->lightPointList, textureUnit);
						}
						
						if(program->lightPointListOffset != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListOffsetIndex]);
							glUniform1i(program->lightPointListOffset, textureUnit);
						}
						
						if(program->lightPointListData != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListDataIndex]);
							glUniform1i(program->lightPointListData, textureUnit);
						}
						
						// Spot lights
						if(program->lightSpotList != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListIndicesIndex]);
							glUniform1i(program->lightSpotList, textureUnit);
						}
						
						if(program->lightSpotListOffset!= -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListOffsetIndex]);
							glUniform1i(program->lightSpotListOffset, textureUnit);
						}
						
						if(program->lightSpotListData != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListDataIndex]);
							glUniform1i(program->lightSpotListData, textureUnit);
						}
					}
					
					glUniform4fv(program->ambient, 1, &material->ambient.r);
					glUniform4fv(program->diffuse, 1, &material->diffuse.r);
					glUniform4fv(program->emissive, 1, &material->emissive.r);
					glUniform4fv(program->specular, 1, &material->specular.r);
					glUniform1f(program->shininess, material->shininess);
				}
				
				if(RN_EXPECT_FALSE(wantsInstancing))
				{
					DrawMeshInstanced(object);
					continue;
				}
				
				// More updates
				if(object.skeleton)
				{
					const float *data = reinterpret_cast<const float *>(object.skeleton->Matrices().data());
					glUniformMatrix4fv(program->matBones, object.skeleton->NumBones(), GL_FALSE, data);
				}
				
				glUniformMatrix4fv(program->matModel, 1, GL_FALSE, transform.m);
				glUniformMatrix4fv(program->matModelInverse, 1, GL_FALSE, inverseTransform.m);
				
				if(object.rotation)
				{
					if(program->matNormal != -1)
						glUniformMatrix4fv(program->matNormal, 1, GL_FALSE, object.rotation->RotationMatrix().m);
					
					if(program->matNormalInverse != -1)
						glUniformMatrix4fv(program->matNormalInverse, 1, GL_FALSE, object.rotation->RotationMatrix().Inverse().m);
				}
				
				if(program->matViewModel != -1)
				{
					Matrix viewModel = viewMatrix * transform;
					glUniformMatrix4fv(program->matViewModel, 1, GL_FALSE, viewModel.m);
				}
				
				if(program->matViewModelInverse != -1)
				{
					Matrix viewModel = inverseViewMatrix * inverseTransform;
					glUniformMatrix4fv(program->matViewModelInverse, 1, GL_FALSE, viewModel.m);
				}
				
				if(program->matProjViewModel != -1)
				{
					Matrix projViewModel = projectionViewMatrix * transform;
					glUniformMatrix4fv(program->matProjViewModel, 1, GL_FALSE, projViewModel.m);
				}
				
				if(program->matProjViewModelInverse != -1)
				{
					Matrix projViewModelInverse = inverseProjectionViewMatrix * inverseTransform;
					glUniformMatrix4fv(program->matProjViewModelInverse, 1, GL_FALSE, projViewModelInverse.m);
				}
				
				if(RN_EXPECT_FALSE(object.type == RenderingObject::Type::Custom))
				{
					object.callback(this, object);
					continue;
				}
				
				DrawMesh(mesh, object.offset, object.count);
			}
		}
		else
		{
			DrawCameraStage(source, camera);
		}
		
		camera->Unbind();
	}
	
	void Renderer32::DrawMesh(Mesh *mesh, uint32 offset, uint32 count)
	{
		bool usesIndices = mesh->SupportsFeature(kMeshFeatureIndices);
		MeshDescriptor *descriptor = usesIndices ? mesh->Descriptor(kMeshFeatureIndices) : mesh->Descriptor(kMeshFeatureVertices);
		
		BindVAO(std::tuple<ShaderProgram *, Mesh *>(_currentProgram, mesh));
		
		GLsizei glCount = static_cast<GLsizei>(descriptor->elementCount);
		if(count != 0)
			glCount = MIN(glCount, count);
		
		if(usesIndices)
		{
			GLenum type;
			switch(descriptor->elementSize)
			{
				case 1:
					type = GL_UNSIGNED_BYTE;
					break;
					
				case 2:
					type = GL_UNSIGNED_SHORT;
					break;
					
				case 4:
					type = GL_UNSIGNED_INT;
					break;
					
				default:
					throw Exception(Exception::Type::InconsistencyException, "");
					break;
			}
			
			glDrawElements(mesh->Mode(), glCount, type, reinterpret_cast<void *>(offset));
		}
		else
		{
			glDrawArrays(mesh->Mode(), 0, glCount);
		}
		
		_renderedVertices += glCount;
		BindVAO(0);
	}
	
	void Renderer32::DrawMeshInstanced(const RenderingObject& object)
	{
		Mesh *mesh = object.mesh;
		MeshDescriptor *descriptor = mesh->Descriptor(kMeshFeatureIndices);
		
		BindVAO(std::tuple<ShaderProgram *, Mesh *>(_currentProgram, mesh));
		RN_ASSERT(_currentProgram->instancingData != -1, "");
		
		uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, object.instancingData);
		glUniform1i(_currentProgram->instancingData, textureUnit);
		
		if(descriptor)
		{
			GLenum type;
			switch(descriptor->elementSize)
			{
				case 1:
					type = GL_UNSIGNED_BYTE;
					break;
					
				case 2:
					type = GL_UNSIGNED_SHORT;
					break;
					
				case 4:
					type = GL_UNSIGNED_INT;
					break;
					
				default:
					throw Exception(Exception::Type::InconsistencyException, "");
					break;
			}
			
			glDrawElementsInstanced(mesh->Mode(), (GLsizei)descriptor->elementCount, type, 0, (GLsizei)object.count);
		}
		else
		{
			descriptor = mesh->Descriptor(kMeshFeatureVertices);
			glDrawArraysInstanced(mesh->Mode(), 0, (GLsizei)descriptor->elementCount, (GLsizei)object.count);
		}
		
		_renderedVertices += descriptor->elementCount * object.count;
		BindVAO(0);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Rendering
	// ---------------------
	
	void Renderer32::SetMaxLightFastPathCount(uint32 maxLights)
	{
		_maxLightFastPath = maxLights;
	}
}
