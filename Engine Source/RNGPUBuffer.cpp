//
//  RNGPUBuffer.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNGPUBuffer.h"
#include "RNOpenGLQueue.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(GPUBuffer, Object)
	RNDefineMeta(GPUScalarBuffer, GPUBuffer)
	RNDefineMeta(GPUTextureBuffer, GPUBuffer)
	
	// ---------------------
	// MARK: -
	// MARK: GPUBuffer
	// ---------------------
	
	GPUBuffer::GPUBuffer()
	{}
	
	void GPUBuffer::SetBindPoint(const std::string &bindPoint)
	{
		_bindPoint = bindPoint;
	}
	
	// ---------------------
	// MARK: -
	// MARK: GPUScalarBuffer
	// ---------------------
	
	void GPUScalarBuffer::Bind(Renderer *renderer, ShaderProgram *program)
	{
		
	}
	
	// ---------------------
	// MARK: -
	// MARK: GPUTexture
	// ---------------------
	
	GPUTextureBuffer::GPUTextureBuffer() :
		_size(0),
		_data(nullptr)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenTextures(1, &_texture);
			gl::GenBuffers(1, &_buffer);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R8, _buffer);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		});
	}
	
	GPUTextureBuffer::~GPUTextureBuffer()
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteTextures(1, &_texture);
			gl::DeleteBuffers(1, &_buffer);
		}, true);
	}
	
	void GPUTextureBuffer::SetSize(size_t size)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this, size] {
			
			if(size < _size)
				return;
			
			uint8 *temp = new uint8[size];
			size_t psize = _size;
			
			if(_data)
			{
				std::copy(_data, _data + std::min(_size, size), temp);
				delete [] _data;
			}
			
			_data = temp;
			_size = size;
			
			gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferData(GL_TEXTURE_BUFFER, psize, nullptr, GL_STATIC_DRAW);
			gl::BufferData(GL_TEXTURE_BUFFER, size, _data, GL_STATIC_DRAW);
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		});
	}
	
	void GPUTextureBuffer::SetData(const void *data)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			
			const uint8 *temp = reinterpret_cast<const uint8 *>(data);
			std::copy(temp, temp + _size, _data);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferData(GL_TEXTURE_BUFFER, _size, _data, GL_STATIC_DRAW);
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}, true);
	}
	
	void GPUTextureBuffer::UpdateData(const void *data, const Range &range)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			
			const uint8 *temp = reinterpret_cast<const uint8 *>(data) + range.origin;
			std::copy(temp, temp + range.length, _data + range.origin);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferData(GL_TEXTURE_BUFFER, _size, _data, GL_STATIC_DRAW);
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		}, true);
	}
	
	void GPUTextureBuffer::Bind(Renderer *renderer, ShaderProgram *program)
	{
		GLuint location = program->GetCustomLocation(_bindPoint);
		if(location != -1)
		{
			uint32 indicesUnit = renderer->BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::Uniform1i(location, indicesUnit);
		}
	}
}
