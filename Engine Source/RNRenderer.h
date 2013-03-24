//
//  RNRenderer.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
#include "RNLightEntity.h"

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
		Renderer();
		~Renderer();
		
		RNAPI void BeginFrame(float delta);
		RNAPI void FinishFrame();
		
		RNAPI void BeginCamera(Camera *camera);
		RNAPI void FlushCamera();
		RNAPI void RenderObject(const RenderingObject& object);
		RNAPI void RenderLight(LightEntity *light);
		
		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);
		
		RNAPI void BindMaterial(Material *material, ShaderProgram *program);
		
		uint32 BindTexture(Texture *texture);
		uint32 BindTexture(GLenum type, GLuint texture);
		void BindVAO(GLuint vao);
		void UseShader(ShaderProgram *shader);
		
		void SetCullingEnabled(bool enabled);
		void SetDepthTestEanbled(bool enabled);
		void SetDepthWriteEnabled(bool enabled);
		void SetBlendingEnabled(bool enabled);
		
		void SetCullMode(GLenum cullMode);
		void SetDepthFunction(GLenum depthFunction);
		void SetBlendFunction(GLenum blendSource, GLenum blendDestination);
		
	protected:
		RNAPI void UpdateShaderData();
		RNAPI void DrawMesh(Mesh *mesh);
		RNAPI void BindVAO(const std::tuple<ShaderProgram *, MeshLODStage *>& tuple);
		RNAPI void CreatePointLightList(Camera *camera, Vector4 **outLightPos, Vector4 **outLightColor, int *outLightCount);
		
		bool _hasValidFramebuffer;
		
		float _scaleFactor;
		float _time;
		
		std::map<std::tuple<ShaderProgram *, MeshLODStage *>, std::tuple<GLuint, uint32>> _autoVAOs;
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
		Array<LightEntity *> _pointLights;
		Array<LightEntity *> _spotLights;
		
	private:
		void Initialize();
		void FlushCamera(Camera *camera);
		void DrawCameraStage(Camera *camera, Camera *stage);
		
		Shader *_copyShader;
		GLuint _copyVAO;
		GLuint _copyVBO;
		GLuint _copyIBO;
		
		Vector4 _copyVertices[4];
		GLshort _copyIndices[6];
		
		GLuint _lightPointTextures[4];
		GLuint _lightPointBuffers[4];
		GLuint _lightSpotTextures[5];
		GLuint _lightSpotBuffers[5];
		
		uint32 _lightPointBufferLengths[3];
		uint32 _lightSpotBufferLengths[3];
		
		int *_lightPointIndexOffset;
		int *_lightPointIndices;
		size_t _lightPointIndexOffsetSize;
		size_t _lightPointIndicesSize;
		
		int *_lightSpotIndexOffset;
		int *_lightSpotIndices;
		size_t _lightSpotIndexOffsetSize;
		size_t _lightSpotIndicesSize;
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
	
	RN_INLINE void Renderer::SetDepthTestEanbled(bool enabled)
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
		if(_depthFunc != depthFunction)
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
