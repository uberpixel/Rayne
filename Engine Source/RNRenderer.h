//
//  RNRenderer.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERER_H__
#define __RAYNE_RENDERER_H__

#include "RNBase.h"
#include "RNMatrixQuaternion.h"
#include "RNTexture.h"
#include "RNShader.h"
#include "RNMaterial.h"
#include "RNMesh.h"
#include "RNSkeleton.h"
#include "RNLight.h"

namespace RN
{
	struct RenderingObject
	{
		Mesh     *mesh;
		Material *material;
		Matrix   *transform;
		Skeleton *skeleton;
	};
	
	class Renderer : public Singleton<Renderer>
	{
	public:
		RNAPI Renderer();
		RNAPI ~Renderer();
		
		RNAPI void BeginFrame(float delta);
		RNAPI void FinishFrame();
		
		RNAPI void BeginCamera(Camera *camera);
		RNAPI void FinishCamera();
		
		RNAPI void RenderObject(const RenderingObject& object);
		RNAPI void RenderLight(Light *light);
		
		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);
		
		RNAPI void BindMaterial(Material *material, ShaderProgram *program);
		
		RNAPI uint32 BindTexture(Texture *texture);
		RNAPI uint32 BindTexture(GLenum type, GLuint texture);
		RNAPI void BindVAO(GLuint vao);
		RNAPI void UseShader(ShaderProgram *shader);
		
		RNAPI void SetCullingEnabled(bool enabled);
		RNAPI void SetDepthTestEnabled(bool enabled);
		RNAPI void SetDepthWriteEnabled(bool enabled);
		RNAPI void SetBlendingEnabled(bool enabled);
		
		RNAPI void SetCullMode(GLenum cullMode);
		RNAPI void SetDepthFunction(GLenum depthFunction);
		RNAPI void SetBlendFunction(GLenum blendSource, GLenum blendDestination);
		
	protected:
		RNAPI void UpdateShaderData();
		RNAPI void DrawMesh(Mesh *mesh);
		RNAPI void DrawMeshInstanced(machine_uint start, machine_uint count);
		RNAPI void BindVAO(const std::tuple<ShaderProgram *, Mesh *>& tuple);
		
		RNAPI void CullLights(Camera *camera, Light **lights, machine_uint lightCount, GLuint indicesBuffer, GLuint offsetBuffer);
		
		RNAPI int CreatePointLightList(Camera *camera);
		RNAPI int CreateSpotLightList(Camera *camera);
		RNAPI int CreateDirectionalLightList(Camera *camera);
		
		bool _hasValidFramebuffer;
		
		float _scaleFactor;
		float _time;
		
		std::map<std::tuple<ShaderProgram *, Mesh *>, std::tuple<GLuint, uint32>> _autoVAOs;
		std::vector<Camera *> _flushCameras;
		
		GLuint _defaultFBO;
		uint32 _defaultWidth;
		uint32 _defaultHeight;
		
		uint32 _textureUnit;
		uint32 _maxTextureUnits;
		
		Camera        *_currentCamera;
		Material      *_currentMaterial;
		ShaderProgram *_currentProgram;
		GLuint         _currentVAO;
		
		bool _cullingEnabled;
		bool _depthTestEnabled;
		bool _blendingEnabled;
		bool _depthWrite;
		
		GLenum _cullMode;
		GLenum _depthFunc;
		
		GLenum _blendSource;
		GLenum _blendDestination;
		
		Camera *_frameCamera;
		Array<RenderingObject> _frame;
		Array<Light *> _pointLights;
		Array<Light *> _spotLights;
		Array<Light *> _directionalLights;
		
	private:
		void Initialize();
		void FlushCamera(Camera *camera);
		void DrawCameraStage(Camera *camera, Camera *stage);
		void AllocateLightBufferStorage(size_t indicesSize, size_t offsetSize);
		
		Shader *_copyShader;
		GLuint _copyVAO;
		GLuint _copyVBO;
		GLuint _copyIBO;
		
		Vector4 _copyVertices[4];
		GLshort _copyIndices[6];
		
		int *_lightIndicesBuffer;
		int *_tempLightIndicesBuffer;
		int *_lightOffsetBuffer;
		size_t _lightIndicesBufferSize;
		size_t _lightOffsetBufferSize;
		
		size_t _lightPointDataSize;
		GLuint _lightPointTextures[3];
		GLuint _lightPointBuffers[3];
		
		size_t _lightSpotDataSize;
		GLuint _lightSpotTextures[3];
		GLuint _lightSpotBuffers[3];
		
		Array<Vector3> _lightDirectionalDirection;
		Array<Vector3> _lightDirectionalColor;
		
		size_t _instancingVBOSize;
		GLuint _instancingVBO;
	};
	
	RN_INLINE uint32 Renderer::BindTexture(Texture *texture)
	{
		_textureUnit ++;
		_textureUnit %= _maxTextureUnits;
		
		glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
		glBindTexture(GL_TEXTURE_2D, texture->Name());
		
		return _textureUnit;
	}
	
	RN_INLINE uint32 Renderer::BindTexture(GLenum type, GLuint texture)
	{
		_textureUnit ++;
		_textureUnit %= _maxTextureUnits;
		
		glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
		glBindTexture(type, texture);
		
		return _textureUnit;
	}
	
	RN_INLINE void Renderer::BindVAO(GLuint vao)
	{
		if(_currentVAO != vao)
		{
			gl::BindVertexArray(vao);
			_currentVAO = vao;
		}
	}
	
	RN_INLINE void Renderer::UseShader(ShaderProgram *shader)
	{
		if(_currentProgram != shader)
		{
			glUseProgram(shader->program);
			if(shader->time != -1)
				glUniform1f(shader->time, _time);
			
			_currentProgram = shader;
		}
	}
	
	
	RN_INLINE void Renderer::SetCullingEnabled(bool enabled)
	{
		if(_cullingEnabled == enabled)
			return;
		
		_cullingEnabled = enabled;
		_cullingEnabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	}
	
	RN_INLINE void Renderer::SetDepthTestEnabled(bool enabled)
	{
		if(_depthTestEnabled == enabled)
			return;
		
		_depthTestEnabled = enabled;
		_depthTestEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	}
	
	RN_INLINE void Renderer::SetDepthWriteEnabled(bool enabled)
	{
		if(_depthWrite == enabled)
			return;
		
		_depthWrite = enabled;
		glDepthMask(_depthWrite ? GL_TRUE : GL_FALSE);
	}
	
	RN_INLINE void Renderer::SetBlendingEnabled(bool enabled)
	{
		if(_blendingEnabled == enabled)
			return;
		
		_blendingEnabled = enabled;
		_blendingEnabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
	}
	
	
	RN_INLINE void Renderer::SetCullMode(GLenum cullMode)
	{
		if(_cullMode == cullMode)
			return;
		
		glCullFace(cullMode);
		_cullMode = cullMode;
	}
	
	RN_INLINE void Renderer::SetDepthFunction(GLenum depthFunction)
	{
		if(_depthFunc == depthFunction)
			return;
		
		glDepthFunc(depthFunction);
		_depthFunc = depthFunction;
	}
	
	RN_INLINE void Renderer::SetBlendFunction(GLenum blendSource, GLenum blendDestination)
	{
		if(_blendSource != blendSource || _blendDestination != blendDestination)
		{
			glBlendFunc(_blendSource, _blendDestination);
			
			
			_blendSource = blendSource;
			_blendDestination = blendDestination;
		}
	}
	
}

#endif /* __RAYNE_RENDERER_H__ */
