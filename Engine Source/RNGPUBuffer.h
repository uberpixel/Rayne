//
//  RNGPUBuffer.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_GPUBUFFER_H__
#define __RAYNE_GPUBUFFER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNOpenGL.h"
#include "RNShader.h"

namespace RN
{
	class Renderer;
	class GPUBuffer : public Object
	{
	public:
		GPUBuffer();
		
		void SetBindPoint(const std::string &bindPoint);
		
		virtual void Bind(Renderer *renderer, ShaderProgram *program) = 0;
		
	protected:
		std::string _bindPoint;
		
		RNDeclareMeta(GPUBuffer)
	};
	
	class GPUScalarBuffer : public GPUBuffer
	{
	public:
		
		void Bind(Renderer *renderer, ShaderProgram *program) override;
		
	private:
		RNDeclareMeta(GPUScalarBuffer)
	};
	
	class GPUTextureBuffer : public GPUBuffer
	{
	public:
		GPUTextureBuffer();
		~GPUTextureBuffer() override;
		
		void SetSize(size_t size);
		void SetData(const void *data);
		void UpdateData(const void *data, const Range &range);
		
		void Bind(Renderer *renderer, ShaderProgram *program) override;
		
	private:
		GLuint _buffer;
		GLuint _texture;
		
		size_t _size;
		uint8 *_data;
		
		RNDeclareMeta(GPUTextureBuffer)
	};
}

#endif /* __RAYNE_GPUBUFFER_H__ */
