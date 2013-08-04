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
	class Renderer;
	class RenderingObject
	{
	public:
		enum class Type
		{
			Object,
			Instanced,
			Custom
		};
		
		RenderingObject(Type ttype=Type::Object) :
			type(ttype)
		{
			offset = 0;
			count  = 0;
			
			instancingData = 0;
			
			mesh      = 0;
			material  = 0;
			rotation  = 0;
			transform = 0;
			skeleton  = 0;
			
			scissorTest = false;
		}
		
		Type type;
		uint32 offset;
		uint32 count;
		
		Mesh     *mesh;
		Material *material;
		Quaternion *rotation;
		Matrix   *transform;
		Skeleton *skeleton;
		
		bool scissorTest;
		Rect scissorRect;
		
		GLuint instancingData;
		std::function<void (Renderer *renderer, RenderingObject&)> prepare;
		std::function<void (Renderer *renderer, const RenderingObject&)> callback;
	};
	
	class Renderer : public NonConstructingSingleton<Renderer>
	{
	public:
		enum class Mode
		{
			ModeWorld,
			ModeUI
		};
		
		RNAPI Renderer();
		RNAPI virtual ~Renderer();
		
		RNAPI void SetDefaultFBO(GLuint fbo);
		RNAPI void SetDefaultFrame(uint32 width, uint32 height);
		RNAPI void SetDefaultFactor(float width, float height);
		
		RNAPI virtual void BeginFrame(float delta);
		RNAPI virtual void FinishFrame();
		
		RNAPI virtual void BeginCamera(Camera *camera);
		RNAPI virtual void FinishCamera();
		
		RNAPI virtual void RenderObject(RenderingObject object);
		RNAPI virtual void RenderDebugObject(RenderingObject object, Mode mode);
		RNAPI virtual void RenderLight(Light *light);
		
		RNAPI virtual void BindMaterial(Material *material, ShaderProgram *program);
		
		RNAPI uint32 BindTexture(Texture *texture);
		RNAPI uint32 BindTexture(GLenum type, GLuint texture);
		RNAPI void BindVAO(GLuint vao);
		RNAPI void UseShader(ShaderProgram *shader);
		
		RNAPI void SetHDRExposure(float exposure);
		RNAPI void SetHDRWhitePoint(float whitepoint);
		
		RNAPI void SetCullingEnabled(bool enabled);
		RNAPI void SetDepthTestEnabled(bool enabled);
		RNAPI void SetDepthWriteEnabled(bool enabled);
		RNAPI void SetBlendingEnabled(bool enabled);
		RNAPI void SetPolygonOffsetEnabled(bool enabled);
		RNAPI void SetMode(Mode mode);
		RNAPI void SetScissorEnabled(bool enabled);
		RNAPI void SetScissorRect(const Rect& rect);
		
		RNAPI void SetCullMode(GLenum cullMode);
		RNAPI void SetDepthFunction(GLenum depthFunction);
		RNAPI void SetBlendFunction(GLenum blendSource, GLenum blendDestination);
		RNAPI void SetPolygonOffset(float factor, float units);
		
		RNAPI void RelinquishMesh(Mesh *mesh);
		RNAPI void RelinquishProgram(ShaderProgram *program);
		
		Camera *ActiveCamera() const { return _currentCamera; }
		Material *ActiveMaterial() const { return _currentMaterial; }
		ShaderProgram *ActiveProgram() const { return _currentProgram; }
		
		float HDRExposure() const { return _hdrExposure; }
		float HDRWhitepoint() const { return _hdrWhitePoint; }
		
		uint32 RenderedVertices() const { return _renderedVertices; }
		uint32 RenderedLights() const { return _renderedLights; }
		
	protected:
		RNAPI virtual void UpdateShaderData();
		RNAPI virtual void DrawCamera(Camera *camera, Camera *source, uint32 skyCubeMeshes);
		RNAPI virtual void BindVAO(const std::tuple<ShaderProgram *, Mesh *>& tuple);
		
		RNAPI virtual void FlushCamera(Camera *camera, Shader *drawShader);
		RNAPI virtual void DrawCameraStage(Camera *camera, Camera *stage);
		
		bool _hasValidFramebuffer;
		
		uint32 _renderedLights;
		uint32 _renderedVertices;
		
		float _scaleFactor;
		float _time;
		
		float _hdrExposure;
		float _hdrWhitePoint;
		
		GLuint _defaultFBO;
		uint32 _defaultWidth;
		uint32 _defaultHeight;
		float _defaultWidthFactor;
		float _defaultHeightFactor;
		
		uint32 _textureUnit;
		uint32 _maxTextureUnits;
		
		Camera        *_currentCamera;
		Material      *_currentMaterial;
		ShaderProgram *_currentProgram;
		GLuint         _currentVAO;
		
		bool _gammaCorrection;
		Mode _mode;
		
		bool _cullingEnabled;
		bool _depthTestEnabled;
		bool _blendingEnabled;
		bool _depthWrite;
		bool _polygonOffsetEnabled;
		bool _scissorTest;
		
		GLenum _cullMode;
		GLenum _depthFunc;
		
		GLenum _blendSource;
		GLenum _blendDestination;
		
		float _polygonOffsetFactor;
		float _polygonOffsetUnits;
		
		Camera *_frameCamera;
		std::vector<RenderingObject> _frame;
		
		std::vector<RenderingObject> _debugFrameWorld;
		std::vector<RenderingObject> _debugFrameUI;
		
		std::vector<Light *> _pointLights;
		std::vector<Light *> _spotLights;
		std::vector<Light *> _directionalLights;
		
	private:		
		std::map<std::tuple<ShaderProgram *, Mesh *>, std::tuple<GLuint, uint32>> _autoVAOs;
		std::vector<std::pair<Camera *, Shader *>> _flushCameras;
		
	};
	
	RN_INLINE uint32 Renderer::BindTexture(Texture *texture)
	{
		glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + _textureUnit));
		glBindTexture(texture->GLType(), texture->Name());
		
		uint32 unit = _textureUnit;
		
		_textureUnit ++;
		_textureUnit %= _maxTextureUnits;
		
		return unit;
	}
	
	RN_INLINE uint32 Renderer::BindTexture(GLenum type, GLuint texture)
	{
		glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + _textureUnit));
		glBindTexture(type, texture);
		
		uint32 unit = _textureUnit;
		
		_textureUnit ++;
		_textureUnit %= _maxTextureUnits;
		
		return unit;
	}
	
	RN_INLINE void Renderer::BindVAO(GLuint vao)
	{
		if(_currentVAO != vao)
		{
			glBindVertexArray(vao);
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
	
	RN_INLINE void Renderer::SetScissorEnabled(bool enabled)
	{
		if(_scissorTest == enabled)
			return;
		
		_scissorTest = enabled;
		_scissorTest ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
	}
	
	RN_INLINE void Renderer::SetScissorRect(const Rect& rect)
	{
		glScissor(rect.x*_scaleFactor, rect.y*_scaleFactor, rect.width*_scaleFactor, rect.height*_scaleFactor);
	}
	
	RN_INLINE void Renderer::SetHDRExposure(float exposure)
	{
		_hdrExposure = exposure;
	}
	
	RN_INLINE void Renderer::SetHDRWhitePoint(float whitepoint)
	{
		_hdrWhitePoint = whitepoint;
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
	
	RN_INLINE void Renderer::SetPolygonOffsetEnabled(bool enabled)
	{
		if(_polygonOffsetEnabled == enabled)
			return;
		
		_polygonOffsetEnabled = enabled;
		_polygonOffsetEnabled ? glEnable(GL_POLYGON_OFFSET_FILL) : glDisable(GL_POLYGON_OFFSET_FILL);
	}
	
	
	RN_INLINE void Renderer::SetCullMode(GLenum cullMode)
	{
		if(_cullMode == cullMode)
			return;
		
		glFrontFace(cullMode);
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
			glBlendFunc(blendSource, blendDestination);
			
			_blendSource = blendSource;
			_blendDestination = blendDestination;
		}
	}
	
	RN_INLINE void Renderer::SetPolygonOffset(float factor, float units)
	{
		if(_polygonOffsetFactor != factor || _polygonOffsetUnits != units)
		{
			glPolygonOffset(factor, units);
			
			_polygonOffsetFactor = factor;
			_polygonOffsetUnits  = units;
		}
	}
	
}

#endif /* __RAYNE_RENDERER_H__ */
