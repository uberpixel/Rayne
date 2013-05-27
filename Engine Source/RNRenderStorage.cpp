//
//  RNRenderStorage.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderStorage.h"
#include "RNKernel.h"

namespace RN
{
	RNDeclareMeta(RenderStorage)
	
	RenderStorage::RenderStorage(BufferFormat format, Texture *depthTexture, float scaleFactor)
	{
		_formatChanged = true;
		_frameChanged  = true;
		_renderTargetsChanged = true;
		
		_boundRenderTargets = 0;
		_renderTargets = new Array();
		_depthTexture = depthTexture ? depthTexture->Retain() : 0;
		_depthLayer = -1;
		
		_framebuffer = _depthbuffer = _stencilbuffer = 0;
		_scaleFactor = (scaleFactor > 0.0f)? scaleFactor : Kernel::SharedInstance()->ScaleFactor();
		_format      = format;
		
		glGenFramebuffers(1, &_framebuffer);
	}
	
	RenderStorage::~RenderStorage()
	{
		if(_depthbuffer)
		{
			glDeleteRenderbuffers(1, &_depthbuffer);
			_stencilbuffer = 0;
			_stencilbuffer = 0;
		}
		
		if(_framebuffer)
			glDeleteFramebuffers(1, &_framebuffer);
		
		if(_renderTargets)
			_renderTargets->Release();
		
		if(_depthTexture)
			_depthTexture->Release();
	}
	
	
	void RenderStorage::SetFrame(const Rect& frame)
	{
		if(_frame != frame)
		{
			_frameChanged = true;
			_frame = frame;
		}
	}
	
	
	void RenderStorage::SetRenderTarget(Texture *target, uint32 index)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		TextureParameter parameter = target->Parameter();
		
		parameter.filter = TextureParameter::Filter::Nearest;
		parameter.wrapMode = TextureParameter::WrapMode::Clamp;
		parameter.mipMaps = 0;
		parameter.generateMipMaps = false;
		
		target->SetParameter(parameter);
		
		_renderTargets->ReplaceObjectAtIndex(index, target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::RemoveRenderTarget(Texture *target)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		_renderTargets->RemoveObject(target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::AddRenderTarget(Texture *target)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		if(_renderTargets->Count() >= MaxRenderTargets())
			throw ErrorException(0, 0, 0);
		
		TextureParameter parameter = target->Parameter();
		
		parameter.filter = TextureParameter::Filter::Linear;
		parameter.wrapMode = TextureParameter::WrapMode::Clamp;
		parameter.mipMaps = 0;
		parameter.generateMipMaps = false;
		
		target->SetParameter(parameter);
		
		_renderTargets->AddObject(target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::AddRenderTarget(TextureParameter::Format format)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		TextureParameter parameter;
		
		parameter.format = format;
		parameter.filter = TextureParameter::Filter::Linear;
		parameter.wrapMode = TextureParameter::WrapMode::Clamp;
		parameter.mipMaps = 0;
		parameter.generateMipMaps = false;
		
		Texture *target = new Texture(parameter, true);
		
		try
		{
			AddRenderTarget(target);
			target->Release();
		}
		catch(ErrorException e)
		{
			target->Release();
		}
	}
	
	void RenderStorage::SetDepthTarget(Texture *depthTexture, uint32 layer)
	{
		if(depthTexture)
		{
			RN_ASSERT(_format & BufferFormatDepth, "Depth textures are only supported for render storages with depth support");
			
			if(_format & BufferFormatStencil)
			{
				RN_ASSERT0(depthTexture->Parameter().format == TextureParameter::Format::DepthStencil);
			}
			else
			{
				RN_ASSERT0(depthTexture->Parameter().format == TextureParameter::Format::Depth);
			}
		}
		
		if(_depthTexture)
			_depthTexture->Release();
		
		_depthTexture = depthTexture ? depthTexture->Retain() : 0;
		_depthLayer = layer;
		_formatChanged = true;
	}
	
	void RenderStorage::SetDepthTarget(TextureParameter::Format format)
    {
        TextureParameter parameter;
        parameter.format = format;
        parameter.wrapMode = TextureParameter::WrapMode::Clamp;
        parameter.mipMaps = 0;
        parameter.generateMipMaps = false;
        
        Texture *target = new Texture(parameter, true);
        
        try
        {
            SetDepthTarget(target);
            target->Release();
        }
        catch(ErrorException e)
        {
            target->Release();
        }
    }
	
	
	void RenderStorage::Bind()
	{
		Thread *thread = Thread::CurrentThread();
		if(thread->SetOpenGLBinding(GL_FRAMEBUFFER, _framebuffer) == 1)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
		}
	}
	
	void RenderStorage::Unbind()
	{
		Thread *thread = Thread::CurrentThread();
		thread->SetOpenGLBinding(GL_FRAMEBUFFER, 0);
	}
	
	void RenderStorage::CheckFramebufferStatus()
	{
#if RN_TARGET_OPENGL
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch(status)
			{
				case GL_FRAMEBUFFER_UNDEFINED:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUndefined, "GL_FRAMEBUFFER_UNDEFINED");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteAttachment, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteMissingAttachment, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteDrawBuffer, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
					break;
					
				case GL_FRAMEBUFFER_UNSUPPORTED:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUnsupported, "GL_FRAMEBUFFER_UNSUPPORTED");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteMultisample, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteLayerTargets, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
					break;
					
				default:
				{
					char buffer[128];
					sprintf(buffer, "Unknown framebuffer status %i\n", status);
					
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferGenericError, std::string(buffer));
					break;
				}
			}
		}
#endif
		
#if RN_TARGET_OPENGL_ES
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch(status)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteAttachment);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteDimensions);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteMissingAttachment);
					break;
					
				case GL_FRAMEBUFFER_UNSUPPORTED:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUnsupported);
					break;
					
				default:
					printf("Unknown framebuffer status %i\n", status);
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferGenericError);
					break;
			}
		}
#endif
	}
	
	void RenderStorage::UpdateDrawBuffers(uint32 count)
	{
#if RN_TARGET_OPENGL
		if(count == 0)
			return;
		
		GLenum buffers[count];
		
		for(uint32 i=0; i<count; i++)
			buffers[i] = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
		
		glDrawBuffers(count, buffers);
#endif
	}
	
	void RenderStorage::UpdateBuffer()
	{
		if(_formatChanged)
		{
			// Remove unused buffers
			if(_depthTexture)
			{
				if(_stencilbuffer)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
					_stencilbuffer = 0;
				}
				
				if(_depthbuffer)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
					glDeleteRenderbuffers(1, &_depthbuffer);
					_depthbuffer = 0;
				}
			}
			
			// Create new buffers
			if((_depthbuffer == 0 && (_format & BufferFormatDepth)) && !_depthTexture)
			{
				glGenRenderbuffers(1, &_depthbuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer);
				
				if(_format & BufferFormatStencil)
				{
					_stencilbuffer = _depthbuffer;
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilbuffer);
				}
			}
			
			if(_depthTexture)
			{
				if(_format & BufferFormatDepth)
				{
					switch(_depthTexture->GLType())
					{
						case GL_TEXTURE_2D_ARRAY:
							if(_depthLayer != -1)
							{
								glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->Name(), 0, _depthLayer);
							}
							else
							{
								glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->Name(), 0);
							}
							break;
							
						case GL_TEXTURE_2D:
							glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->Name(), 0);
							break;
					}
				}
				
				if(_format & BufferFormatStencil)
				{
					switch(_depthTexture->GLType())
					{
						case GL_TEXTURE_2D_ARRAY:
							if(_depthLayer != -1)
							{
								glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->Name(), 0, _depthLayer);
							}
							else
							{
								glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->Name(), 0);
							}
							break;
							
						case GL_TEXTURE_2D:
							glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->Name(), 0);
							break;
					}
				}
			}
			
			_formatChanged = false;
			_frameChanged  = true;
		}
		
		if(_renderTargetsChanged && (_format & BufferFormatColor))
		{
			// Unbind no longer used render targets
			for(machine_uint i=_renderTargets->Count(); i<_boundRenderTargets; i++)
			{
				GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0);
			}
			
			// Bind all render targetst to the framebuffer
			for(machine_uint i=0; i<_renderTargets->Count(); i++)
			{
				Texture *texture = _renderTargets->ObjectAtIndex<Texture>(i);
				GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
				
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->Name(), 0);
			}
			
			_boundRenderTargets = (uint32)_renderTargets->Count();
			_renderTargetsChanged = false;
			
			UpdateDrawBuffers(_boundRenderTargets);
		}
		
		// Allocate storage for the buffers
		if(_frameChanged)
		{
			uint32 width  = (uint32)ceil(_frame.width  * _scaleFactor);
			uint32 height = (uint32)ceil(_frame.height * _scaleFactor);
			
			for(machine_uint i=0; i<_renderTargets->Count(); i++)
			{
				Texture *texture = _renderTargets->ObjectAtIndex<Texture>(i);
				
				texture->Bind();
				texture->SetData(0, width, height, TextureParameter::Format::RGBA8888);
				texture->Unbind();
			}
			
			if(_depthTexture)
			{
				_depthTexture->Bind();
				_depthTexture->SetData(0, width, height, TextureParameter::Format::RGBA8888);
				_depthTexture->Unbind();
			}
			else if((_format & BufferFormatDepth) || (_format & BufferFormatStencil))
			{
				GLenum format = (!(_format & BufferFormatStencil)) ? GL_DEPTH_COMPONENT24 : GL_DEPTH24_STENCIL8;
				
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
			}
			
			CheckFramebufferStatus();
			_frameChanged = false;
		}
	}
	
	uint32 RenderStorage::MaxRenderTargets()
	{
#if GL_MAX_COLOR_ATTACHMENTS
		static GLint maxDrawbuffers = 0;
		
		if(maxDrawbuffers == 0)
			glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxDrawbuffers);
		
		return (uint32)maxDrawbuffers;
#endif
		
		return 1;
	}
}
