//
//  RNRenderStorage.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		_sizeChanged   = true;
		_renderTargetsChanged = true;
		
		_boundRenderTargets = 0;
		_renderTargets = new Array();
		_depthTexture = depthTexture ? depthTexture->Retain() : 0;
		_depthLayer = -1;
		
		_framebuffer = _depthbuffer = _stencilbuffer = 0;
		_format      = format;
		
		_fixedScaleFactor = (scaleFactor > 0.0f);
		_scaleFactor = _fixedScaleFactor ? scaleFactor : Kernel::GetSharedInstance()->GetActiveScaleFactor();
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenFramebuffers(1, &_framebuffer);
		});
		
		MessageCenter::GetSharedInstance()->AddObserver(kRNWindowScaleFactorChanged, [this](Message *message) {
			_scaleFactor = Kernel::GetSharedInstance()->GetActiveScaleFactor();
			_sizeChanged = true;
		}, this);
	}
	
	RenderStorage::~RenderStorage()
	{
		if(_depthbuffer)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				gl::DeleteRenderbuffers(1, &_depthbuffer);
			}, true);
			
			_stencilbuffer = 0;
			_stencilbuffer = 0;
		}
		
		if(_framebuffer)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				gl::DeleteFramebuffers(1, &_framebuffer);
			}, true);
		}
		
		if(_renderTargets)
			_renderTargets->Release();
		
		if(_depthTexture)
			_depthTexture->Release();
		
		MessageCenter::GetSharedInstance()->RemoveObserver(this);
	}
	
	
	void RenderStorage::SetSize(const Vector2& size)
	{
		if(_size != size)
		{
			_sizeChanged = true;
			_size = size;
		}
	}
	
	
	void RenderStorage::SetRenderTarget(Texture *target, uint32 index)
	{
		RN_ASSERT(_format & BufferFormatColor, "Need a color buffer to change render targets");
		
		Texture::Parameter parameter = target->GetParameter();
		
		parameter.filter = Texture::Filter::Nearest;
		parameter.wrapMode = Texture::WrapMode::Clamp;
		parameter.maxMipMaps = 0;
		
		target->SetParameter(parameter);
		
		_renderTargets->ReplaceObjectAtIndex(index, target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::RemoveRenderTarget(Texture *target)
	{
		RN_ASSERT(_format & BufferFormatColor, "Need a color buffer to change render targets");
		
		_renderTargets->RemoveObject(target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::AddRenderTarget(Texture *target)
	{
		RN_ASSERT(_format & BufferFormatColor, "Need a color buffer to change render targets");
		
		if(_renderTargets->GetCount() >= GetMaxRenderTargets())
			throw Exception(Exception::Type::InconsistencyException, "Can't attach any more render targets to the render storage!");
		
		Texture::Parameter parameter = target->GetParameter();
		
		parameter.filter = Texture::Filter::Linear;
		parameter.wrapMode = Texture::WrapMode::Clamp;
		parameter.maxMipMaps = 0;
		
		target->SetParameter(parameter);
		
		_renderTargets->AddObject(target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::AddRenderTarget(Texture::Format format)
	{
		RN_ASSERT(_format & BufferFormatColor, "Need a color buffer to change render targets");
		
		Texture::Parameter parameter;
		
		parameter.format = format;
		parameter.filter = Texture::Filter::Linear;
		parameter.wrapMode = Texture::WrapMode::Clamp;
		parameter.maxMipMaps = 0;
		
		Texture *target = new Texture2D(parameter, true);
		
		try
		{
			AddRenderTarget(target);
			target->Release();
		}
		catch(Exception e)
		{
			target->Release();
		}
	}
	
	void RenderStorage::SetDepthTarget(Texture *depthTexture, uint32 layer)
	{
		if(depthTexture)
		{
			RN_ASSERT(_format & BufferFormatDepth, "Depth textures are only supported for render storages with depth support");
		}
		
		if(_depthTexture)
			_depthTexture->Release();
		
		_depthTexture = depthTexture ? depthTexture->Retain() : 0;
		_depthLayer = layer;
		_formatChanged = true;
	}
	
	void RenderStorage::SetDepthTarget(Texture::Format format)
    {
        Texture::Parameter parameter;
        parameter.format = format;
        parameter.wrapMode = Texture::WrapMode::Clamp;
        parameter.maxMipMaps = 0;
        
        Texture *target = new Texture2D(parameter, true);
        
        try
        {
            SetDepthTarget(target);
            target->Release();
        }
        catch(Exception e)
        {
            target->Release();
        }
    }
	
	
	void RenderStorage::CheckFramebufferStatus()
	{
#if RN_TARGET_OPENGL
		GLenum status = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
		
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch(status)
			{
				case GL_FRAMEBUFFER_UNDEFINED:
					throw Exception(Exception::Type::FramebufferException, "");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					throw Exception(Exception::Type::FramebufferIncompleteAttachmentException, "");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					throw Exception(Exception::Type::FramebufferIncompleteMissingAttachmentException, "");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					throw Exception(Exception::Type::FramebufferIncompleteDrawbufferException, "");
					break;
					
				case GL_FRAMEBUFFER_UNSUPPORTED:
					throw Exception(Exception::Type::FramebufferUnsupportedException, "");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					throw Exception(Exception::Type::FramebufferIncompleteMultisampleException, "");
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
					throw Exception(Exception::Type::FramebufferIncompleteLayerException, "");
					break;
					
				default:
				{
					std::stringstream stream;
					stream << "Unknown framebuffer status " << status;
					
					Exception(Exception::Type::GenericException, stream.str());
					break;
				}
			}
		}
#endif
		
#if RN_TARGET_OPENGL_ES
		GLenum status = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
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
		
		GLenum buffers[128];
		
		for(uint32 i = 0; i < count; i ++)
			buffers[i] = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
		
		gl::DrawBuffers(count, buffers);
#endif
	}
	
	void RenderStorage::BindAndUpdateBuffer()
	{
		OpenGLQueue *queue = OpenGLQueue::GetSharedInstance();
		
		queue->SubmitCommand([this] {
			gl::BindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
		});
		
		if(_formatChanged)
		{
			// Remove unused buffers
			if(_depthTexture)
			{
				queue->SubmitCommand([this] {
					if(_stencilbuffer)
					{
						gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
						_stencilbuffer = 0;
					}
					
					if(_depthbuffer)
					{
						gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
						gl::DeleteRenderbuffers(1, &_depthbuffer);
						_depthbuffer = 0;
					}
				});
			}
			
			// Create new buffers
			if((_depthbuffer == 0 && (_format & BufferFormatDepth)) && !_depthTexture)
			{
				queue->SubmitCommand([this] {
					gl::GenRenderbuffers(1, &_depthbuffer);
					gl::BindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
					gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer);
					
					if(_format & BufferFormatStencil)
					{
						_stencilbuffer = _depthbuffer;
						gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilbuffer);
					}
				});
			}
			
			if(_depthTexture)
			{
				queue->SubmitCommand([this] {
					if(_format & BufferFormatDepth)
					{
						switch(_depthTexture->GetGLType())
						{
							case GL_TEXTURE_2D_ARRAY:
								if(_depthLayer != -1)
									gl::FramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->GetName(), 0, _depthLayer);
								else
									gl::FramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->GetName(), 0);
								break;
								
							case GL_TEXTURE_2D:
								gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->GetName(), 0);
								break;
								
							case GL_TEXTURE_CUBE_MAP:
								if(_depthLayer != -1)
									gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X+_depthLayer, _depthTexture->GetName(), 0);
								else
									gl::FramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->GetName(), 0);
								break;
						}
					}
					
					if(_format & BufferFormatStencil)
					{
						switch(_depthTexture->GetGLType())
						{
							case GL_TEXTURE_2D_ARRAY:
								if(_depthLayer != -1)
									gl::FramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->GetName(), 0, _depthLayer);
								else
									gl::FramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->GetName(), 0);
								break;
								
							case GL_TEXTURE_2D:
								gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->GetName(), 0);
								break;
								
							case GL_TEXTURE_CUBE_MAP:
								if(_depthLayer != -1)
									gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X+_depthLayer, _depthTexture->GetName(), 0);
								else
									gl::FramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture->GetName(), 0);
								break;
						}
					}
				});
			}
			
			_formatChanged = false;
			_sizeChanged   = true;
		}
		
		if(_renderTargetsChanged && (_format & BufferFormatColor))
		{
			queue->SubmitCommand([&] {
				// Unbind no longer used render targets
				for(size_t i=_renderTargets->GetCount(); i<_boundRenderTargets; i++)
				{
					GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
					gl::FramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0);
				}
				
				// Bind all render targetst to the framebuffer
				for(size_t i=0; i<_renderTargets->GetCount(); i++)
				{
					Texture *texture = _renderTargets->GetObjectAtIndex<Texture>(i);
					GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
					
					gl::FramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->GetName(), 0);
				}
			}, true);
			
			_boundRenderTargets = (uint32)_renderTargets->GetCount();
			_renderTargetsChanged = false;
			
			queue->SubmitCommand([&] {
				UpdateDrawBuffers(_boundRenderTargets);
			}, true);
		}
		
		// Allocate storage for the buffers
		if(_sizeChanged)
		{
			size_t width  = static_cast<size_t>(ceil(_size.x * _scaleFactor));
			size_t height = static_cast<size_t>(ceil(_size.y * _scaleFactor));
			
			for(size_t i = 0; i < _renderTargets->GetCount(); i ++)
			{
				Texture2D *texture = _renderTargets->GetObjectAtIndex<Texture2D>(i);
				texture->SetSize(width, height);
			}
			
			if(_depthTexture)
			{
				_depthTexture->SetSize(width, height);
			}
			else if((_format & BufferFormatDepth) || (_format & BufferFormatStencil))
			{
				queue->SubmitCommand([this, width, height] {
					GLenum format = (!(_format & BufferFormatStencil)) ? GL_DEPTH_COMPONENT24 : GL_DEPTH24_STENCIL8;
					
					gl::BindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
					gl::RenderbufferStorage(GL_RENDERBUFFER, format, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
				});
			}
			
			bool failed = false;
			Exception exception(Exception::Type::GenericException, "This exception shouldn't be thrown!");
			
			queue->SubmitCommand([&] {
				try
				{
					CheckFramebufferStatus();
				}
				catch(Exception e)
				{
					exception = std::move(e);
					failed = true;
				}
			}, true);
				
			if(failed)
				throw exception;
			
			_sizeChanged = false;
		}
	}
	
	uint32 RenderStorage::GetMaxRenderTargets()
	{
#if GL_MAX_COLOR_ATTACHMENTS
		static GLint maxDrawbuffers = 0;
		
		if(maxDrawbuffers == 0)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				gl::GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxDrawbuffers);
			}, true);
		}
		
		return static_cast<uint32>(maxDrawbuffers);
#endif
		
		return 1;
	}
}
