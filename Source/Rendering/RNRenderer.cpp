//
//  RNRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer.h"
#include "../Base/RNSettings.h"

namespace RN
{
	RNDefineMeta(Renderer, Object)

	RNExceptionImp(ShaderCompilation)

	static Renderer *_activeRenderer = nullptr;

	Renderer::Renderer(RendererDescriptor *descriptor, RenderingDevice *device) :
		_device(device),
		_descriptor(descriptor)
	{
		RN_ASSERT(descriptor, "Descriptor mustn't be NULL");
		RN_ASSERT(device, "Device mustn't be NULL");
	}

	Renderer::~Renderer()
	{}

	bool Renderer::IsHeadless()
	{
		return !_activeRenderer;
	}

	Renderer *Renderer::GetActiveRenderer()
	{
		RN_ASSERT(_activeRenderer, "GetActiveRenderer() called, but no renderer is currently active");
		return _activeRenderer;
	}

	void Renderer::Activate()
	{
		RN_ASSERT(!_activeRenderer, "Rayne only supports one active renderer at a time");
		_activeRenderer = this;
	}

	void Renderer::Deactivate()
	{
		_activeRenderer = nullptr;
	}

	Shader *Renderer::GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint hint)
	{
		Shader::Options *realOptions = options? options->Copy() : Shader::Options::WithNone()->Retain();
		
		if(hint == Shader::UsageHint::Multiview || hint == Shader::UsageHint::DepthMultiview)
		{
			realOptions->EnableMultiview();
		}
		
		ShaderLibrary *shaderLibrary = GetDefaultShaderLibrary();
		Shader *shader = nullptr;
		if(type == Shader::Type::Vertex)
		{
			if(hint == Shader::UsageHint::Depth)
			{
				shader = shaderLibrary->GetShaderWithName(RNCSTR("depth_vertex"), realOptions);
			}
			else
			{
				if(realOptions && realOptions->HasValue("RN_SKY", "1"))	//Use a different shader for the sky
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("sky_vertex"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_PARTICLES", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("particles_vertex"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_UI", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("ui_vertex"), realOptions);
				}
				else
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("gouraud_vertex"), realOptions);
				}
			}
		}
		else if(type == Shader::Type::Fragment)
		{
			if(hint == Shader::UsageHint::Depth)
			{
				shader = shaderLibrary->GetShaderWithName(RNCSTR("depth_fragment"), realOptions);
			}
			else
			{
				if(realOptions && realOptions->HasValue("RN_SKY", "1"))	//Use a different shader for the sky
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("sky_fragment"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_PARTICLES", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("particles_fragment"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_UI", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("ui_fragment"), realOptions);
				}
				else
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("gouraud_fragment"), realOptions);
				}
			}
		}
		
		realOptions->Release();

		return shader;
	}
}
