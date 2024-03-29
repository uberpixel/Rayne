//
//  RNVulkanInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINTERNALS_H__
#define __RAYNE_VULKANINTERNALS_H__

#include "RNVulkan.h"
#include "RNVulkanStateCoordinator.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanSwapChain.h"

#include <vk_mem_alloc.h>

namespace RN
{
	class BufferedDescriptorSet
	{
		public:
			BufferedDescriptorSet() : _layout(VK_NULL_HANDLE), _currentIndex(0), _resetFrame(0)
			{

			}

			~BufferedDescriptorSet()
			{
				VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
				if(_descriptorSets.size() > 0) RNVulkanValidate(vk::FreeDescriptorSets(renderer->GetVulkanDevice()->GetDevice(), renderer->GetDescriptorPool(), _descriptorSets.size(), _descriptorSets.data()));
			}

			void UpdateLayout(VkDescriptorSetLayout layout, size_t currentFrame)
			{
				_layout = layout;
				_resetFrame = currentFrame;
			}

			void Advance(size_t currentFrame, size_t completedFrame)
			{
				if(_descriptorSets.size() == 0)
				{
					VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.pNext = NULL;
					descriptorSetAllocateInfo.descriptorPool = renderer->GetDescriptorPool();
					descriptorSetAllocateInfo.pSetLayouts = &_layout;
					descriptorSetAllocateInfo.descriptorSetCount = 1;

					VkDescriptorSet descriptorSet;
					RNVulkanValidate(vk::AllocateDescriptorSets(renderer->GetVulkanDevice()->GetDevice(), &descriptorSetAllocateInfo, &descriptorSet));

					_descriptorSets.push_back(descriptorSet);
					_usedFrames.push_back(currentFrame);

					return;
				}

				_currentIndex = (_currentIndex + 1) % _descriptorSets.size();

				if(_usedFrames[_currentIndex] <= completedFrame && _usedFrames[_currentIndex] <= _resetFrame && completedFrame != -1)
				{
					VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
					RNVulkanValidate(vk::FreeDescriptorSets(renderer->GetVulkanDevice()->GetDevice(), renderer->GetDescriptorPool(), 1, &_descriptorSets[_currentIndex]));

					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.pNext = NULL;
					descriptorSetAllocateInfo.descriptorPool = renderer->GetDescriptorPool();
					descriptorSetAllocateInfo.pSetLayouts = &_layout;
					descriptorSetAllocateInfo.descriptorSetCount = 1;

					RNVulkanValidate(vk::AllocateDescriptorSets(renderer->GetVulkanDevice()->GetDevice(), &descriptorSetAllocateInfo, &_descriptorSets[_currentIndex]));
				}

				if(_usedFrames[_currentIndex] > completedFrame || completedFrame == -1)
				{
					VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.pNext = NULL;
					descriptorSetAllocateInfo.descriptorPool = renderer->GetDescriptorPool();
					descriptorSetAllocateInfo.pSetLayouts = &_layout;
					descriptorSetAllocateInfo.descriptorSetCount = 1;

					VkDescriptorSet descriptorSet;
					RNVulkanValidate(vk::AllocateDescriptorSets(renderer->GetVulkanDevice()->GetDevice(), &descriptorSetAllocateInfo, &descriptorSet));

					_currentIndex += 1;
					if(_currentIndex >= _descriptorSets.size())
					{
						_descriptorSets.push_back(descriptorSet);
						_usedFrames.push_back(currentFrame);
					}
					else
					{
						_descriptorSets.insert(_descriptorSets.begin() + _currentIndex, descriptorSet);
						_usedFrames.insert(_usedFrames.begin() + _currentIndex, currentFrame);
					}
				}
				else
				{
					_usedFrames[_currentIndex] = currentFrame;
				}
			}

			VkDescriptorSet GetActiveDescriptorSet()
			{
				return _descriptorSets[_currentIndex];
			}

		private:
			VkDescriptorSetLayout _layout;

			size_t _currentIndex;
			size_t _resetFrame;
			std::vector<VkDescriptorSet> _descriptorSets;
			std::vector<size_t> _usedFrames;
	};

	struct VulkanDrawable : public Drawable
	{
		struct CameraSpecific
		{
			const VulkanPipelineState *pipelineState; //No need for cleanup here as it's shared, but maybe add reference counting to clear later.
			VulkanUniformState *uniformState;
			bool dirty;
			BufferedDescriptorSet *descriptorSet;
            Camera *camera;
		};

		~VulkanDrawable()
		{
			VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
			for(CameraSpecific &specific : _cameraSpecifics)
			{
				renderer->AddFrameFinishedCallback([specific](){
					if(specific.descriptorSet)
					{
						delete specific.descriptorSet;
					}
					if(specific.uniformState)
					{
						delete specific.uniformState;
					}
				});
			}
		}

		void AddCameraSpecificsIfNeeded(size_t cameraID)
		{
			while(_cameraSpecifics.size() <= cameraID)
			{
				_cameraSpecifics.push_back({ nullptr, nullptr, true, nullptr });
			}
		}

		void UpdateRenderingState(size_t cameraID, Camera *camera, const VulkanPipelineState *pipelineState, VulkanUniformState *uniformState)
		{
			_cameraSpecifics[cameraID].pipelineState = pipelineState;
            _cameraSpecifics[cameraID].camera = camera;
			if(_cameraSpecifics[cameraID].uniformState)
			{
				VulkanUniformState *oldUniformState = _cameraSpecifics[cameraID].uniformState;
				if(oldUniformState)
				{
					VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
					renderer->AddFrameFinishedCallback([oldUniformState](){
						if(oldUniformState)
						{
							delete oldUniformState;
						}
					});
				}
			}
			_cameraSpecifics[cameraID].uniformState = uniformState;
			_cameraSpecifics[cameraID].dirty = false;
		}

		virtual void MakeDirty() override
		{
			for(int i = 0; i < _cameraSpecifics.size(); i++)
			{
				_cameraSpecifics[i].dirty = true;
			}
		}

		//TODO: This can get somewhat big with lots of post processing stages...
		std::vector<CameraSpecific> _cameraSpecifics;
	};

	struct VulkanDirectionalLight
	{
		Vector3 direction;
		float padding;
		Vector4 color;
	};

    struct VulkanPointLight
	{
		Vector3 position;
		float range;
		Vector4 color;
	};

	struct VulkanSpotLight
	{
		Vector3 position;
		float range;
		Vector3 direction;
		float angle;
		Vector4 color;
	};

	struct VulkanRenderPassCameraInfo
	{
		Camera *camera;
		Vector3 viewPosition;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;
	};

	struct VulkanRenderPass
	{
		enum Type
		{
			Default,
			ResolveMSAA,
			Blit,
			Convert
		};

		Type type;
		RenderPass *renderPass;
		RenderPass *previousRenderPass;

		VulkanFramebuffer *framebuffer;
		VulkanFramebuffer *resolveFramebuffer;
		Shader::UsageHint shaderHint;
		Material *overrideMaterial;

		Color cameraAmbientColor;
		Color cameraFogColor0;
		Color cameraFogColor1;
		Vector2 cameraClipDistance;
		Vector2 cameraFogDistance;
		VulkanRenderPassCameraInfo cameraInfo;
		std::vector<VulkanRenderPassCameraInfo> multiviewCameraInfo;
		uint8 multiviewLayer;

		std::vector<uint32> instanceSteps; //number of drawables in the drawables list that use the same pipeline state and can all be rendered with the same draw call as result
		std::vector<VulkanDrawable *> drawables;
		std::vector<VulkanDirectionalLight> directionalLights;
		std::vector<VulkanPointLight> pointLights;
		std::vector<VulkanSpotLight> spotLights;

		std::vector<VulkanTexture *> renderTargetsUsedInShader;

		std::vector<Matrix> directionalShadowMatrices;
		VulkanTexture *directionalShadowDepthTexture;
		Vector2 directionalShadowInfo;
	};

	struct VulkanFrameResource
	{
		size_t frame;
		std::function<void()> finishedCallback;
	};

	class VulkanCommandBuffer : public Object
	{
	public:
		friend RN::VulkanRenderer;

		~VulkanCommandBuffer();

		void Begin();
		void Reset();
		void End();

		VkCommandBuffer GetCommandBuffer() const {return _commandBuffer;}

	private:
		VulkanCommandBuffer(VkDevice device, VkCommandPool pool);

		VkCommandBuffer _commandBuffer;
		VkDevice _device;
		VkCommandPool _pool;
		uint32 _frameValue;

		RNDeclareMetaAPI(VulkanCommandBuffer, VKAPI)
	};

	struct VulkanRendererInternals
	{
		std::vector<VulkanRenderPass> renderPasses;
		VulkanStateCoordinator stateCoordinator;

		std::vector<VulkanSwapChain*> swapChains;
		std::vector<VulkanFrameResource> frameResources;

		size_t currentRenderPassIndex;
		size_t currentDrawableResourceIndex;
		size_t totalDrawableCount;

		size_t totalDescriptorTables;

		const VulkanPipelineState *currentPipelineState;
		VulkanDrawable *currentInstanceDrawable;

		VmaAllocator memoryAllocator;
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */
