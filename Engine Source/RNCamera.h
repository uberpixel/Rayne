//
//  RNCamera.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAMERA_H__
#define __RAYNE_CAMERA_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRenderingResource.h"
#include "RNRect.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNColor.h"
#include "RNTexture.h"

namespace RN
{
	class Camera : public Object, public RenderingResource
	{
	public:
		Camera(const Vector2& size);
		Camera(GLuint framebuffer, const Vector2& size);
		virtual ~Camera();
		
		void Bind();
		void Unbind();
		virtual void PrepareForRendering();
		
		virtual void SetFrame(const Rect& frame);
		void SetClearColor(const Color& color);
		
		virtual void UpdateProjection();
		virtual void UpdateCamera();
		
		const Rect& Frame() const;
		const Color& ClearColor() const;
		
		const Matrix& ProjectionMatrix() const { return _projectionMatrix; }
		const Matrix& InverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		
		const Matrix& ViewMatrix() const { return _viewMatrix; }
		const Matrix& InverseViewMatrix() const { return _inverseViewMatrix; }
		
		GLuint DepthBuffer() const { return _depthBuffer; }
		GLuint SteincilBuffer() const { return _stencilBuffer; }
		
		Texture *Target() const { return _texture; }
		
		float arc;
		float aspect;
		float clipnear;
		float clipfar;
		
		Vector3 position;
		Quaternion rotation;
		
	protected:
		void CheckError();
		
		Rect _frame;
		Color _clearColor;
		
		Matrix _projectionMatrix;
		Matrix _inverseProjectionMatrix;
		
		Matrix _viewMatrix;
		Matrix _inverseViewMatrix;
		
		bool _ownsBuffer;
		GLuint _frameBuffer;
		GLuint _depthBuffer;
		GLuint _stencilBuffer;
		
	private:
		int _current;
		Texture *_texture;
		
		void SetDefaultValues();
	};
}

#endif /* __RAYNE_CAMERA_H__ */
