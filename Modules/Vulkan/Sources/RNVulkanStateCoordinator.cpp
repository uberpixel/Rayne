//
//  RNVulkanStateCoordinator.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanStateCoordinator.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanShader.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanWindow.h"
#include "RNVulkanDynamicGPUBuffer.h"
#include "RNVulkanInternals.h"

namespace RN
{
	VkFormat _vertexFormatLookup[] =
		{
			VK_FORMAT_UNDEFINED,

			VK_FORMAT_R8G8_UINT,
			VK_FORMAT_R16G16_UINT,
			VK_FORMAT_R32_UINT,

			VK_FORMAT_R8G8_SINT,
			VK_FORMAT_R16G16_SINT,
			VK_FORMAT_R32_SINT,

			VK_FORMAT_R16_SFLOAT,
			VK_FORMAT_R16G16_SFLOAT,
			VK_FORMAT_R16G16B16_SFLOAT,
			VK_FORMAT_R16G16B16A16_SFLOAT,

            VK_FORMAT_R32_SFLOAT,
			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT,

			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT,

			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT
		};

	VkBlendFactor _blendFactorLookup[] =
	{
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_SRC_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_FACTOR_DST_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		VK_BLEND_FACTOR_DST_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
		VK_BLEND_FACTOR_CONSTANT_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		VK_BLEND_FACTOR_CONSTANT_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA
	};

	VulkanUniformState::VulkanUniformState() : instanceAttributesBuffer(nullptr), instanceAttributesArgumentBuffer(nullptr)
	{

	}
	VulkanUniformState::~VulkanUniformState()
	{
		for(VulkanDynamicBufferReference *buffer : vertexConstantBuffers)
		{
			buffer->Release();
		}
		for(VulkanDynamicBufferReference *buffer : fragmentConstantBuffers)
		{
			buffer->Release();
		}
		for(VulkanDynamicBufferReference *buffer : computeConstantBuffers)
		{
			buffer->Release();
		}

		SafeRelease(instanceAttributesBuffer);
		SafeRelease(instanceAttributesArgumentBuffer);

		for(Shader::ArgumentBuffer *buffer : constantBufferToArgumentMapping)
			buffer->Release();
	}


	VulkanRootSignature::~VulkanRootSignature()
	{
		//signature->Release();
	}

	VulkanPipelineState::~VulkanPipelineState()
	{
		//state->Release();
	}

	VulkanComputePipelineState::~VulkanComputePipelineState()
	{
		//state->Release();
	}

	VulkanStateCoordinator::VulkanStateCoordinator() :
		_lastDepthStencilState(nullptr), _pipelineCache(VK_NULL_HANDLE), _pipelineCacheNeedsSaving(false)
	{

	}

	VulkanStateCoordinator::~VulkanStateCoordinator()
	{
		//TODO: Clean up correctly...
		for(VulkanPipelineStateCollection *collection : _renderingStates)
			delete collection;
		for(VulkanComputePipelineState *state : _computeStates)
			delete state;
	}

	void VulkanStateCoordinator::LoadPipelineCache(uint64 buildNumber, VulkanDevice *device, VkAllocationCallbacks *allocatorCallbacks)
	{
		if(_pipelineCache != VK_NULL_HANDLE) return;

		RN::String *cachePath = FileManager::GetSharedInstance()->GetPathForLocation(RN::FileManager::Location::InternalSaveDirectory);
		cachePath = cachePath->StringByAppendingString(RNCSTR("/cache/"));

		size_t dataSize = 0;
		uint8 *data = nullptr;
		try
		{
			File *cacheFile = File::WithName(RNSTR(cachePath << "shaders.blob"), File::Mode::Read);
			dataSize = cacheFile->GetSize();

			if(dataSize > sizeof(VulkanPipelineCachePrefixHeader))
			{
				VulkanPipelineCachePrefixHeader prefixHeader = {};
				cacheFile->Read(&prefixHeader, sizeof(VulkanPipelineCachePrefixHeader));

				bool isValid = true;
				uint8 uuid[VK_UUID_SIZE];
				device->GetPipelineCacheUUID(uuid);
				if(prefixHeader.magic != 8372610) isValid = false;
				if(prefixHeader.dataSize != dataSize - sizeof(VulkanPipelineCachePrefixHeader)) isValid = false;
				if(prefixHeader.buildNumber != buildNumber) isValid = false;
				if(prefixHeader.vendorID != device->GetVendorID()) isValid = false;
				if(prefixHeader.deviceID != device->GetDeviceID()) isValid = false;
				if(prefixHeader.driverVersion != device->GetDriverVersion()) isValid = false;
				if(prefixHeader.driverABI != sizeof(void*)) isValid = false;
				for(int i = 0; i < VK_UUID_SIZE; i++)
				{
					if(prefixHeader.uuid[i] != uuid[i]) isValid = false;
				}

				if(isValid)
				{
					dataSize = prefixHeader.dataSize;
					data = new uint8[dataSize];
					cacheFile->Read(data, dataSize);

					uint64 simpleHash = 0;
					for(int i = 0; i < dataSize; i++)
					{
						simpleHash += data[i];
					}
					if(prefixHeader.dataHash != simpleHash) isValid = false;
					else RNInfo("Loading pipeline cache with size: " << dataSize);
				}

				if(!isValid)
				{
					dataSize = 0;
					RNInfo("Creating new pipeline cache");
				}
			}
		}
		catch(RN::FileNotFoundException exception)
		{

		}

		VkPipelineCacheCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.initialDataSize = dataSize;
		createInfo.pInitialData = data;

		RNVulkanValidate(vk::CreatePipelineCache(device->GetDevice(), &createInfo, allocatorCallbacks, &_pipelineCache));

		delete[] data;
	}

	void VulkanStateCoordinator::SavePipelineCache(uint64 buildNumber, VulkanDevice *device)
	{
		if(_pipelineCache == VK_NULL_HANDLE || !_pipelineCacheNeedsSaving) return;

		size_t dataSize = 0;
		RNVulkanValidate(vk::GetPipelineCacheData(device->GetDevice(), _pipelineCache, &dataSize, nullptr));

		uint8 *data = new uint8[dataSize];
		RNVulkanValidate(vk::GetPipelineCacheData(device->GetDevice(), _pipelineCache, &dataSize, data));

		uint64 simpleHash = 0;
		for(int i = 0; i < dataSize; i++)
		{
			simpleHash += data[i];
		}

		VulkanPipelineCachePrefixHeader prefixHeader = {};
		prefixHeader.magic = 8372610;
		prefixHeader.dataSize = dataSize;
		prefixHeader.dataHash = simpleHash;
		prefixHeader.buildNumber = buildNumber;
		prefixHeader.vendorID = device->GetVendorID();
		prefixHeader.deviceID = device->GetDeviceID();
		prefixHeader.driverVersion = device->GetDriverVersion();
		prefixHeader.driverABI = sizeof(void*);
		device->GetPipelineCacheUUID(prefixHeader.uuid);

		RN::String *cachePath = FileManager::GetSharedInstance()->GetPathForLocation(RN::FileManager::Location::InternalSaveDirectory);
		cachePath = cachePath->StringByAppendingString(RNCSTR("/cache/"));

		if(!FileManager::GetSharedInstance()->PathExists(cachePath))
		{
			FileManager::GetSharedInstance()->CreateDirectory(cachePath);
		}

		File *cacheFile = File::WithName(RNSTR(cachePath << "shaders.new.blob"), File::Mode::Write);
		cacheFile->Write(&prefixHeader, sizeof(VulkanPipelineCachePrefixHeader));
		cacheFile->Write(data, dataSize);

		delete[] data;

		FileManager::GetSharedInstance()->RenameFile(RNSTR(cachePath << "shaders.new.blob"), RNSTR(cachePath << "shaders.blob"));
		_pipelineCacheNeedsSaving = false;
	}

	void VulkanStateCoordinator::DestroyPipelineCache(VulkanDevice *device, VkAllocationCallbacks *allocatorCallbacks)
	{
		if(_pipelineCache == VK_NULL_HANDLE) return;

		vk::DestroyPipelineCache(device->GetDevice(), _pipelineCache, allocatorCallbacks);
		_pipelineCache = VK_NULL_HANDLE;
	}

	const VulkanRootSignature *VulkanStateCoordinator::GetRootSignature(const VulkanPipelineStateDescriptor &descriptor)
	{
		Array *samplerArray = new Array();
		samplerArray->Autorelease();
		std::vector<uint16> bindingIndex;
		std::vector<VulkanRootSignatureBindingType> bindingType;

		VulkanShader *vertexShader = static_cast<VulkanShader *>(descriptor.vertexShader);
		VulkanShader *fragmentShader = static_cast<VulkanShader *>(descriptor.fragmentShader);
		VulkanShader *computeShader = static_cast<VulkanShader *>(descriptor.computeShader);

		const Shader::Signature *vertexShaderSignature = vertexShader? vertexShader->GetSignature() : nullptr;
		const Shader::Signature *fragmentShaderSignature = fragmentShader? fragmentShader->GetSignature() : nullptr;
		const Shader::Signature *computeShaderSignature = computeShader? computeShader->GetSignature() : nullptr;

		uint8 textureCount = 0;
		uint8 constantBufferCount = 0;
		uint8 subpassInputCount = 0;

		if(vertexShaderSignature)
		{
			vertexShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer ? VulkanRootSignatureBindingType::VertexUniformBuffer : VulkanRootSignatureBindingType::VertexStorageBuffer);
			});

			vertexShaderSignature->GetSubpassInputs()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(VulkanRootSignatureBindingType::VertexSubpassInput);
			});

			vertexShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(VulkanRootSignatureBindingType::VertexSampler);
			});

			vertexShaderSignature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VulkanRootSignatureBindingType::VertexStorageImage : VulkanRootSignatureBindingType::VertexSampledImage);
			});

			textureCount += vertexShaderSignature->GetTextures()->GetCount();
			constantBufferCount += vertexShaderSignature->GetBuffers()->GetCount();
			subpassInputCount += vertexShaderSignature->GetSubpassInputs()->GetCount();

			samplerArray->AddObjectsFromArray(vertexShaderSignature->GetSamplers());
		}
		if(fragmentShaderSignature)
		{
			fragmentShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer ? VulkanRootSignatureBindingType::FragmentUniformBuffer : VulkanRootSignatureBindingType::FragmentStorageBuffer);
			});

			fragmentShaderSignature->GetSubpassInputs()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(VulkanRootSignatureBindingType::FragmentSubpassInput);
			});

			fragmentShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(VulkanRootSignatureBindingType::FragmentSampler);
			});

			fragmentShaderSignature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VulkanRootSignatureBindingType::FragmentStorageImage : VulkanRootSignatureBindingType::FragmentSampledImage);
			});

			textureCount += fragmentShaderSignature->GetTextures()->GetCount();
			constantBufferCount += fragmentShaderSignature->GetBuffers()->GetCount();
			subpassInputCount += fragmentShaderSignature->GetSubpassInputs()->GetCount();

			samplerArray->AddObjectsFromArray(fragmentShaderSignature->GetSamplers());
		}
		if(computeShaderSignature)
		{
			computeShaderSignature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer ? VulkanRootSignatureBindingType::ComputeUniformBuffer : VulkanRootSignatureBindingType::ComputeStorageBuffer);
			});

			computeShaderSignature->GetSamplers()->Enumerate<Shader::Argument>([&](Shader::Argument *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(VulkanRootSignatureBindingType::ComputeSampler);
			});

			computeShaderSignature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop){
				bindingIndex.push_back(argument->GetIndex());
				bindingType.push_back(argument->GetType() == Shader::ArgumentTexture::Type::Storage ? VulkanRootSignatureBindingType::ComputeStorageImage : VulkanRootSignatureBindingType::ComputeSampledImage);
			});

			textureCount += computeShaderSignature->GetTextures()->GetCount();
			constantBufferCount += computeShaderSignature->GetBuffers()->GetCount();

			samplerArray->AddObjectsFromArray(computeShaderSignature->GetSamplers());
		}

		for(VulkanRootSignature *signature : _rootSignatures)
		{
			if(signature->bindingIndex != bindingIndex) continue;
			if(signature->bindingType != bindingType) continue;

			bool notEqual = false;
			signature->samplers->Enumerate<Shader::ArgumentSampler>([&](Shader::ArgumentSampler *sampler, size_t index, bool &stop) {
				if(!(sampler == samplerArray->GetObjectAtIndex(index)))
				{
					notEqual = true;
					stop = true;
				}
			});
			if(notEqual)
			{
				continue;
			}

			return signature;
		}

		VulkanRenderer *renderer = static_cast<VulkanRenderer *>(Renderer::GetActiveRenderer());
		VulkanDevice *device = renderer->GetVulkanDevice();

		VulkanRootSignature *signature = new VulkanRootSignature();
		signature->bindingIndex = bindingIndex;
		signature->bindingType = bindingType;
		signature->samplers = samplerArray->Retain();
		signature->textureCount = textureCount;
		signature->subpassInputCount = subpassInputCount;
		signature->constantBufferCount = constantBufferCount;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		auto addSetLayoutBinding = [&](const VkDescriptorSetLayoutBinding &layoutBinding) {
			for(VkDescriptorSetLayoutBinding &existingLayoutBinding : setLayoutBindings)
			{
				if(existingLayoutBinding.binding != layoutBinding.binding)
					continue;

				RN_ASSERT(existingLayoutBinding.descriptorType == layoutBinding.descriptorType, "Descriptor binding %u is used with incompatible descriptor types", layoutBinding.binding);
				RN_ASSERT(existingLayoutBinding.descriptorCount == layoutBinding.descriptorCount, "Descriptor binding %u is used with incompatible descriptor counts", layoutBinding.binding);

				existingLayoutBinding.stageFlags |= layoutBinding.stageFlags;
				if(!existingLayoutBinding.pImmutableSamplers)
					existingLayoutBinding.pImmutableSamplers = layoutBinding.pImmutableSamplers;
				return;
			}

			setLayoutBindings.push_back(layoutBinding);
		};

		//Vertex buffer!?
		/*VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
		setUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		setUniformLayoutBinding.binding = setLayoutBindings.size();
		setUniformLayoutBinding.descriptorCount = 0;
		setLayoutBindings.push_back(setUniformLayoutBinding);*/

		if(vertexShader)
		{
			const Shader::Signature *signature = vertexShader->GetSignature();

			//Vertex shader constant buffers
			signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
				setUniformLayoutBinding.descriptorType = (buffer->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				setUniformLayoutBinding.binding = buffer->GetIndex();
				setUniformLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setUniformLayoutBinding);
			});

			//Vertex shader textures
			signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
				setImageLayoutBinding.descriptorType = texture->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				setImageLayoutBinding.binding = texture->GetIndex();
				setImageLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setImageLayoutBinding);
			});

			//Vertex shader subpass inputs
			signature->GetSubpassInputs()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setInputAttachmentLayoutBinding = {};
				setInputAttachmentLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				setInputAttachmentLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				setInputAttachmentLayoutBinding.binding = texture->GetIndex();
				setInputAttachmentLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setInputAttachmentLayoutBinding);
			});
		}

		if(fragmentShader)
		{
			const Shader::Signature *signature = fragmentShader->GetSignature();

			//Vertex shader constant buffers
			signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
				setUniformLayoutBinding.descriptorType = (buffer->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setUniformLayoutBinding.binding = buffer->GetIndex();
				setUniformLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setUniformLayoutBinding);
			});

			//Fragment shader textures
			signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
				setImageLayoutBinding.descriptorType = texture->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setImageLayoutBinding.binding = texture->GetIndex();
				setImageLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setImageLayoutBinding);
			});

			//Fragment shader subpass inputs
			signature->GetSubpassInputs()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setInputAttachmentLayoutBinding = {};
				setInputAttachmentLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				setInputAttachmentLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setInputAttachmentLayoutBinding.binding = texture->GetIndex();
				setInputAttachmentLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setInputAttachmentLayoutBinding);
			});
		}

		if(computeShader)
		{
			const Shader::Signature *signature = computeShader->GetSignature();

			signature->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setUniformLayoutBinding = {};
				setUniformLayoutBinding.descriptorType = (buffer->GetType() == Shader::ArgumentBuffer::Type::UniformBuffer)? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				setUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				setUniformLayoutBinding.binding = buffer->GetIndex();
				setUniformLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setUniformLayoutBinding);
			});

			signature->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *texture, size_t index, bool &stop){
				VkDescriptorSetLayoutBinding setImageLayoutBinding = {};
				setImageLayoutBinding.descriptorType = texture->GetType() == Shader::ArgumentTexture::Type::Storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				setImageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				setImageLayoutBinding.binding = texture->GetIndex();
				setImageLayoutBinding.descriptorCount = 1;
				addSetLayoutBinding(setImageLayoutBinding);
			});

			RN_ASSERT(signature->GetSubpassInputs()->GetCount() == 0, "Compute shaders cannot use subpass inputs");
		}

		// Create samplers
		std::vector<VkSampler> samplers;
		std::vector<Shader::ArgumentSampler *> processedSamplers;
		samplers.reserve(signature->samplers->GetCount());
		processedSamplers.reserve(signature->samplers->GetCount());
		if(signature->samplers->GetCount() > 0)
		{
			signature->samplers->Enumerate<Shader::ArgumentSampler>([&](Shader::ArgumentSampler *sampler, size_t index, bool &stop) {
				for(Shader::ArgumentSampler *processedSampler : processedSamplers)
				{
					if(processedSampler->GetIndex() == sampler->GetIndex())
					{
						RN_ASSERT(*processedSampler == *sampler, "Sampler binding %u is used with incompatible sampler states", sampler->GetIndex());
						return;
					}
				}
				processedSamplers.push_back(sampler);

				VkFilter filter = VK_FILTER_LINEAR;
				switch(sampler->GetFilter())
				{
					case Shader::ArgumentSampler::Filter::Anisotropic:
						filter = VK_FILTER_LINEAR;
						break;
					case Shader::ArgumentSampler::Filter::Linear:
						filter = VK_FILTER_LINEAR;
						break;
					case Shader::ArgumentSampler::Filter::Nearest:
						filter = VK_FILTER_NEAREST;
						break;
				}

				VkCompareOp comparisonFunction = VK_COMPARE_OP_NEVER;
				if(sampler->GetComparisonFunction() != Shader::ArgumentSampler::ComparisonFunction::Never)
				{
					switch(sampler->GetComparisonFunction())
					{
						case Shader::ArgumentSampler::ComparisonFunction::Always:
							comparisonFunction = VK_COMPARE_OP_ALWAYS;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Equal:
							comparisonFunction = VK_COMPARE_OP_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::GreaterEqual:
							comparisonFunction = VK_COMPARE_OP_GREATER_OR_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Greater:
							comparisonFunction = VK_COMPARE_OP_GREATER;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Less:
							comparisonFunction = VK_COMPARE_OP_LESS;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::LessEqual:
							comparisonFunction = VK_COMPARE_OP_LESS_OR_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::NotEqual:
							comparisonFunction = VK_COMPARE_OP_NOT_EQUAL;
							break;
						case Shader::ArgumentSampler::ComparisonFunction::Never:
							comparisonFunction = VK_COMPARE_OP_NEVER;
							break;
					}
				}

				VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				switch(sampler->GetWrapMode())
				{
					case Shader::ArgumentSampler::WrapMode::Repeat:
						addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						break;
					case Shader::ArgumentSampler::WrapMode::Clamp:
						addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
						break;
				}

				// Create sampler
				VkSamplerCreateInfo samplerInfo = {};
				samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerInfo.magFilter = filter;
				samplerInfo.minFilter = filter;
				samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerInfo.addressModeU = addressMode;
				samplerInfo.addressModeV = addressMode;
				samplerInfo.addressModeW = addressMode;
				samplerInfo.mipLodBias = 0.0f;
				samplerInfo.compareEnable = comparisonFunction == VK_COMPARE_OP_NEVER ? VK_FALSE : VK_TRUE;
				samplerInfo.compareOp = comparisonFunction;
				samplerInfo.minLod = 0.0f;
				samplerInfo.maxLod = std::numeric_limits<float>::max();
				const bool wantsAnisotropy = (sampler->GetFilter() == Shader::ArgumentSampler::Filter::Anisotropic);
				const bool supportsAnisotropy = device->GetSupportsSamplerAnisotropy();
				const float clampedAnisotropy = supportsAnisotropy? std::min(static_cast<float>(sampler->GetAnisotropy()), device->GetMaxSamplerAnisotropy()) : 1.0f;
				samplerInfo.maxAnisotropy = clampedAnisotropy;
				samplerInfo.anisotropyEnable = (wantsAnisotropy && supportsAnisotropy)? VK_TRUE:VK_FALSE;
				samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				samplerInfo.unnormalizedCoordinates = VK_FALSE;

				VkSampler vkSampler;
				RNVulkanValidate(vk::CreateSampler(device->GetDevice(), &samplerInfo, renderer->GetAllocatorCallback(), &vkSampler));
				//TODO: Store and release samplers with the pipeline? Maybe can immediately be released after creating pipeline?

				samplers.push_back(vkSampler);

				VkDescriptorSetLayoutBinding staticSamplerBinding = {};
				staticSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				staticSamplerBinding.stageFlags = VK_SHADER_STAGE_ALL;
				staticSamplerBinding.binding = sampler->GetIndex();
				staticSamplerBinding.descriptorCount = 1;
				staticSamplerBinding.pImmutableSamplers = &samplers[samplers.size()-1];

				addSetLayoutBinding(staticSamplerBinding);
			});
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
		descriptorSetLayoutCreateInfo.bindingCount = setLayoutBindings.size();

		RNVulkanValidate(vk::CreateDescriptorSetLayout(device->GetDevice(), &descriptorSetLayoutCreateInfo, renderer->GetAllocatorCallback(), &signature->descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &signature->descriptorSetLayout;

		RNVulkanValidate(vk::CreatePipelineLayout(device->GetDevice(), &pipelineLayoutCreateInfo, renderer->GetAllocatorCallback(), &signature->pipelineLayout));

		_rootSignatures.push_back(signature);
		return signature;
	}

	const VulkanPipelineState *VulkanStateCoordinator::GetRenderPipelineState(Shader *vertexShader, Shader *fragmentShader, const Mesh::DrawSnapshot &mesh, const Material::PipelineProperties &mergedMaterialProperties, const RenderFrame &renderFrame, const VulkanRenderPass *rootVulkanPass, uint32 subpassIndex, uint8 multiviewCount)
	{
		const VulkanFramebuffer *framebuffer = rootVulkanPass->framebuffer;

		const Mesh::VertexDescriptor &descriptor = mesh.GetVertexDescriptor();
		VulkanPipelineStateDescriptor pipelineDescriptor;
		pipelineDescriptor.depthStencilFormat = (framebuffer->_depthStencilTarget) ? framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format : VK_FORMAT_UNDEFINED;
		pipelineDescriptor.sampleCount = framebuffer->GetSampleCount();
		//pipelineDescriptor.sampleQuality = 0;//(framebuffer->_colorTargets.size() > 0 && !framebuffer->GetSwapChain()) ? framebuffer->_colorTargets[0]->targetView texture->GetDescriptor().sampleQuality : 0;
		pipelineDescriptor.renderPass = GetRenderPassState(renderFrame, rootVulkanPass, multiviewCount)->renderPass;
		pipelineDescriptor.vertexShader = vertexShader;
		pipelineDescriptor.fragmentShader = fragmentShader;
		pipelineDescriptor.computeShader = nullptr;
		pipelineDescriptor.depthWriteEnabled = mergedMaterialProperties.depthWriteEnabled;
		pipelineDescriptor.colorWriteMask = mergedMaterialProperties.colorWriteMask;
		pipelineDescriptor.depthMode = mergedMaterialProperties.depthMode;
		pipelineDescriptor.cullMode = mergedMaterialProperties.cullMode;
		pipelineDescriptor.usePolygonOffset = mergedMaterialProperties.usePolygonOffset;
		pipelineDescriptor.polygonOffsetFactor = mergedMaterialProperties.polygonOffsetFactor;
		pipelineDescriptor.polygonOffsetUnits = mergedMaterialProperties.polygonOffsetUnits;
		pipelineDescriptor.useAlphaToCoverage = mergedMaterialProperties.useAlphaToCoverage;
		pipelineDescriptor.blendOperationRGB = mergedMaterialProperties.blendOperationRGB;
        pipelineDescriptor.blendOperationAlpha = mergedMaterialProperties.blendOperationAlpha;
        pipelineDescriptor.blendFactorSourceRGB = mergedMaterialProperties.blendFactorSourceRGB;
        pipelineDescriptor.blendFactorDestinationRGB = mergedMaterialProperties.blendFactorDestinationRGB;
        pipelineDescriptor.blendFactorSourceAlpha = mergedMaterialProperties.blendFactorSourceAlpha;
        pipelineDescriptor.blendFactorDestinationAlpha = mergedMaterialProperties.blendFactorDestinationAlpha;

		pipelineDescriptor.subpassIndex = subpassIndex;
		
		// Determine color attachment count for this subpass to match pipeline color blend state
		{
			uint32 totalColorAttachments = framebuffer->GetColorTargetCount();
			uint32 countForPipeline = totalColorAttachments;
			if(rootVulkanPass && rootVulkanPass->subpasses.size() > 0 && subpassIndex < rootVulkanPass->subpasses.size())
			{
				const VulkanRenderPass &subpass = rootVulkanPass->subpasses[subpassIndex];
				countForPipeline = renderFrame.GetPass(subpass.renderFramePassIndex).GetDrawSnapshot().GetSubpass().GetWritesColorAttachmentCount();
			}
			pipelineDescriptor.colorAttachmentCount = static_cast<uint8>(countForPipeline);
		}

		for(VulkanPipelineStateCollection *collection : _renderingStates)
		{
			if(collection->descriptor.IsEqual(descriptor))
			{
				if(collection->vertexShader == pipelineDescriptor.vertexShader && collection->fragmentShader == pipelineDescriptor.fragmentShader)
				{
					return GetRenderPipelineStateInCollection(collection, mesh, pipelineDescriptor);
				}
			}
		}

		VulkanPipelineStateCollection *collection = new VulkanPipelineStateCollection(descriptor, pipelineDescriptor.vertexShader, pipelineDescriptor.fragmentShader);
		_renderingStates.push_back(collection);

		return GetRenderPipelineStateInCollection(collection, mesh, pipelineDescriptor);
	}

	const VulkanComputePipelineState *VulkanStateCoordinator::GetComputePipelineState(Shader *computeShader)
	{
		RN_ASSERT(computeShader && computeShader->GetType() == Shader::Type::Compute, "Compute pipeline state requires a compute shader");

		for(const VulkanComputePipelineState *state : _computeStates)
		{
			if(state->computeShader == computeShader)
				return state;
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();

		VulkanPipelineStateDescriptor descriptor = {};
		descriptor.computeShader = computeShader;
		const VulkanRootSignature *rootSignature = GetRootSignature(descriptor);

		VulkanShader *vulkanShader = computeShader->Downcast<VulkanShader>();
		VkComputePipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = NULL;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.stage = vulkanShader->_shaderStage;
		pipelineCreateInfo.layout = rootSignature->pipelineLayout;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		VkPipeline pipeline;
		VkResult result = vk::CreateComputePipelines(device->GetDevice(), _pipelineCache, 1, &pipelineCreateInfo, renderer->GetAllocatorCallback(), &pipeline);
		if(result != VK_SUCCESS)
			RNErrorf("Vulkan compute pipeline create failed for shader '%s' with result %i", computeShader->GetName()->GetUTF8String(), result);
		RNVulkanValidate(result);
		_pipelineCacheNeedsSaving = true;

		VulkanComputePipelineState *state = new VulkanComputePipelineState();
		state->computeShader = computeShader;
		state->rootSignature = rootSignature;
		state->state = pipeline;
		_computeStates.push_back(state);
		return state;
	}

	const VulkanPipelineState *VulkanStateCoordinator::GetRenderPipelineStateInCollection(VulkanPipelineStateCollection *collection, const Mesh::DrawSnapshot &mesh, const VulkanPipelineStateDescriptor &descriptor)
	{
		const VulkanRootSignature *rootSignature = GetRootSignature(descriptor);

		//TODO: Make sure all possible cases are covered...
		//TODO: Maybe solve nicer...
		for(const VulkanPipelineState *state : collection->states)
		{
			if(state->descriptor.renderPass == descriptor.renderPass && state->descriptor.depthStencilFormat == descriptor.depthStencilFormat && rootSignature->pipelineLayout == state->rootSignature->pipelineLayout)
			{
				if(state->descriptor.sampleCount == descriptor.sampleCount && state->descriptor.colorWriteMask == descriptor.colorWriteMask && state->descriptor.depthWriteEnabled == descriptor.depthWriteEnabled && state->descriptor.depthMode == descriptor.depthMode && state->descriptor.cullMode == descriptor.cullMode && state->descriptor.usePolygonOffset == descriptor.usePolygonOffset && state->descriptor.polygonOffsetFactor == descriptor.polygonOffsetFactor && state->descriptor.polygonOffsetUnits == descriptor.polygonOffsetUnits && state->descriptor.useAlphaToCoverage == descriptor.useAlphaToCoverage && state->descriptor.blendOperationRGB == descriptor.blendOperationRGB && state->descriptor.blendOperationAlpha == descriptor.blendOperationAlpha && state->descriptor.blendFactorSourceRGB == descriptor.blendFactorSourceRGB && state->descriptor.blendFactorSourceAlpha == descriptor.blendFactorSourceAlpha && state->descriptor.blendFactorDestinationRGB == descriptor.blendFactorDestinationRGB && state->descriptor.blendFactorDestinationAlpha == descriptor.blendFactorDestinationAlpha && state->descriptor.subpassIndex == descriptor.subpassIndex && state->descriptor.colorAttachmentCount == descriptor.colorAttachmentCount)
				{
					return state;
				}
			}
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();

		//Collect shaders
		VulkanShader *vertexShaderRayne = collection->vertexShader->Downcast<VulkanShader>();
		VulkanShader *fragmentShaderRayne = collection->fragmentShader->Downcast<VulkanShader>();
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = vertexShaderRayne->_shaderStage;
		shaderStages[1] = fragmentShaderRayne->_shaderStage;

		//Handle vertex attributes
        bool vertexPositionsOnly = false;
		bool hasInstancing = vertexShaderRayne->GetHasInstancing() && vertexShaderRayne->_instancingAttributes != nullptr;
        const std::vector<VkVertexInputAttributeDescription> &attributeDescriptions = CreateVertexElementDescriptors(mesh, vertexShaderRayne, vertexPositionsOnly);
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
		if(mesh.GetVertexPositionsSeparatedSize() > 0)
		{
			//Positions buffer
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = vertexBindingDescriptions.size();
			bindingDescription.stride = mesh.GetVertexPositionsSeparatedStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindingDescriptions.push_back(bindingDescription);
		}
        if(!vertexPositionsOnly) //vertexPositionsOnly will be true only if separated positions are used and no other vertex attributes exist
		{
			//Interleaved buffer
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = vertexBindingDescriptions.size();
			bindingDescription.stride = mesh.GetStride();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexBindingDescriptions.push_back(bindingDescription);
		}
		if(hasInstancing)
		{
			//Per instance data buffer binding
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = vertexBindingDescriptions.size();
			bindingDescription.stride = vertexShaderRayne->_instancingAttributes->GetTotalUniformSize();
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			vertexBindingDescriptions.push_back(bindingDescription);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.pNext = NULL;
		vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		switch(mesh.GetDrawMode())
        {
            case DrawMode::Point:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                break;

            case DrawMode::Line:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                break;

            case DrawMode::LineStrip:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                break;

            case DrawMode::Triangle:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;

            case DrawMode::TriangleStrip:
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                break;
        }
		inputAssemblyState.flags = 0;
		inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.flags = 0;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		if(descriptor.usePolygonOffset)
		{
			rasterizationState.depthBiasEnable = VK_TRUE;
			rasterizationState.depthBiasConstantFactor = descriptor.polygonOffsetUnits*0.5f;
			rasterizationState.depthBiasSlopeFactor = descriptor.polygonOffsetFactor;
		}
		switch(descriptor.cullMode)
		{
			case CullMode::BackFace:
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
				break;
			case CullMode::FrontFace:
				rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
				break;
			case CullMode::None:
				rasterizationState.cullMode = VK_CULL_MODE_NONE;
				break;
		}

		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = descriptor.colorWriteMask;
		blendAttachmentState.blendEnable = VK_FALSE;
		if(descriptor.blendOperationRGB != BlendOperation::None && descriptor.blendOperationAlpha != BlendOperation::None)
		{
			blendAttachmentState.blendEnable = VK_TRUE;
			blendAttachmentState.colorBlendOp = static_cast<VkBlendOp>(descriptor.blendOperationRGB);
			blendAttachmentState.alphaBlendOp = static_cast<VkBlendOp>(descriptor.blendOperationAlpha);
			blendAttachmentState.srcColorBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorSourceRGB)];
			blendAttachmentState.srcAlphaBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorSourceAlpha)];
			blendAttachmentState.dstColorBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorDestinationRGB)];
			blendAttachmentState.dstAlphaBlendFactor = _blendFactorLookup[static_cast<uint32>(descriptor.blendFactorDestinationAlpha)];
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.pNext = NULL;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		if(descriptor.colorAttachmentCount > 0)
		{
			blendAttachmentStates.resize(descriptor.colorAttachmentCount, blendAttachmentState);
			colorBlendState.attachmentCount = descriptor.colorAttachmentCount;
			colorBlendState.pAttachments = blendAttachmentStates.data();
		}
		else
		{
			colorBlendState.attachmentCount = 0;
			colorBlendState.pAttachments = nullptr;
		}

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		if(descriptor.depthStencilFormat != VK_FORMAT_UNDEFINED)
		{
			depthStencilState.depthTestEnable = descriptor.depthMode != DepthMode::Never;
			depthStencilState.depthWriteEnable = descriptor.depthWriteEnabled;

			switch(descriptor.depthMode)
			{
				case DepthMode::Never:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
					break;

				case DepthMode::Always:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
					break;

				case DepthMode::Less:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
					break;

				case DepthMode::LessOrEqual:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					break;

				case DepthMode::Equal:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;
					break;

				case DepthMode::NotEqual:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
					break;

				case DepthMode::GreaterOrEqual:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
					break;

				case DepthMode::Greater:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
					break;

				default:
					depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					break;
			}

			depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilState.front = depthStencilState.back;
		}
		else
		{
			depthStencilState.depthTestEnable = VK_FALSE;
			depthStencilState.depthWriteEnable = VK_FALSE;
		}

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.flags = 0;

		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(descriptor.sampleCount);
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.alphaToCoverageEnable = (descriptor.useAlphaToCoverage && descriptor.colorWriteMask > 0)? VK_TRUE : VK_FALSE;
		//TODO: Maybe set minSampleShading?

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = dynamicStateEnables.size();

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = NULL;
		pipelineCreateInfo.layout = rootSignature->pipelineLayout;
		pipelineCreateInfo.renderPass = descriptor.renderPass;
		pipelineCreateInfo.subpass = descriptor.subpassIndex;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VkPipeline pipeline;
		VkResult result = vk::CreateGraphicsPipelines(device->GetDevice(), _pipelineCache, 1, &pipelineCreateInfo, renderer->GetAllocatorCallback(), &pipeline);
		if(result != VK_SUCCESS)
			RNErrorf("Vulkan graphics pipeline create failed for vertex shader '%s', fragment shader '%s', subpass %u, samples %u, color attachments %u with result %i", vertexShaderRayne->GetName()->GetUTF8String(), fragmentShaderRayne->GetName()->GetUTF8String(), descriptor.subpassIndex, descriptor.sampleCount, descriptor.colorAttachmentCount, result);
		RNVulkanValidate(result);
		_pipelineCacheNeedsSaving = true;

		// Create the rendering state
		VulkanPipelineState *state = new VulkanPipelineState();
		state->descriptor = std::move(descriptor);
		state->rootSignature = rootSignature;
		state->state = pipeline;
        state->vertexAttributeBufferCount = vertexBindingDescriptions.size();

		collection->states.push_back(state);
		return state;



/*
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.NumRenderTargets = descriptor.colorFormats.size();
		int counter = 0;
		for(DXGI_FORMAT format : descriptor.colorFormats)
		{
			psoDesc.RTVFormats[counter++] = format;
			if(counter >= 8)
				break;
		}*/
	}

	std::vector<VkVertexInputAttributeDescription> VulkanStateCoordinator::CreateVertexElementDescriptors(const Mesh::DrawSnapshot &mesh, VulkanShader *vertexShader, bool &vertexPositionsOnly)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        vertexPositionsOnly = true;

		uint8 vertexBinding = 0;
		const std::vector<Mesh::VertexAttribute> &attributes = mesh.GetVertexAttributes();
		for(const Mesh::VertexAttribute &attribute : attributes)
		{
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Indices)
				continue;

            uint32 attributeLocation = vertexShader->_hasInputVertexAttribute[static_cast<uint32>(attribute.GetFeature())];
			if(attributeLocation != -1)
			{
				VkVertexInputAttributeDescription attributeDescription = {};
				attributeDescription.location = attributeLocation;
				attributeDescription.binding = vertexBinding;
				attributeDescription.format = _vertexFormatLookup[static_cast<VkFormat>(attribute.GetType())];
				attributeDescription.offset = attribute.GetOffset();

				attributeDescriptions.push_back(attributeDescription);

				if(attribute.GetFeature() != Mesh::VertexAttribute::Feature::Vertices || mesh.GetVertexPositionsSeparatedSize() == 0)
					vertexPositionsOnly = false;
			}

			// The mesh binds its separated position stream even when the shader
			// does not read positions. Remaining attributes still use binding 1.
			if(attribute.GetFeature() == Mesh::VertexAttribute::Feature::Vertices && mesh.GetVertexPositionsSeparatedSize() > 0)
				vertexBinding += 1;
		}

		if(vertexShader->_instancingAttributes && vertexShader->GetHasInstancing())
		{
			if(!vertexPositionsOnly) vertexBinding += 1;
			vertexShader->_instancingAttributes->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *attribute, size_t index, bool &stop){
                for(int i = 0; i < (attribute->GetType() == PrimitiveType::Matrix4x4? 4 : (attribute->GetType() == PrimitiveType::Matrix3x3? 3 : 1)); i++)
                {
                    VkVertexInputAttributeDescription attributeDescription = {};
                    attributeDescription.location = attribute->GetAttributeLocation() + i;
                    attributeDescription.binding = vertexBinding;
                    attributeDescription.format = _vertexFormatLookup[static_cast<VkFormat>(attribute->GetType())];
                    attributeDescription.offset = attribute->GetOffset() + i * 4 * 4;

                    attributeDescriptions.push_back(attributeDescription);
                }
			});
		}

		return attributeDescriptions;
	}

	VulkanUniformState *VulkanStateCoordinator::GetUniformStateForPipelineState(const VulkanPipelineState *pipelineState)
	{
		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

		VulkanUniformState *state = new VulkanUniformState();
		Shader *vertexShader = pipelineState->descriptor.vertexShader;
		Shader *fragmentShader = pipelineState->descriptor.fragmentShader;
		if(vertexShader && vertexShader->GetSignature())
		{
			VulkanShader *vulkanShader = vertexShader->Downcast<VulkanShader>();
			if(vulkanShader->GetHasInstancing() && vulkanShader->_instancingAttributes)
			{
				state->instanceAttributesArgumentBuffer = vulkanShader->_instancingAttributes->Retain();
				state->instanceAttributesBuffer = renderer->GetConstantBufferReference(vulkanShader->_instancingAttributes->GetTotalUniformSize(), vulkanShader->_instancingAttributes->GetIndex(), GPUResource::UsageOptions::Vertex)->Retain();
			}
			vertexShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				if(buffer->GetSource() != Shader::ArgumentBuffer::Source::Draw)
					return;

				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					state->constantBufferToArgumentMapping.push_back(buffer->Retain());
					state->vertexConstantBuffers.push_back(renderer->GetConstantBufferReference(totalSize, buffer->GetIndex())->Retain());
				}
			});
		}

		if(fragmentShader && fragmentShader->GetSignature())
		{
			fragmentShader->GetSignature()->GetBuffers()->Enumerate<Shader::ArgumentBuffer>([&](Shader::ArgumentBuffer *buffer, size_t index, bool &stop){
				if(buffer->GetSource() != Shader::ArgumentBuffer::Source::Draw)
					return;

				size_t totalSize = buffer->GetTotalUniformSize();
				if(totalSize > 0)
				{
					state->constantBufferToArgumentMapping.push_back(buffer->Retain());
					state->fragmentConstantBuffers.push_back(renderer->GetConstantBufferReference(totalSize, buffer->GetIndex())->Retain());
				}
			});
		}

		return state;
	}

	VulkanRenderPassState *VulkanStateCoordinator::GetRenderPassState(const RenderFrame &renderFrame, const VulkanRenderPass *rootVulkanPass, uint8 multiviewCount)
	{
		const VulkanFramebuffer *framebuffer = rootVulkanPass->framebuffer;
		const VulkanFramebuffer *resolveFramebuffer = rootVulkanPass->resolveFramebuffer;
		const RenderPass::DrawSnapshot &rootDrawSnapshot = renderFrame.GetPass(rootVulkanPass->renderFramePassIndex).GetDrawSnapshot();
		const RenderPass::SubpassSnapshot &rootSubpassSnapshot = rootDrawSnapshot.GetSubpass();
		RenderPass::Flags flags = rootDrawSnapshot.GetFlags();
		//TODO: Maybe handle swapchain case better...
		RN_ASSERT(!resolveFramebuffer || framebuffer->_colorTargets.size() <= resolveFramebuffer->_colorTargets.size(), "Resolve framebuffer needs a target for each target in the framebuffer!");

		VulkanRenderPassState renderPassState;
		renderPassState.flags = flags;
		renderPassState.multiviewCount = multiviewCount;
		const VulkanFramebuffer *fragmentDensityFramebuffer = resolveFramebuffer? resolveFramebuffer : framebuffer;
		renderPassState.hasFragmentDensityMap = fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0;
		for(const VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
		{
			renderPassState.imageFormats.push_back(targetView->vulkanTargetViewDescriptor.format);
		}

		if(resolveFramebuffer)
		{
			const bool resolvesToSwapChain = resolveFramebuffer->_swapChain;
			for(uint32 sourceTargetIndex = 0; sourceTargetIndex < framebuffer->_colorTargets.size(); sourceTargetIndex += 1)
			{
				VkFormat resolveFormat = VK_FORMAT_UNDEFINED;
				uint32 resolveTargetIndex = resolvesToSwapChain? 0 : sourceTargetIndex;
				if((!resolvesToSwapChain || sourceTargetIndex == 0) && resolveTargetIndex < resolveFramebuffer->_colorTargets.size())
				{
					resolveFormat = resolveFramebuffer->_colorTargets[resolveTargetIndex]->vulkanTargetViewDescriptor.format;
				}
				renderPassState.resolveFormats.push_back(resolveFormat);
			}
		}

		if(framebuffer->_depthStencilTarget)
		{
			renderPassState.imageFormats.push_back(framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format);
		}

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			renderPassState.imageFormats.push_back(fragmentDensityFramebuffer->_fragmentDensityTargets[0]->vulkanTargetViewDescriptor.format);
		}

		renderPassState.subpassSignature = rootVulkanPass->subpassSignature;
		renderPassState.imageSampleCount = framebuffer->GetSampleCount();
		renderPassState.resolveSampleCount = resolveFramebuffer ? resolveFramebuffer->GetSampleCount() : 1;

		for(VulkanRenderPassState *state : _renderPassStates)
		{
			if((*state) == renderPassState) return state;
		}

		VulkanRenderPassState *state = new VulkanRenderPassState;
		state->flags = renderPassState.flags;
		state->imageFormats = renderPassState.imageFormats;
		state->resolveFormats = renderPassState.resolveFormats;
		state->multiviewCount = renderPassState.multiviewCount;
		state->hasFragmentDensityMap = renderPassState.hasFragmentDensityMap;
		state->subpassSignature = renderPassState.subpassSignature;
		state->imageSampleCount = renderPassState.imageSampleCount;
		state->resolveSampleCount = renderPassState.resolveSampleCount;

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		std::vector<VkAttachmentReference> resolveAttachmentRefs;

		uint32 counter = 0;
		for(VulkanFramebuffer::VulkanTargetView *targetView : framebuffer->_colorTargets)
		{
			VkAttachmentReference colorReference = {};
			colorReference.attachment = attachments.size();
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentRefs.push_back(colorReference);

			VkAttachmentDescription attachment = {};
			attachment.format = targetView->vulkanTargetViewDescriptor.format;
			attachment.flags = 0;
			attachment.samples = static_cast<VkSampleCountFlagBits>(framebuffer->_sampleCount);
			if(flags & RenderPass::Flags::ClearColor) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else if(flags & RenderPass::Flags::LoadColor) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			else attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			if(flags & RenderPass::Flags::StoreColor) attachment.storeOp = resolveFramebuffer? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
			else attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			if(rootVulkanPass && rootVulkanPass->subpasses.size() > 0)
			{
				const auto colorAttachment = rootSubpassSnapshot.GetColorAttachment(counter);
				attachment.initialLayout = colorAttachment.GetFirstUseIsRead()? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.finalLayout = colorAttachment.GetLastUseIsRead()? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			attachments.push_back(attachment);

			if(resolveFramebuffer)
			{
				VkAttachmentReference resolveReference = {};
				if(resolveFramebuffer->_swapChain)
				{
					// Only color attachment 0 resolves to the swapchain. Other color attachments are not resolved.
					if(counter == 0)
					{
						resolveReference.attachment = attachments.size();
						resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						// Create a single resolve attachment for color[0]
						VkAttachmentDescription resolveAttachment = {};
						resolveAttachment.format = resolveFramebuffer->_colorTargets[0]->vulkanTargetViewDescriptor.format;
						resolveAttachment.flags = 0;
						resolveAttachment.samples = static_cast<VkSampleCountFlagBits>(resolveFramebuffer->_sampleCount);
						resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						resolveAttachment.storeOp = (flags & RenderPass::Flags::StoreColor)? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
						resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Could be PRESENT for swapchain
						attachments.push_back(resolveAttachment);
					}
					else
					{
						// No resolve attachment for non-zero color indices when resolving to swapchain
						resolveReference.attachment = VK_ATTACHMENT_UNUSED;
						resolveReference.layout = VK_IMAGE_LAYOUT_UNDEFINED;
					}
				}
				else
				{
					// Resolve to an offscreen framebuffer: create a resolve per color attachment
					resolveReference.attachment = attachments.size();
					resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

					VkAttachmentDescription resolveAttachment = {};
					resolveAttachment.format = resolveFramebuffer->_colorTargets[counter]->vulkanTargetViewDescriptor.format;
					resolveAttachment.flags = 0;
					resolveAttachment.samples = static_cast<VkSampleCountFlagBits>(resolveFramebuffer->_sampleCount);
					resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					resolveAttachment.storeOp = (flags & RenderPass::Flags::StoreColor)? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
					resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					attachments.push_back(resolveAttachment);
				}

				resolveAttachmentRefs.push_back(resolveReference);
			}

			counter += 1;
			if(framebuffer->_swapChain) break; //Swapchain only has one color attachment
		}

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;
		subpass.colorAttachmentCount = colorAttachmentRefs.size();
		subpass.pColorAttachments = colorAttachmentRefs.data();

		if(resolveFramebuffer)
		{
			subpass.pResolveAttachments = resolveAttachmentRefs.data();
		}

		VkAttachmentReference depthReference = {};
		if(framebuffer->_depthStencilTarget)
		{
			depthReference.attachment = attachments.size();
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription attachment = {};
			attachment.format = framebuffer->_depthStencilTarget->vulkanTargetViewDescriptor.format;
			attachment.flags = 0;
			attachment.samples = static_cast<VkSampleCountFlagBits>(framebuffer->_sampleCount);
			if(flags & RenderPass::Flags::ClearDepthStencil) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else if(flags & RenderPass::Flags::LoadDepthStencil) attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			else attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			if(flags & RenderPass::Flags::StoreDepthStencil) attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			else attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			// Choose initial/final layouts based on depth usage across subpasses (read-only vs write)
			if(rootVulkanPass)
			{
				bool depthFirstIsReadOnly = rootSubpassSnapshot.GetDepthFirstUseIsRead();
				bool depthLastIsReadOnly = rootSubpassSnapshot.GetDepthLastUseIsRead();

				attachment.initialLayout = depthFirstIsReadOnly? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachment.finalLayout = depthLastIsReadOnly? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			attachments.push_back(attachment);

			subpass.pDepthStencilAttachment = &depthReference;

			//TODO: Figure out depth resolve!?
		}

		VkRenderPassFragmentDensityMapCreateInfoEXT fragmentDensityMapCreateInfo = {};
		fragmentDensityMapCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
		fragmentDensityMapCreateInfo.pNext = nullptr;

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			fragmentDensityMapCreateInfo.fragmentDensityMapAttachment.attachment = attachments.size();
			fragmentDensityMapCreateInfo.fragmentDensityMapAttachment.layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;

			VkAttachmentDescription attachment = {};
			attachment.format = fragmentDensityFramebuffer->_fragmentDensityTargets[0]->vulkanTargetViewDescriptor.format;
			attachment.flags = 0;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //This is ok for fragment density maps
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
			attachment.finalLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
			attachments.push_back(attachment);
		}

		// Build subpass array (explicit or default)
		std::vector<VkSubpassDescription> multiSubpasses;
		std::vector<VkSubpassDependency> subpassDependencies;
		std::vector<std::vector<VkAttachmentReference>> perSubpassColorRefs;
		std::vector<std::vector<uint32>> perSubpassColorIndices; // map pColorAttachments order to color index
		std::vector<std::vector<VkAttachmentReference>> perSubpassInputRefs;
		std::vector<std::vector<VkAttachmentReference>> perSubpassResolveRefs;
		std::vector<VkAttachmentReference> perSubpassDepthRefs;
		std::vector<std::vector<uint32>> perSubpassPreserveAttachments;

		if(rootVulkanPass && rootVulkanPass->subpasses.size() > 0)
		{
			perSubpassColorRefs.resize(rootVulkanPass->subpasses.size());
			perSubpassColorIndices.resize(rootVulkanPass->subpasses.size());
			perSubpassInputRefs.resize(rootVulkanPass->subpasses.size());
			perSubpassResolveRefs.resize(rootVulkanPass->subpasses.size());
			perSubpassDepthRefs.resize(rootVulkanPass->subpasses.size());
			perSubpassPreserveAttachments.resize(rootVulkanPass->subpasses.size());

			for(size_t si = 0; si < rootVulkanPass->subpasses.size(); si++)
			{
				const RenderPass::SubpassSnapshot &subpassSnapshot = renderFrame.GetPass(rootVulkanPass->subpasses[si].renderFramePassIndex).GetDrawSnapshot().GetSubpass();

				VkSubpassDescription sp = {};
				sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

				for(uint32 ci = 0; ci < colorAttachmentRefs.size(); ci++)
				{
					const auto colorAttachment = subpassSnapshot.GetColorAttachment(ci);
					if(colorAttachment.GetWrites())
					{
						perSubpassColorRefs[si].push_back({ colorAttachmentRefs[ci].attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
						perSubpassColorIndices[si].push_back(ci);
					}
					else if(colorAttachment.GetReads())
					{
						perSubpassInputRefs[si].push_back({ colorAttachmentRefs[ci].attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
					}
				}
				sp.colorAttachmentCount = perSubpassColorRefs[si].size();
				sp.pColorAttachments = (sp.colorAttachmentCount > 0)? perSubpassColorRefs[si].data() : nullptr;

				if(framebuffer->_depthStencilTarget)
				{
					if(subpassSnapshot.GetWritesDepthStencil())
					{
						perSubpassDepthRefs[si] = { depthReference.attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
						sp.pDepthStencilAttachment = &perSubpassDepthRefs[si];
					}
					else if(subpassSnapshot.GetReadsDepthStencil())
					{
						// Keep depth bound for depth testing but in read-only layout
						perSubpassDepthRefs[si] = { depthReference.attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
						sp.pDepthStencilAttachment = &perSubpassDepthRefs[si];
						// Also expose as input attachment if shader wants to sample depth
						perSubpassInputRefs[si].push_back({ depthReference.attachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL });
					}
				}

				sp.inputAttachmentCount = perSubpassInputRefs[si].size();
				sp.pInputAttachments = (sp.inputAttachmentCount > 0)? perSubpassInputRefs[si].data() : nullptr;

				// Preserve attachments that are not used in this subpass but needed later (color and depth)
				std::vector<uint32> preserveAttachments;
				// Color attachments
				for(uint32 ci = 0; ci < colorAttachmentRefs.size(); ci++)
				{
					if(subpassSnapshot.GetColorAttachment(ci).GetNeedsPreserve())
					{
						preserveAttachments.push_back(colorAttachmentRefs[ci].attachment);
					}
				}
				// Depth attachment
				if(framebuffer->_depthStencilTarget)
				{
					if(subpassSnapshot.GetNeedsToPreserveDepthStencil())
					{
						preserveAttachments.push_back(depthReference.attachment);
					}
				}

				perSubpassPreserveAttachments[si] = preserveAttachments;
				sp.preserveAttachmentCount = preserveAttachments.size();
				sp.pPreserveAttachments = (preserveAttachments.size() > 0)? perSubpassPreserveAttachments[si].data() : nullptr;

				// Resolve attachments only on last writer subpasses
				if(resolveFramebuffer && sp.colorAttachmentCount > 0)
				{
					perSubpassResolveRefs[si].reserve(perSubpassColorRefs[si].size());
					bool anyLast = false;
					for(size_t k = 0; k < perSubpassColorIndices[si].size(); ++k)
					{
						uint32 ci = perSubpassColorIndices[si][k];
						if(subpassSnapshot.GetColorAttachment(ci).GetIsLastWrite())
						{
							perSubpassResolveRefs[si].push_back({ resolveAttachmentRefs[ci].attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
							anyLast = true;
						}
						else
						{
							perSubpassResolveRefs[si].push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED });
						}
					}
					sp.pResolveAttachments = anyLast ? perSubpassResolveRefs[si].data() : nullptr;
				}

				multiSubpasses.push_back(sp);

				if(si > 0)
				{
					VkSubpassDependency dep = {};
					dep.srcSubpass = si - 1;
					dep.dstSubpass = si;
					dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
					dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
					if(multiviewCount > 1)
					{
						dep.dependencyFlags = static_cast<VkDependencyFlags>(dep.dependencyFlags | VK_DEPENDENCY_VIEW_LOCAL_BIT);
					}
					subpassDependencies.push_back(dep);
				}
			}
		}
		else
		{
			// No explicit subpasses: use default single subpass
			multiSubpasses.push_back(subpass);
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = multiSubpasses.size();
		renderPassInfo.pSubpasses = multiSubpasses.data();
		renderPassInfo.dependencyCount = subpassDependencies.size();
		renderPassInfo.pDependencies = (subpassDependencies.size() > 0)? subpassDependencies.data() : nullptr;

		//Multiview stuff
		uint32 viewMask = 0;
		uint32 correlationMask = 0;
		if(multiviewCount > 1)
		{
			for(int i = 0; i < multiviewCount; i++)
			{
				viewMask |= (1 << i);
				correlationMask |= (1 << i);
			}
		}
		VkRenderPassMultiviewCreateInfoKHR multiviewPassInfo = {};
		multiviewPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		multiviewPassInfo.pNext = nullptr;
		std::vector<uint32> perSubpassViewMasks;
		if(multiviewCount > 1)
		{
			perSubpassViewMasks.resize(std::max<size_t>(1, multiSubpasses.size()), viewMask);
			multiviewPassInfo.subpassCount = static_cast<uint32>(perSubpassViewMasks.size());
			multiviewPassInfo.pViewMasks = perSubpassViewMasks.data();
		}
		multiviewPassInfo.dependencyCount = 0;
		multiviewPassInfo.correlationMaskCount = 1;
		multiviewPassInfo.pCorrelationMasks = &correlationMask;

		if(multiviewCount > 1)
		{
			renderPassInfo.pNext = &multiviewPassInfo;
		}

		if(fragmentDensityFramebuffer->_fragmentDensityTargets.size() > 0)
		{
			if(multiviewCount > 1)
			{
				multiviewPassInfo.pNext = &fragmentDensityMapCreateInfo;
			}
			else
			{
				renderPassInfo.pNext = &fragmentDensityMapCreateInfo;
			}
		}

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();
		RNVulkanValidate(vk::CreateRenderPass(device->GetDevice(), &renderPassInfo, renderer->GetAllocatorCallback(), &state->renderPass));

		_renderPassStates.push_back(state);

		return state;
	}
}
