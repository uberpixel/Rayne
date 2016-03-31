//
//  RNVulkanShaderLibrary.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanShaderLibrary.h"
#include "RNVulkanShader.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanShaderLibrary, ShaderLibrary)

	VulkanShaderLibrary::VulkanShaderLibrary(VulkanRenderer *renderer, const String *file, const ShaderCompileOptions *options) : _shaders(new Dictionary()), _renderer(renderer)
	{
		VkDevice device = renderer->GetVulkanDevice()->GetDevice();

		Data *data = Data::WithContentsOfFile(file);
		Array *mainArray = JSONSerialization::ObjectFromData<Array>(data, 0);

		//Load the shader code
		mainArray->Enumerate<Dictionary>([&](Dictionary *libraryDictionary, size_t index, bool &stop) {

			String *file = libraryDictionary->GetObjectForKey<String>(RNCSTR("file~vulkan"));
			if(!file)
				file = libraryDictionary->GetObjectForKey<String>(RNCSTR("file"));
			if(!file)
				return;

			Data *content = Data::WithContentsOfFile(file);

			Array *shaderArray = libraryDictionary->GetObjectForKey<Array>(RNCSTR("shaders"));

			//Create the shader module
			const char *shaderCode = static_cast<const char*>(content->GetBytes());
			size_t size = content->GetLength();

			VkShaderModuleCreateInfo moduleCreateInfo;

			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.pNext = NULL;

			moduleCreateInfo.codeSize = size;
			moduleCreateInfo.pCode = (uint32_t*)shaderCode;
			moduleCreateInfo.flags = 0;

			RNVulkanValidate(vk::CreateShaderModule(device, &moduleCreateInfo, NULL, &_shaderModule));

			//Build all the shader programs
			shaderArray->Enumerate<Dictionary>([&](Dictionary *shaderDictionary, size_t index, bool &stop) {
				String *name = shaderDictionary->GetObjectForKey<String>(RNCSTR("name"));
				String *typeName = shaderDictionary->GetObjectForKey<String>(RNCSTR("type"));

				Shader::Type type;
				if(typeName->IsEqual(RNCSTR("vertex")))
				{
					type = Shader::Type::Vertex;
				}
				else if(typeName->IsEqual(RNCSTR("fragment")))
				{
					type = Shader::Type::Fragment;
				}
				else if(typeName->IsEqual(RNCSTR("compute")))
				{
					type = Shader::Type::Compute;
				}

				VulkanShader *shader = new VulkanShader(name, type, _shaderModule, this);
				_shaders->SetObjectForKey(shader, name);
			});
		});
	}
	VulkanShaderLibrary::~VulkanShaderLibrary()
	{
		_shaders->Release();
	}

	Shader *VulkanShaderLibrary::GetShaderWithName(const String *name)
	{
		return _shaders->GetObjectForKey<Shader>(name);
	}
	Array *VulkanShaderLibrary::GetShaderNames() const
	{
		return _shaders->GetAllKeys();
	}
}
