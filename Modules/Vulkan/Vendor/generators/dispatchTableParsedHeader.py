from dispatchTableClasses import *
vk_core_0 = Extension(name='VK_core', version=0, guard=None, commands=[
    Command(name='CreateInstance', dispatch=None),
    Command(name='DestroyInstance', dispatch='VkInstance'),
    Command(name='EnumeratePhysicalDevices', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceFeatures', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceFormatProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceImageFormatProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceQueueFamilyProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceMemoryProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetInstanceProcAddr', dispatch='VkInstance'),
    Command(name='GetDeviceProcAddr', dispatch='VkDevice'),
    Command(name='CreateDevice', dispatch='VkPhysicalDevice'),
    Command(name='DestroyDevice', dispatch='VkDevice'),
    Command(name='EnumerateInstanceExtensionProperties', dispatch=None),
    Command(name='EnumerateDeviceExtensionProperties', dispatch='VkPhysicalDevice'),
    Command(name='EnumerateInstanceLayerProperties', dispatch=None),
    Command(name='EnumerateDeviceLayerProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetDeviceQueue', dispatch='VkDevice'),
    Command(name='QueueSubmit', dispatch='VkQueue'),
    Command(name='QueueWaitIdle', dispatch='VkQueue'),
    Command(name='DeviceWaitIdle', dispatch='VkDevice'),
    Command(name='AllocateMemory', dispatch='VkDevice'),
    Command(name='FreeMemory', dispatch='VkDevice'),
    Command(name='MapMemory', dispatch='VkDevice'),
    Command(name='UnmapMemory', dispatch='VkDevice'),
    Command(name='FlushMappedMemoryRanges', dispatch='VkDevice'),
    Command(name='InvalidateMappedMemoryRanges', dispatch='VkDevice'),
    Command(name='GetDeviceMemoryCommitment', dispatch='VkDevice'),
    Command(name='BindBufferMemory', dispatch='VkDevice'),
    Command(name='BindImageMemory', dispatch='VkDevice'),
    Command(name='GetBufferMemoryRequirements', dispatch='VkDevice'),
    Command(name='GetImageMemoryRequirements', dispatch='VkDevice'),
    Command(name='GetImageSparseMemoryRequirements', dispatch='VkDevice'),
    Command(name='GetPhysicalDeviceSparseImageFormatProperties', dispatch='VkPhysicalDevice'),
    Command(name='QueueBindSparse', dispatch='VkQueue'),
    Command(name='CreateFence', dispatch='VkDevice'),
    Command(name='DestroyFence', dispatch='VkDevice'),
    Command(name='ResetFences', dispatch='VkDevice'),
    Command(name='GetFenceStatus', dispatch='VkDevice'),
    Command(name='WaitForFences', dispatch='VkDevice'),
    Command(name='CreateSemaphore', dispatch='VkDevice'),
    Command(name='DestroySemaphore', dispatch='VkDevice'),
    Command(name='CreateEvent', dispatch='VkDevice'),
    Command(name='DestroyEvent', dispatch='VkDevice'),
    Command(name='GetEventStatus', dispatch='VkDevice'),
    Command(name='SetEvent', dispatch='VkDevice'),
    Command(name='ResetEvent', dispatch='VkDevice'),
    Command(name='CreateQueryPool', dispatch='VkDevice'),
    Command(name='DestroyQueryPool', dispatch='VkDevice'),
    Command(name='GetQueryPoolResults', dispatch='VkDevice'),
    Command(name='CreateBuffer', dispatch='VkDevice'),
    Command(name='DestroyBuffer', dispatch='VkDevice'),
    Command(name='CreateBufferView', dispatch='VkDevice'),
    Command(name='DestroyBufferView', dispatch='VkDevice'),
    Command(name='CreateImage', dispatch='VkDevice'),
    Command(name='DestroyImage', dispatch='VkDevice'),
    Command(name='GetImageSubresourceLayout', dispatch='VkDevice'),
    Command(name='CreateImageView', dispatch='VkDevice'),
    Command(name='DestroyImageView', dispatch='VkDevice'),
    Command(name='CreateShaderModule', dispatch='VkDevice'),
    Command(name='DestroyShaderModule', dispatch='VkDevice'),
    Command(name='CreatePipelineCache', dispatch='VkDevice'),
    Command(name='DestroyPipelineCache', dispatch='VkDevice'),
    Command(name='GetPipelineCacheData', dispatch='VkDevice'),
    Command(name='MergePipelineCaches', dispatch='VkDevice'),
    Command(name='CreateGraphicsPipelines', dispatch='VkDevice'),
    Command(name='CreateComputePipelines', dispatch='VkDevice'),
    Command(name='DestroyPipeline', dispatch='VkDevice'),
    Command(name='CreatePipelineLayout', dispatch='VkDevice'),
    Command(name='DestroyPipelineLayout', dispatch='VkDevice'),
    Command(name='CreateSampler', dispatch='VkDevice'),
    Command(name='DestroySampler', dispatch='VkDevice'),
    Command(name='CreateDescriptorSetLayout', dispatch='VkDevice'),
    Command(name='DestroyDescriptorSetLayout', dispatch='VkDevice'),
    Command(name='CreateDescriptorPool', dispatch='VkDevice'),
    Command(name='DestroyDescriptorPool', dispatch='VkDevice'),
    Command(name='ResetDescriptorPool', dispatch='VkDevice'),
    Command(name='AllocateDescriptorSets', dispatch='VkDevice'),
    Command(name='FreeDescriptorSets', dispatch='VkDevice'),
    Command(name='UpdateDescriptorSets', dispatch='VkDevice'),
    Command(name='CreateFramebuffer', dispatch='VkDevice'),
    Command(name='DestroyFramebuffer', dispatch='VkDevice'),
    Command(name='CreateRenderPass', dispatch='VkDevice'),
    Command(name='DestroyRenderPass', dispatch='VkDevice'),
    Command(name='GetRenderAreaGranularity', dispatch='VkDevice'),
    Command(name='CreateCommandPool', dispatch='VkDevice'),
    Command(name='DestroyCommandPool', dispatch='VkDevice'),
    Command(name='ResetCommandPool', dispatch='VkDevice'),
    Command(name='AllocateCommandBuffers', dispatch='VkDevice'),
    Command(name='FreeCommandBuffers', dispatch='VkDevice'),
    Command(name='BeginCommandBuffer', dispatch='VkCommandBuffer'),
    Command(name='EndCommandBuffer', dispatch='VkCommandBuffer'),
    Command(name='ResetCommandBuffer', dispatch='VkCommandBuffer'),
    Command(name='CmdBindPipeline', dispatch='VkCommandBuffer'),
    Command(name='CmdSetViewport', dispatch='VkCommandBuffer'),
    Command(name='CmdSetScissor', dispatch='VkCommandBuffer'),
    Command(name='CmdSetLineWidth', dispatch='VkCommandBuffer'),
    Command(name='CmdSetDepthBias', dispatch='VkCommandBuffer'),
    Command(name='CmdSetBlendConstants', dispatch='VkCommandBuffer'),
    Command(name='CmdSetDepthBounds', dispatch='VkCommandBuffer'),
    Command(name='CmdSetStencilCompareMask', dispatch='VkCommandBuffer'),
    Command(name='CmdSetStencilWriteMask', dispatch='VkCommandBuffer'),
    Command(name='CmdSetStencilReference', dispatch='VkCommandBuffer'),
    Command(name='CmdBindDescriptorSets', dispatch='VkCommandBuffer'),
    Command(name='CmdBindIndexBuffer', dispatch='VkCommandBuffer'),
    Command(name='CmdBindVertexBuffers', dispatch='VkCommandBuffer'),
    Command(name='CmdDraw', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndexed', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndirect', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndexedIndirect', dispatch='VkCommandBuffer'),
    Command(name='CmdDispatch', dispatch='VkCommandBuffer'),
    Command(name='CmdDispatchIndirect', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyBuffer', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyImage', dispatch='VkCommandBuffer'),
    Command(name='CmdBlitImage', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyBufferToImage', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyImageToBuffer', dispatch='VkCommandBuffer'),
    Command(name='CmdUpdateBuffer', dispatch='VkCommandBuffer'),
    Command(name='CmdFillBuffer', dispatch='VkCommandBuffer'),
    Command(name='CmdClearColorImage', dispatch='VkCommandBuffer'),
    Command(name='CmdClearDepthStencilImage', dispatch='VkCommandBuffer'),
    Command(name='CmdClearAttachments', dispatch='VkCommandBuffer'),
    Command(name='CmdResolveImage', dispatch='VkCommandBuffer'),
    Command(name='CmdSetEvent', dispatch='VkCommandBuffer'),
    Command(name='CmdResetEvent', dispatch='VkCommandBuffer'),
    Command(name='CmdWaitEvents', dispatch='VkCommandBuffer'),
    Command(name='CmdPipelineBarrier', dispatch='VkCommandBuffer'),
    Command(name='CmdBeginQuery', dispatch='VkCommandBuffer'),
    Command(name='CmdEndQuery', dispatch='VkCommandBuffer'),
    Command(name='CmdResetQueryPool', dispatch='VkCommandBuffer'),
    Command(name='CmdWriteTimestamp', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyQueryPoolResults', dispatch='VkCommandBuffer'),
    Command(name='CmdPushConstants', dispatch='VkCommandBuffer'),
    Command(name='CmdBeginRenderPass', dispatch='VkCommandBuffer'),
    Command(name='CmdNextSubpass', dispatch='VkCommandBuffer'),
    Command(name='CmdEndRenderPass', dispatch='VkCommandBuffer'),
    Command(name='CmdExecuteCommands', dispatch='VkCommandBuffer'),
])

vk_core_1 = Extension(name='VK_core', version=1, guard=None, commands=[
    Command(name='EnumerateInstanceVersion', dispatch=None),
    Command(name='BindBufferMemory2', dispatch='VkDevice'),
    Command(name='BindImageMemory2', dispatch='VkDevice'),
    Command(name='GetDeviceGroupPeerMemoryFeatures', dispatch='VkDevice'),
    Command(name='CmdSetDeviceMask', dispatch='VkCommandBuffer'),
    Command(name='CmdDispatchBase', dispatch='VkCommandBuffer'),
    Command(name='EnumeratePhysicalDeviceGroups', dispatch='VkInstance'),
    Command(name='GetImageMemoryRequirements2', dispatch='VkDevice'),
    Command(name='GetBufferMemoryRequirements2', dispatch='VkDevice'),
    Command(name='GetImageSparseMemoryRequirements2', dispatch='VkDevice'),
    Command(name='GetPhysicalDeviceFeatures2', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceProperties2', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceFormatProperties2', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceImageFormatProperties2', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceQueueFamilyProperties2', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceMemoryProperties2', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceSparseImageFormatProperties2', dispatch='VkPhysicalDevice'),
    Command(name='TrimCommandPool', dispatch='VkDevice'),
    Command(name='GetDeviceQueue2', dispatch='VkDevice'),
    Command(name='CreateSamplerYcbcrConversion', dispatch='VkDevice'),
    Command(name='DestroySamplerYcbcrConversion', dispatch='VkDevice'),
    Command(name='CreateDescriptorUpdateTemplate', dispatch='VkDevice'),
    Command(name='DestroyDescriptorUpdateTemplate', dispatch='VkDevice'),
    Command(name='UpdateDescriptorSetWithTemplate', dispatch='VkDevice'),
    Command(name='GetPhysicalDeviceExternalBufferProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceExternalFenceProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceExternalSemaphoreProperties', dispatch='VkPhysicalDevice'),
    Command(name='GetDescriptorSetLayoutSupport', dispatch='VkDevice'),
])

vk_core_2 = Extension(name='VK_core', version=2, guard=None, commands=[
    Command(name='CmdDrawIndirectCount', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndexedIndirectCount', dispatch='VkCommandBuffer'),
    Command(name='CreateRenderPass2', dispatch='VkDevice'),
    Command(name='CmdBeginRenderPass2', dispatch='VkCommandBuffer'),
    Command(name='CmdNextSubpass2', dispatch='VkCommandBuffer'),
    Command(name='CmdEndRenderPass2', dispatch='VkCommandBuffer'),
    Command(name='ResetQueryPool', dispatch='VkDevice'),
    Command(name='GetSemaphoreCounterValue', dispatch='VkDevice'),
    Command(name='WaitSemaphores', dispatch='VkDevice'),
    Command(name='SignalSemaphore', dispatch='VkDevice'),
    Command(name='GetBufferDeviceAddress', dispatch='VkDevice'),
    Command(name='GetBufferOpaqueCaptureAddress', dispatch='VkDevice'),
    Command(name='GetDeviceMemoryOpaqueCaptureAddress', dispatch='VkDevice'),
])

vk_khr_surface_25 = Extension(name='VK_KHR_surface', version=25, guard=None, commands=[
    Command(name='DestroySurfaceKHR', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceSurfaceSupportKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceSurfaceCapabilitiesKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceSurfaceFormatsKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceSurfacePresentModesKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_swapchain_70 = Extension(name='VK_KHR_swapchain', version=70, guard=None, commands=[
    Command(name='CreateSwapchainKHR', dispatch='VkDevice'),
    Command(name='DestroySwapchainKHR', dispatch='VkDevice'),
    Command(name='GetSwapchainImagesKHR', dispatch='VkDevice'),
    Command(name='AcquireNextImageKHR', dispatch='VkDevice'),
    Command(name='QueuePresentKHR', dispatch='VkQueue'),
    Command(name='GetDeviceGroupPresentCapabilitiesKHR', dispatch='VkDevice'),
    Command(name='GetDeviceGroupSurfacePresentModesKHR', dispatch='VkDevice'),
    Command(name='GetPhysicalDevicePresentRectanglesKHR', dispatch='VkPhysicalDevice'),
    Command(name='AcquireNextImage2KHR', dispatch='VkDevice'),
])

vk_khr_display_23 = Extension(name='VK_KHR_display', version=23, guard=None, commands=[
    Command(name='GetPhysicalDeviceDisplayPropertiesKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceDisplayPlanePropertiesKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetDisplayPlaneSupportedDisplaysKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetDisplayModePropertiesKHR', dispatch='VkPhysicalDevice'),
    Command(name='CreateDisplayModeKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetDisplayPlaneCapabilitiesKHR', dispatch='VkPhysicalDevice'),
    Command(name='CreateDisplayPlaneSurfaceKHR', dispatch='VkInstance'),
])

vk_khr_display_swapchain_10 = Extension(name='VK_KHR_display_swapchain', version=10, guard=None, commands=[
    Command(name='CreateSharedSwapchainsKHR', dispatch='VkDevice'),
])

vk_khr_sampler_mirror_clamp_to_edge_3 = Extension(name='VK_KHR_sampler_mirror_clamp_to_edge', version=3, guard=None, commands=[
])

vk_khr_multiview_1 = Extension(name='VK_KHR_multiview', version=1, guard=None, commands=[
])

vk_khr_get_physical_device_properties2_2 = Extension(name='VK_KHR_get_physical_device_properties2', version=2, guard=None, commands=[
    Command(name='GetPhysicalDeviceFeatures2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceFormatProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceImageFormatProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceQueueFamilyProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceMemoryProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceSparseImageFormatProperties2KHR', dispatch='VkPhysicalDevice'),
])

vk_khr_device_group_4 = Extension(name='VK_KHR_device_group', version=4, guard=None, commands=[
    Command(name='GetDeviceGroupPeerMemoryFeaturesKHR', dispatch='VkDevice'),
    Command(name='CmdSetDeviceMaskKHR', dispatch='VkCommandBuffer'),
    Command(name='CmdDispatchBaseKHR', dispatch='VkCommandBuffer'),
])

vk_khr_shader_draw_parameters_1 = Extension(name='VK_KHR_shader_draw_parameters', version=1, guard=None, commands=[
])

vk_khr_maintenance1_2 = Extension(name='VK_KHR_maintenance1', version=2, guard=None, commands=[
    Command(name='TrimCommandPoolKHR', dispatch='VkDevice'),
])

vk_khr_device_group_creation_1 = Extension(name='VK_KHR_device_group_creation', version=1, guard=None, commands=[
    Command(name='EnumeratePhysicalDeviceGroupsKHR', dispatch='VkInstance'),
])

vk_khr_external_memory_capabilities_1 = Extension(name='VK_KHR_external_memory_capabilities', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceExternalBufferPropertiesKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_external_memory_1 = Extension(name='VK_KHR_external_memory', version=1, guard=None, commands=[
])

vk_khr_external_memory_fd_1 = Extension(name='VK_KHR_external_memory_fd', version=1, guard=None, commands=[
    Command(name='GetMemoryFdKHR', dispatch='VkDevice'),
    Command(name='GetMemoryFdPropertiesKHR', dispatch='VkDevice'),
])

vk_khr_external_semaphore_capabilities_1 = Extension(name='VK_KHR_external_semaphore_capabilities', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceExternalSemaphorePropertiesKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_external_semaphore_1 = Extension(name='VK_KHR_external_semaphore', version=1, guard=None, commands=[
])

vk_khr_external_semaphore_fd_1 = Extension(name='VK_KHR_external_semaphore_fd', version=1, guard=None, commands=[
    Command(name='ImportSemaphoreFdKHR', dispatch='VkDevice'),
    Command(name='GetSemaphoreFdKHR', dispatch='VkDevice'),
])

vk_khr_push_descriptor_2 = Extension(name='VK_KHR_push_descriptor', version=2, guard=None, commands=[
    Command(name='CmdPushDescriptorSetKHR', dispatch='VkCommandBuffer'),
    Command(name='CmdPushDescriptorSetWithTemplateKHR', dispatch='VkCommandBuffer'),
])

vk_khr_shader_float16_int8_1 = Extension(name='VK_KHR_shader_float16_int8', version=1, guard=None, commands=[
])

vk_khr_16bit_storage_1 = Extension(name='VK_KHR_16bit_storage', version=1, guard=None, commands=[
])

vk_khr_incremental_present_1 = Extension(name='VK_KHR_incremental_present', version=1, guard=None, commands=[
])

vk_khr_descriptor_update_template_1 = Extension(name='VK_KHR_descriptor_update_template', version=1, guard=None, commands=[
    Command(name='CreateDescriptorUpdateTemplateKHR', dispatch='VkDevice'),
    Command(name='DestroyDescriptorUpdateTemplateKHR', dispatch='VkDevice'),
    Command(name='UpdateDescriptorSetWithTemplateKHR', dispatch='VkDevice'),
])

vk_khr_imageless_framebuffer_1 = Extension(name='VK_KHR_imageless_framebuffer', version=1, guard=None, commands=[
])

vk_khr_create_renderpass2_1 = Extension(name='VK_KHR_create_renderpass2', version=1, guard=None, commands=[
    Command(name='CreateRenderPass2KHR', dispatch='VkDevice'),
    Command(name='CmdBeginRenderPass2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdNextSubpass2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdEndRenderPass2KHR', dispatch='VkCommandBuffer'),
])

vk_khr_shared_presentable_image_1 = Extension(name='VK_KHR_shared_presentable_image', version=1, guard=None, commands=[
    Command(name='GetSwapchainStatusKHR', dispatch='VkDevice'),
])

vk_khr_external_fence_capabilities_1 = Extension(name='VK_KHR_external_fence_capabilities', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceExternalFencePropertiesKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_external_fence_1 = Extension(name='VK_KHR_external_fence', version=1, guard=None, commands=[
])

vk_khr_external_fence_fd_1 = Extension(name='VK_KHR_external_fence_fd', version=1, guard=None, commands=[
    Command(name='ImportFenceFdKHR', dispatch='VkDevice'),
    Command(name='GetFenceFdKHR', dispatch='VkDevice'),
])

vk_khr_performance_query_1 = Extension(name='VK_KHR_performance_query', version=1, guard=None, commands=[
    Command(name='EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR', dispatch='VkPhysicalDevice'),
    Command(name='AcquireProfilingLockKHR', dispatch='VkDevice'),
    Command(name='ReleaseProfilingLockKHR', dispatch='VkDevice'),
])

vk_khr_maintenance2_1 = Extension(name='VK_KHR_maintenance2', version=1, guard=None, commands=[
])

vk_khr_get_surface_capabilities2_1 = Extension(name='VK_KHR_get_surface_capabilities2', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceSurfaceCapabilities2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceSurfaceFormats2KHR', dispatch='VkPhysicalDevice'),
])

vk_khr_variable_pointers_1 = Extension(name='VK_KHR_variable_pointers', version=1, guard=None, commands=[
])

vk_khr_get_display_properties2_1 = Extension(name='VK_KHR_get_display_properties2', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceDisplayProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetPhysicalDeviceDisplayPlaneProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetDisplayModeProperties2KHR', dispatch='VkPhysicalDevice'),
    Command(name='GetDisplayPlaneCapabilities2KHR', dispatch='VkPhysicalDevice'),
])

vk_khr_dedicated_allocation_3 = Extension(name='VK_KHR_dedicated_allocation', version=3, guard=None, commands=[
])

vk_khr_storage_buffer_storage_class_1 = Extension(name='VK_KHR_storage_buffer_storage_class', version=1, guard=None, commands=[
])

vk_khr_relaxed_block_layout_1 = Extension(name='VK_KHR_relaxed_block_layout', version=1, guard=None, commands=[
])

vk_khr_get_memory_requirements2_1 = Extension(name='VK_KHR_get_memory_requirements2', version=1, guard=None, commands=[
    Command(name='GetImageMemoryRequirements2KHR', dispatch='VkDevice'),
    Command(name='GetBufferMemoryRequirements2KHR', dispatch='VkDevice'),
    Command(name='GetImageSparseMemoryRequirements2KHR', dispatch='VkDevice'),
])

vk_khr_image_format_list_1 = Extension(name='VK_KHR_image_format_list', version=1, guard=None, commands=[
])

vk_khr_sampler_ycbcr_conversion_14 = Extension(name='VK_KHR_sampler_ycbcr_conversion', version=14, guard=None, commands=[
    Command(name='CreateSamplerYcbcrConversionKHR', dispatch='VkDevice'),
    Command(name='DestroySamplerYcbcrConversionKHR', dispatch='VkDevice'),
])

vk_khr_bind_memory2_1 = Extension(name='VK_KHR_bind_memory2', version=1, guard=None, commands=[
    Command(name='BindBufferMemory2KHR', dispatch='VkDevice'),
    Command(name='BindImageMemory2KHR', dispatch='VkDevice'),
])

vk_khr_maintenance3_1 = Extension(name='VK_KHR_maintenance3', version=1, guard=None, commands=[
    Command(name='GetDescriptorSetLayoutSupportKHR', dispatch='VkDevice'),
])

vk_khr_draw_indirect_count_1 = Extension(name='VK_KHR_draw_indirect_count', version=1, guard=None, commands=[
    Command(name='CmdDrawIndirectCountKHR', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndexedIndirectCountKHR', dispatch='VkCommandBuffer'),
])

vk_khr_shader_subgroup_extended_types_1 = Extension(name='VK_KHR_shader_subgroup_extended_types', version=1, guard=None, commands=[
])

vk_khr_8bit_storage_1 = Extension(name='VK_KHR_8bit_storage', version=1, guard=None, commands=[
])

vk_khr_shader_atomic_int64_1 = Extension(name='VK_KHR_shader_atomic_int64', version=1, guard=None, commands=[
])

vk_khr_shader_clock_1 = Extension(name='VK_KHR_shader_clock', version=1, guard=None, commands=[
])

vk_khr_driver_properties_1 = Extension(name='VK_KHR_driver_properties', version=1, guard=None, commands=[
])

vk_khr_shader_float_controls_4 = Extension(name='VK_KHR_shader_float_controls', version=4, guard=None, commands=[
])

vk_khr_depth_stencil_resolve_1 = Extension(name='VK_KHR_depth_stencil_resolve', version=1, guard=None, commands=[
])

vk_khr_swapchain_mutable_format_1 = Extension(name='VK_KHR_swapchain_mutable_format', version=1, guard=None, commands=[
])

vk_khr_timeline_semaphore_2 = Extension(name='VK_KHR_timeline_semaphore', version=2, guard=None, commands=[
    Command(name='GetSemaphoreCounterValueKHR', dispatch='VkDevice'),
    Command(name='WaitSemaphoresKHR', dispatch='VkDevice'),
    Command(name='SignalSemaphoreKHR', dispatch='VkDevice'),
])

vk_khr_vulkan_memory_model_3 = Extension(name='VK_KHR_vulkan_memory_model', version=3, guard=None, commands=[
])

vk_khr_shader_terminate_invocation_1 = Extension(name='VK_KHR_shader_terminate_invocation', version=1, guard=None, commands=[
])

vk_khr_fragment_shading_rate_1 = Extension(name='VK_KHR_fragment_shading_rate', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceFragmentShadingRatesKHR', dispatch='VkPhysicalDevice'),
    Command(name='CmdSetFragmentShadingRateKHR', dispatch='VkCommandBuffer'),
])

vk_khr_spirv_1_4_1 = Extension(name='VK_KHR_spirv_1_4', version=1, guard=None, commands=[
])

vk_khr_surface_protected_capabilities_1 = Extension(name='VK_KHR_surface_protected_capabilities', version=1, guard=None, commands=[
])

vk_khr_separate_depth_stencil_layouts_1 = Extension(name='VK_KHR_separate_depth_stencil_layouts', version=1, guard=None, commands=[
])

vk_khr_uniform_buffer_standard_layout_1 = Extension(name='VK_KHR_uniform_buffer_standard_layout', version=1, guard=None, commands=[
])

vk_khr_buffer_device_address_1 = Extension(name='VK_KHR_buffer_device_address', version=1, guard=None, commands=[
    Command(name='GetBufferDeviceAddressKHR', dispatch='VkDevice'),
    Command(name='GetBufferOpaqueCaptureAddressKHR', dispatch='VkDevice'),
    Command(name='GetDeviceMemoryOpaqueCaptureAddressKHR', dispatch='VkDevice'),
])

vk_khr_deferred_host_operations_4 = Extension(name='VK_KHR_deferred_host_operations', version=4, guard=None, commands=[
    Command(name='CreateDeferredOperationKHR', dispatch='VkDevice'),
    Command(name='DestroyDeferredOperationKHR', dispatch='VkDevice'),
    Command(name='GetDeferredOperationMaxConcurrencyKHR', dispatch='VkDevice'),
    Command(name='GetDeferredOperationResultKHR', dispatch='VkDevice'),
    Command(name='DeferredOperationJoinKHR', dispatch='VkDevice'),
])

vk_khr_pipeline_executable_properties_1 = Extension(name='VK_KHR_pipeline_executable_properties', version=1, guard=None, commands=[
    Command(name='GetPipelineExecutablePropertiesKHR', dispatch='VkDevice'),
    Command(name='GetPipelineExecutableStatisticsKHR', dispatch='VkDevice'),
    Command(name='GetPipelineExecutableInternalRepresentationsKHR', dispatch='VkDevice'),
])

vk_khr_pipeline_library_1 = Extension(name='VK_KHR_pipeline_library', version=1, guard=None, commands=[
])

vk_khr_shader_non_semantic_info_1 = Extension(name='VK_KHR_shader_non_semantic_info', version=1, guard=None, commands=[
])

vk_khr_copy_commands2_1 = Extension(name='VK_KHR_copy_commands2', version=1, guard=None, commands=[
    Command(name='CmdCopyBuffer2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyImage2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyBufferToImage2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyImageToBuffer2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdBlitImage2KHR', dispatch='VkCommandBuffer'),
    Command(name='CmdResolveImage2KHR', dispatch='VkCommandBuffer'),
])

vk_ext_debug_report_9 = Extension(name='VK_EXT_debug_report', version=9, guard=None, commands=[
    Command(name='CreateDebugReportCallbackEXT', dispatch='VkInstance'),
    Command(name='DestroyDebugReportCallbackEXT', dispatch='VkInstance'),
    Command(name='DebugReportMessageEXT', dispatch='VkInstance'),
])

vk_nv_glsl_shader_1 = Extension(name='VK_NV_glsl_shader', version=1, guard=None, commands=[
])

vk_ext_depth_range_unrestricted_1 = Extension(name='VK_EXT_depth_range_unrestricted', version=1, guard=None, commands=[
])

vk_img_filter_cubic_1 = Extension(name='VK_IMG_filter_cubic', version=1, guard=None, commands=[
])

vk_amd_rasterization_order_1 = Extension(name='VK_AMD_rasterization_order', version=1, guard=None, commands=[
])

vk_amd_shader_trinary_minmax_1 = Extension(name='VK_AMD_shader_trinary_minmax', version=1, guard=None, commands=[
])

vk_amd_shader_explicit_vertex_parameter_1 = Extension(name='VK_AMD_shader_explicit_vertex_parameter', version=1, guard=None, commands=[
])

vk_ext_debug_marker_4 = Extension(name='VK_EXT_debug_marker', version=4, guard=None, commands=[
    Command(name='DebugMarkerSetObjectTagEXT', dispatch='VkDevice'),
    Command(name='DebugMarkerSetObjectNameEXT', dispatch='VkDevice'),
    Command(name='CmdDebugMarkerBeginEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdDebugMarkerEndEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdDebugMarkerInsertEXT', dispatch='VkCommandBuffer'),
])

vk_amd_gcn_shader_1 = Extension(name='VK_AMD_gcn_shader', version=1, guard=None, commands=[
])

vk_nv_dedicated_allocation_1 = Extension(name='VK_NV_dedicated_allocation', version=1, guard=None, commands=[
])

vk_ext_transform_feedback_1 = Extension(name='VK_EXT_transform_feedback', version=1, guard=None, commands=[
    Command(name='CmdBindTransformFeedbackBuffersEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdBeginTransformFeedbackEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdEndTransformFeedbackEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdBeginQueryIndexedEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdEndQueryIndexedEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndirectByteCountEXT', dispatch='VkCommandBuffer'),
])

vk_nvx_image_view_handle_2 = Extension(name='VK_NVX_image_view_handle', version=2, guard=None, commands=[
    Command(name='GetImageViewHandleNVX', dispatch='VkDevice'),
    Command(name='GetImageViewAddressNVX', dispatch='VkDevice'),
])

vk_amd_draw_indirect_count_2 = Extension(name='VK_AMD_draw_indirect_count', version=2, guard=None, commands=[
    Command(name='CmdDrawIndirectCountAMD', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawIndexedIndirectCountAMD', dispatch='VkCommandBuffer'),
])

vk_amd_negative_viewport_height_1 = Extension(name='VK_AMD_negative_viewport_height', version=1, guard=None, commands=[
])

vk_amd_gpu_shader_half_float_2 = Extension(name='VK_AMD_gpu_shader_half_float', version=2, guard=None, commands=[
])

vk_amd_shader_ballot_1 = Extension(name='VK_AMD_shader_ballot', version=1, guard=None, commands=[
])

vk_amd_texture_gather_bias_lod_1 = Extension(name='VK_AMD_texture_gather_bias_lod', version=1, guard=None, commands=[
])

vk_amd_shader_info_1 = Extension(name='VK_AMD_shader_info', version=1, guard=None, commands=[
    Command(name='GetShaderInfoAMD', dispatch='VkDevice'),
])

vk_amd_shader_image_load_store_lod_1 = Extension(name='VK_AMD_shader_image_load_store_lod', version=1, guard=None, commands=[
])

vk_nv_corner_sampled_image_2 = Extension(name='VK_NV_corner_sampled_image', version=2, guard=None, commands=[
])

vk_img_format_pvrtc_1 = Extension(name='VK_IMG_format_pvrtc', version=1, guard=None, commands=[
])

vk_nv_external_memory_capabilities_1 = Extension(name='VK_NV_external_memory_capabilities', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceExternalImageFormatPropertiesNV', dispatch='VkPhysicalDevice'),
])

vk_nv_external_memory_1 = Extension(name='VK_NV_external_memory', version=1, guard=None, commands=[
])

vk_ext_validation_flags_2 = Extension(name='VK_EXT_validation_flags', version=2, guard=None, commands=[
])

vk_ext_shader_subgroup_ballot_1 = Extension(name='VK_EXT_shader_subgroup_ballot', version=1, guard=None, commands=[
])

vk_ext_shader_subgroup_vote_1 = Extension(name='VK_EXT_shader_subgroup_vote', version=1, guard=None, commands=[
])

vk_ext_texture_compression_astc_hdr_1 = Extension(name='VK_EXT_texture_compression_astc_hdr', version=1, guard=None, commands=[
])

vk_ext_astc_decode_mode_1 = Extension(name='VK_EXT_astc_decode_mode', version=1, guard=None, commands=[
])

vk_ext_conditional_rendering_2 = Extension(name='VK_EXT_conditional_rendering', version=2, guard=None, commands=[
    Command(name='CmdBeginConditionalRenderingEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdEndConditionalRenderingEXT', dispatch='VkCommandBuffer'),
])

vk_nv_clip_space_w_scaling_1 = Extension(name='VK_NV_clip_space_w_scaling', version=1, guard=None, commands=[
    Command(name='CmdSetViewportWScalingNV', dispatch='VkCommandBuffer'),
])

vk_ext_direct_mode_display_1 = Extension(name='VK_EXT_direct_mode_display', version=1, guard=None, commands=[
    Command(name='ReleaseDisplayEXT', dispatch='VkPhysicalDevice'),
])

vk_ext_display_surface_counter_1 = Extension(name='VK_EXT_display_surface_counter', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceSurfaceCapabilities2EXT', dispatch='VkPhysicalDevice'),
])

vk_ext_display_control_1 = Extension(name='VK_EXT_display_control', version=1, guard=None, commands=[
    Command(name='DisplayPowerControlEXT', dispatch='VkDevice'),
    Command(name='RegisterDeviceEventEXT', dispatch='VkDevice'),
    Command(name='RegisterDisplayEventEXT', dispatch='VkDevice'),
    Command(name='GetSwapchainCounterEXT', dispatch='VkDevice'),
])

vk_google_display_timing_1 = Extension(name='VK_GOOGLE_display_timing', version=1, guard=None, commands=[
    Command(name='GetRefreshCycleDurationGOOGLE', dispatch='VkDevice'),
    Command(name='GetPastPresentationTimingGOOGLE', dispatch='VkDevice'),
])

vk_nv_sample_mask_override_coverage_1 = Extension(name='VK_NV_sample_mask_override_coverage', version=1, guard=None, commands=[
])

vk_nv_geometry_shader_passthrough_1 = Extension(name='VK_NV_geometry_shader_passthrough', version=1, guard=None, commands=[
])

vk_nv_viewport_array2_1 = Extension(name='VK_NV_viewport_array2', version=1, guard=None, commands=[
])

vk_nvx_multiview_per_view_attributes_1 = Extension(name='VK_NVX_multiview_per_view_attributes', version=1, guard=None, commands=[
])

vk_nv_viewport_swizzle_1 = Extension(name='VK_NV_viewport_swizzle', version=1, guard=None, commands=[
])

vk_ext_discard_rectangles_1 = Extension(name='VK_EXT_discard_rectangles', version=1, guard=None, commands=[
    Command(name='CmdSetDiscardRectangleEXT', dispatch='VkCommandBuffer'),
])

vk_ext_conservative_rasterization_1 = Extension(name='VK_EXT_conservative_rasterization', version=1, guard=None, commands=[
])

vk_ext_depth_clip_enable_1 = Extension(name='VK_EXT_depth_clip_enable', version=1, guard=None, commands=[
])

vk_ext_swapchain_colorspace_4 = Extension(name='VK_EXT_swapchain_colorspace', version=4, guard=None, commands=[
])

vk_ext_hdr_metadata_2 = Extension(name='VK_EXT_hdr_metadata', version=2, guard=None, commands=[
    Command(name='SetHdrMetadataEXT', dispatch='VkDevice'),
])

vk_ext_external_memory_dma_buf_1 = Extension(name='VK_EXT_external_memory_dma_buf', version=1, guard=None, commands=[
])

vk_ext_queue_family_foreign_1 = Extension(name='VK_EXT_queue_family_foreign', version=1, guard=None, commands=[
])

vk_ext_debug_utils_2 = Extension(name='VK_EXT_debug_utils', version=2, guard=None, commands=[
    Command(name='SetDebugUtilsObjectNameEXT', dispatch='VkDevice'),
    Command(name='SetDebugUtilsObjectTagEXT', dispatch='VkDevice'),
    Command(name='QueueBeginDebugUtilsLabelEXT', dispatch='VkQueue'),
    Command(name='QueueEndDebugUtilsLabelEXT', dispatch='VkQueue'),
    Command(name='QueueInsertDebugUtilsLabelEXT', dispatch='VkQueue'),
    Command(name='CmdBeginDebugUtilsLabelEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdEndDebugUtilsLabelEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdInsertDebugUtilsLabelEXT', dispatch='VkCommandBuffer'),
    Command(name='CreateDebugUtilsMessengerEXT', dispatch='VkInstance'),
    Command(name='DestroyDebugUtilsMessengerEXT', dispatch='VkInstance'),
    Command(name='SubmitDebugUtilsMessageEXT', dispatch='VkInstance'),
])

vk_ext_sampler_filter_minmax_2 = Extension(name='VK_EXT_sampler_filter_minmax', version=2, guard=None, commands=[
])

vk_amd_gpu_shader_int16_2 = Extension(name='VK_AMD_gpu_shader_int16', version=2, guard=None, commands=[
])

vk_amd_mixed_attachment_samples_1 = Extension(name='VK_AMD_mixed_attachment_samples', version=1, guard=None, commands=[
])

vk_amd_shader_fragment_mask_1 = Extension(name='VK_AMD_shader_fragment_mask', version=1, guard=None, commands=[
])

vk_ext_inline_uniform_block_1 = Extension(name='VK_EXT_inline_uniform_block', version=1, guard=None, commands=[
])

vk_ext_shader_stencil_export_1 = Extension(name='VK_EXT_shader_stencil_export', version=1, guard=None, commands=[
])

vk_ext_sample_locations_1 = Extension(name='VK_EXT_sample_locations', version=1, guard=None, commands=[
    Command(name='CmdSetSampleLocationsEXT', dispatch='VkCommandBuffer'),
    Command(name='GetPhysicalDeviceMultisamplePropertiesEXT', dispatch='VkPhysicalDevice'),
])

vk_ext_blend_operation_advanced_2 = Extension(name='VK_EXT_blend_operation_advanced', version=2, guard=None, commands=[
])

vk_nv_fragment_coverage_to_color_1 = Extension(name='VK_NV_fragment_coverage_to_color', version=1, guard=None, commands=[
])

vk_nv_framebuffer_mixed_samples_1 = Extension(name='VK_NV_framebuffer_mixed_samples', version=1, guard=None, commands=[
])

vk_nv_fill_rectangle_1 = Extension(name='VK_NV_fill_rectangle', version=1, guard=None, commands=[
])

vk_nv_shader_sm_builtins_1 = Extension(name='VK_NV_shader_sm_builtins', version=1, guard=None, commands=[
])

vk_ext_post_depth_coverage_1 = Extension(name='VK_EXT_post_depth_coverage', version=1, guard=None, commands=[
])

vk_ext_image_drm_format_modifier_1 = Extension(name='VK_EXT_image_drm_format_modifier', version=1, guard=None, commands=[
    Command(name='GetImageDrmFormatModifierPropertiesEXT', dispatch='VkDevice'),
])

vk_ext_validation_cache_1 = Extension(name='VK_EXT_validation_cache', version=1, guard=None, commands=[
    Command(name='CreateValidationCacheEXT', dispatch='VkDevice'),
    Command(name='DestroyValidationCacheEXT', dispatch='VkDevice'),
    Command(name='MergeValidationCachesEXT', dispatch='VkDevice'),
    Command(name='GetValidationCacheDataEXT', dispatch='VkDevice'),
])

vk_ext_descriptor_indexing_2 = Extension(name='VK_EXT_descriptor_indexing', version=2, guard=None, commands=[
])

vk_ext_shader_viewport_index_layer_1 = Extension(name='VK_EXT_shader_viewport_index_layer', version=1, guard=None, commands=[
])

vk_nv_shading_rate_image_3 = Extension(name='VK_NV_shading_rate_image', version=3, guard=None, commands=[
    Command(name='CmdBindShadingRateImageNV', dispatch='VkCommandBuffer'),
    Command(name='CmdSetViewportShadingRatePaletteNV', dispatch='VkCommandBuffer'),
    Command(name='CmdSetCoarseSampleOrderNV', dispatch='VkCommandBuffer'),
])

vk_nv_ray_tracing_3 = Extension(name='VK_NV_ray_tracing', version=3, guard=None, commands=[
    Command(name='CreateAccelerationStructureNV', dispatch='VkDevice'),
    Command(name='DestroyAccelerationStructureNV', dispatch='VkDevice'),
    Command(name='GetAccelerationStructureMemoryRequirementsNV', dispatch='VkDevice'),
    Command(name='BindAccelerationStructureMemoryNV', dispatch='VkDevice'),
    Command(name='CmdBuildAccelerationStructureNV', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyAccelerationStructureNV', dispatch='VkCommandBuffer'),
    Command(name='CmdTraceRaysNV', dispatch='VkCommandBuffer'),
    Command(name='CreateRayTracingPipelinesNV', dispatch='VkDevice'),
    Command(name='GetRayTracingShaderGroupHandlesKHR', dispatch='VkDevice'),
    Command(name='GetRayTracingShaderGroupHandlesNV', dispatch='VkDevice'),
    Command(name='GetAccelerationStructureHandleNV', dispatch='VkDevice'),
    Command(name='CmdWriteAccelerationStructuresPropertiesNV', dispatch='VkCommandBuffer'),
    Command(name='CompileDeferredNV', dispatch='VkDevice'),
])

vk_nv_representative_fragment_test_2 = Extension(name='VK_NV_representative_fragment_test', version=2, guard=None, commands=[
])

vk_ext_filter_cubic_3 = Extension(name='VK_EXT_filter_cubic', version=3, guard=None, commands=[
])

vk_qcom_render_pass_shader_resolve_4 = Extension(name='VK_QCOM_render_pass_shader_resolve', version=4, guard=None, commands=[
])

vk_ext_global_priority_2 = Extension(name='VK_EXT_global_priority', version=2, guard=None, commands=[
])

vk_ext_external_memory_host_1 = Extension(name='VK_EXT_external_memory_host', version=1, guard=None, commands=[
    Command(name='GetMemoryHostPointerPropertiesEXT', dispatch='VkDevice'),
])

vk_amd_buffer_marker_1 = Extension(name='VK_AMD_buffer_marker', version=1, guard=None, commands=[
    Command(name='CmdWriteBufferMarkerAMD', dispatch='VkCommandBuffer'),
])

vk_amd_pipeline_compiler_control_1 = Extension(name='VK_AMD_pipeline_compiler_control', version=1, guard=None, commands=[
])

vk_ext_calibrated_timestamps_1 = Extension(name='VK_EXT_calibrated_timestamps', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceCalibrateableTimeDomainsEXT', dispatch='VkPhysicalDevice'),
    Command(name='GetCalibratedTimestampsEXT', dispatch='VkDevice'),
])

vk_amd_shader_core_properties_2 = Extension(name='VK_AMD_shader_core_properties', version=2, guard=None, commands=[
])

vk_amd_memory_overallocation_behavior_1 = Extension(name='VK_AMD_memory_overallocation_behavior', version=1, guard=None, commands=[
])

vk_ext_vertex_attribute_divisor_3 = Extension(name='VK_EXT_vertex_attribute_divisor', version=3, guard=None, commands=[
])

vk_ext_pipeline_creation_feedback_1 = Extension(name='VK_EXT_pipeline_creation_feedback', version=1, guard=None, commands=[
])

vk_nv_shader_subgroup_partitioned_1 = Extension(name='VK_NV_shader_subgroup_partitioned', version=1, guard=None, commands=[
])

vk_nv_compute_shader_derivatives_1 = Extension(name='VK_NV_compute_shader_derivatives', version=1, guard=None, commands=[
])

vk_nv_mesh_shader_1 = Extension(name='VK_NV_mesh_shader', version=1, guard=None, commands=[
    Command(name='CmdDrawMeshTasksNV', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawMeshTasksIndirectNV', dispatch='VkCommandBuffer'),
    Command(name='CmdDrawMeshTasksIndirectCountNV', dispatch='VkCommandBuffer'),
])

vk_nv_fragment_shader_barycentric_1 = Extension(name='VK_NV_fragment_shader_barycentric', version=1, guard=None, commands=[
])

vk_nv_shader_image_footprint_2 = Extension(name='VK_NV_shader_image_footprint', version=2, guard=None, commands=[
])

vk_nv_scissor_exclusive_1 = Extension(name='VK_NV_scissor_exclusive', version=1, guard=None, commands=[
    Command(name='CmdSetExclusiveScissorNV', dispatch='VkCommandBuffer'),
])

vk_nv_device_diagnostic_checkpoints_2 = Extension(name='VK_NV_device_diagnostic_checkpoints', version=2, guard=None, commands=[
    Command(name='CmdSetCheckpointNV', dispatch='VkCommandBuffer'),
    Command(name='GetQueueCheckpointDataNV', dispatch='VkQueue'),
])

vk_intel_shader_integer_functions2_1 = Extension(name='VK_INTEL_shader_integer_functions2', version=1, guard=None, commands=[
])

vk_intel_performance_query_2 = Extension(name='VK_INTEL_performance_query', version=2, guard=None, commands=[
    Command(name='InitializePerformanceApiINTEL', dispatch='VkDevice'),
    Command(name='UninitializePerformanceApiINTEL', dispatch='VkDevice'),
    Command(name='CmdSetPerformanceMarkerINTEL', dispatch='VkCommandBuffer'),
    Command(name='CmdSetPerformanceStreamMarkerINTEL', dispatch='VkCommandBuffer'),
    Command(name='CmdSetPerformanceOverrideINTEL', dispatch='VkCommandBuffer'),
    Command(name='AcquirePerformanceConfigurationINTEL', dispatch='VkDevice'),
    Command(name='ReleasePerformanceConfigurationINTEL', dispatch='VkDevice'),
    Command(name='QueueSetPerformanceConfigurationINTEL', dispatch='VkQueue'),
    Command(name='GetPerformanceParameterINTEL', dispatch='VkDevice'),
])

vk_ext_pci_bus_info_2 = Extension(name='VK_EXT_pci_bus_info', version=2, guard=None, commands=[
])

vk_amd_display_native_hdr_1 = Extension(name='VK_AMD_display_native_hdr', version=1, guard=None, commands=[
    Command(name='SetLocalDimmingAMD', dispatch='VkDevice'),
])

vk_ext_fragment_density_map_1 = Extension(name='VK_EXT_fragment_density_map', version=1, guard=None, commands=[
])

vk_ext_scalar_block_layout_1 = Extension(name='VK_EXT_scalar_block_layout', version=1, guard=None, commands=[
])

vk_google_hlsl_functionality1_1 = Extension(name='VK_GOOGLE_hlsl_functionality1', version=1, guard=None, commands=[
])

vk_google_decorate_string_1 = Extension(name='VK_GOOGLE_decorate_string', version=1, guard=None, commands=[
])

vk_ext_subgroup_size_control_2 = Extension(name='VK_EXT_subgroup_size_control', version=2, guard=None, commands=[
])

vk_amd_shader_core_properties2_1 = Extension(name='VK_AMD_shader_core_properties2', version=1, guard=None, commands=[
])

vk_amd_device_coherent_memory_1 = Extension(name='VK_AMD_device_coherent_memory', version=1, guard=None, commands=[
])

vk_ext_shader_image_atomic_int64_1 = Extension(name='VK_EXT_shader_image_atomic_int64', version=1, guard=None, commands=[
])

vk_ext_memory_budget_1 = Extension(name='VK_EXT_memory_budget', version=1, guard=None, commands=[
])

vk_ext_memory_priority_1 = Extension(name='VK_EXT_memory_priority', version=1, guard=None, commands=[
])

vk_nv_dedicated_allocation_image_aliasing_1 = Extension(name='VK_NV_dedicated_allocation_image_aliasing', version=1, guard=None, commands=[
])

vk_ext_buffer_device_address_2 = Extension(name='VK_EXT_buffer_device_address', version=2, guard=None, commands=[
    Command(name='GetBufferDeviceAddressEXT', dispatch='VkDevice'),
])

vk_ext_tooling_info_1 = Extension(name='VK_EXT_tooling_info', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceToolPropertiesEXT', dispatch='VkPhysicalDevice'),
])

vk_ext_separate_stencil_usage_1 = Extension(name='VK_EXT_separate_stencil_usage', version=1, guard=None, commands=[
])

vk_ext_validation_features_4 = Extension(name='VK_EXT_validation_features', version=4, guard=None, commands=[
])

vk_nv_cooperative_matrix_1 = Extension(name='VK_NV_cooperative_matrix', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceCooperativeMatrixPropertiesNV', dispatch='VkPhysicalDevice'),
])

vk_nv_coverage_reduction_mode_1 = Extension(name='VK_NV_coverage_reduction_mode', version=1, guard=None, commands=[
    Command(name='GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV', dispatch='VkPhysicalDevice'),
])

vk_ext_fragment_shader_interlock_1 = Extension(name='VK_EXT_fragment_shader_interlock', version=1, guard=None, commands=[
])

vk_ext_ycbcr_image_arrays_1 = Extension(name='VK_EXT_ycbcr_image_arrays', version=1, guard=None, commands=[
])

vk_ext_headless_surface_1 = Extension(name='VK_EXT_headless_surface', version=1, guard=None, commands=[
    Command(name='CreateHeadlessSurfaceEXT', dispatch='VkInstance'),
])

vk_ext_line_rasterization_1 = Extension(name='VK_EXT_line_rasterization', version=1, guard=None, commands=[
    Command(name='CmdSetLineStippleEXT', dispatch='VkCommandBuffer'),
])

vk_ext_shader_atomic_float_1 = Extension(name='VK_EXT_shader_atomic_float', version=1, guard=None, commands=[
])

vk_ext_host_query_reset_1 = Extension(name='VK_EXT_host_query_reset', version=1, guard=None, commands=[
    Command(name='ResetQueryPoolEXT', dispatch='VkDevice'),
])

vk_ext_index_type_uint8_1 = Extension(name='VK_EXT_index_type_uint8', version=1, guard=None, commands=[
])

vk_ext_extended_dynamic_state_1 = Extension(name='VK_EXT_extended_dynamic_state', version=1, guard=None, commands=[
    Command(name='CmdSetCullModeEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetFrontFaceEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetPrimitiveTopologyEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetViewportWithCountEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetScissorWithCountEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdBindVertexBuffers2EXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetDepthTestEnableEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetDepthWriteEnableEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetDepthCompareOpEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetDepthBoundsTestEnableEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetStencilTestEnableEXT', dispatch='VkCommandBuffer'),
    Command(name='CmdSetStencilOpEXT', dispatch='VkCommandBuffer'),
])

vk_ext_shader_demote_to_helper_invocation_1 = Extension(name='VK_EXT_shader_demote_to_helper_invocation', version=1, guard=None, commands=[
])

vk_nv_device_generated_commands_3 = Extension(name='VK_NV_device_generated_commands', version=3, guard=None, commands=[
    Command(name='GetGeneratedCommandsMemoryRequirementsNV', dispatch='VkDevice'),
    Command(name='CmdPreprocessGeneratedCommandsNV', dispatch='VkCommandBuffer'),
    Command(name='CmdExecuteGeneratedCommandsNV', dispatch='VkCommandBuffer'),
    Command(name='CmdBindPipelineShaderGroupNV', dispatch='VkCommandBuffer'),
    Command(name='CreateIndirectCommandsLayoutNV', dispatch='VkDevice'),
    Command(name='DestroyIndirectCommandsLayoutNV', dispatch='VkDevice'),
])

vk_ext_texel_buffer_alignment_1 = Extension(name='VK_EXT_texel_buffer_alignment', version=1, guard=None, commands=[
])

vk_qcom_render_pass_transform_1 = Extension(name='VK_QCOM_render_pass_transform', version=1, guard=None, commands=[
])

vk_ext_device_memory_report_1 = Extension(name='VK_EXT_device_memory_report', version=1, guard=None, commands=[
])

vk_ext_robustness2_1 = Extension(name='VK_EXT_robustness2', version=1, guard=None, commands=[
])

vk_ext_custom_border_color_12 = Extension(name='VK_EXT_custom_border_color', version=12, guard=None, commands=[
])

vk_google_user_type_1 = Extension(name='VK_GOOGLE_user_type', version=1, guard=None, commands=[
])

vk_ext_private_data_1 = Extension(name='VK_EXT_private_data', version=1, guard=None, commands=[
    Command(name='CreatePrivateDataSlotEXT', dispatch='VkDevice'),
    Command(name='DestroyPrivateDataSlotEXT', dispatch='VkDevice'),
    Command(name='SetPrivateDataEXT', dispatch='VkDevice'),
    Command(name='GetPrivateDataEXT', dispatch='VkDevice'),
])

vk_ext_pipeline_creation_cache_control_3 = Extension(name='VK_EXT_pipeline_creation_cache_control', version=3, guard=None, commands=[
])

vk_nv_device_diagnostics_config_1 = Extension(name='VK_NV_device_diagnostics_config', version=1, guard=None, commands=[
])

vk_qcom_render_pass_store_ops_2 = Extension(name='VK_QCOM_render_pass_store_ops', version=2, guard=None, commands=[
])

vk_nv_fragment_shading_rate_enums_1 = Extension(name='VK_NV_fragment_shading_rate_enums', version=1, guard=None, commands=[
    Command(name='CmdSetFragmentShadingRateEnumNV', dispatch='VkCommandBuffer'),
])

vk_ext_fragment_density_map2_1 = Extension(name='VK_EXT_fragment_density_map2', version=1, guard=None, commands=[
])

vk_qcom_rotated_copy_commands_0 = Extension(name='VK_QCOM_rotated_copy_commands', version=0, guard=None, commands=[
])

vk_ext_image_robustness_1 = Extension(name='VK_EXT_image_robustness', version=1, guard=None, commands=[
])

vk_ext_4444_formats_1 = Extension(name='VK_EXT_4444_formats', version=1, guard=None, commands=[
])

vk_nv_acquire_winrt_display_1 = Extension(name='VK_NV_acquire_winrt_display', version=1, guard=None, commands=[
    Command(name='AcquireWinrtDisplayNV', dispatch='VkPhysicalDevice'),
    Command(name='GetWinrtDisplayNV', dispatch='VkPhysicalDevice'),
])

vk_valve_mutable_descriptor_type_1 = Extension(name='VK_VALVE_mutable_descriptor_type', version=1, guard=None, commands=[
])

vk_khr_acceleration_structure_11 = Extension(name='VK_KHR_acceleration_structure', version=11, guard=None, commands=[
    Command(name='CreateAccelerationStructureKHR', dispatch='VkDevice'),
    Command(name='DestroyAccelerationStructureKHR', dispatch='VkDevice'),
    Command(name='CmdBuildAccelerationStructuresKHR', dispatch='VkCommandBuffer'),
    Command(name='CmdBuildAccelerationStructuresIndirectKHR', dispatch='VkCommandBuffer'),
    Command(name='BuildAccelerationStructuresKHR', dispatch='VkDevice'),
    Command(name='CopyAccelerationStructureKHR', dispatch='VkDevice'),
    Command(name='CopyAccelerationStructureToMemoryKHR', dispatch='VkDevice'),
    Command(name='CopyMemoryToAccelerationStructureKHR', dispatch='VkDevice'),
    Command(name='WriteAccelerationStructuresPropertiesKHR', dispatch='VkDevice'),
    Command(name='CmdCopyAccelerationStructureKHR', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyAccelerationStructureToMemoryKHR', dispatch='VkCommandBuffer'),
    Command(name='CmdCopyMemoryToAccelerationStructureKHR', dispatch='VkCommandBuffer'),
    Command(name='GetAccelerationStructureDeviceAddressKHR', dispatch='VkDevice'),
    Command(name='CmdWriteAccelerationStructuresPropertiesKHR', dispatch='VkCommandBuffer'),
    Command(name='GetDeviceAccelerationStructureCompatibilityKHR', dispatch='VkDevice'),
    Command(name='GetAccelerationStructureBuildSizesKHR', dispatch='VkDevice'),
])

vk_khr_ray_tracing_pipeline_1 = Extension(name='VK_KHR_ray_tracing_pipeline', version=1, guard=None, commands=[
    Command(name='CmdTraceRaysKHR', dispatch='VkCommandBuffer'),
    Command(name='CreateRayTracingPipelinesKHR', dispatch='VkDevice'),
    Command(name='GetRayTracingCaptureReplayShaderGroupHandlesKHR', dispatch='VkDevice'),
    Command(name='CmdTraceRaysIndirectKHR', dispatch='VkCommandBuffer'),
    Command(name='GetRayTracingShaderGroupStackSizeKHR', dispatch='VkDevice'),
    Command(name='CmdSetRayTracingPipelineStackSizeKHR', dispatch='VkCommandBuffer'),
])

vk_khr_ray_query_1 = Extension(name='VK_KHR_ray_query', version=1, guard=None, commands=[
])

vk_khr_android_surface_6 = Extension(name='VK_KHR_android_surface', version=6, guard='VK_USE_PLATFORM_ANDROID_KHR', commands=[
    Command(name='CreateAndroidSurfaceKHR', dispatch='VkInstance'),
])

vk_android_external_memory_android_hardware_buffer_3 = Extension(name='VK_ANDROID_external_memory_android_hardware_buffer', version=3, guard='VK_USE_PLATFORM_ANDROID_KHR', commands=[
    Command(name='GetAndroidHardwareBufferPropertiesANDROID', dispatch='VkDevice'),
    Command(name='GetMemoryAndroidHardwareBufferANDROID', dispatch='VkDevice'),
])

vk_fuchsia_imagepipe_surface_1 = Extension(name='VK_FUCHSIA_imagepipe_surface', version=1, guard='VK_USE_PLATFORM_FUCHSIA', commands=[
    Command(name='CreateImagePipeSurfaceFUCHSIA', dispatch='VkInstance'),
])

vk_mvk_ios_surface_3 = Extension(name='VK_MVK_ios_surface', version=3, guard='VK_USE_PLATFORM_IOS_MVK', commands=[
    Command(name='CreateIOSSurfaceMVK', dispatch='VkInstance'),
])

vk_mvk_macos_surface_3 = Extension(name='VK_MVK_macos_surface', version=3, guard='VK_USE_PLATFORM_MACOS_MVK', commands=[
    Command(name='CreateMacOSSurfaceMVK', dispatch='VkInstance'),
])

vk_ext_metal_surface_1 = Extension(name='VK_EXT_metal_surface', version=1, guard='VK_USE_PLATFORM_METAL_EXT', commands=[
    Command(name='CreateMetalSurfaceEXT', dispatch='VkInstance'),
])

vk_nn_vi_surface_1 = Extension(name='VK_NN_vi_surface', version=1, guard='VK_USE_PLATFORM_VI_NN', commands=[
    Command(name='CreateViSurfaceNN', dispatch='VkInstance'),
])

vk_khr_wayland_surface_6 = Extension(name='VK_KHR_wayland_surface', version=6, guard='VK_USE_PLATFORM_WAYLAND_KHR', commands=[
    Command(name='CreateWaylandSurfaceKHR', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceWaylandPresentationSupportKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_win32_surface_6 = Extension(name='VK_KHR_win32_surface', version=6, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
    Command(name='CreateWin32SurfaceKHR', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceWin32PresentationSupportKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_external_memory_win32_1 = Extension(name='VK_KHR_external_memory_win32', version=1, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
    Command(name='GetMemoryWin32HandleKHR', dispatch='VkDevice'),
    Command(name='GetMemoryWin32HandlePropertiesKHR', dispatch='VkDevice'),
])

vk_khr_win32_keyed_mutex_1 = Extension(name='VK_KHR_win32_keyed_mutex', version=1, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
])

vk_khr_external_semaphore_win32_1 = Extension(name='VK_KHR_external_semaphore_win32', version=1, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
    Command(name='ImportSemaphoreWin32HandleKHR', dispatch='VkDevice'),
    Command(name='GetSemaphoreWin32HandleKHR', dispatch='VkDevice'),
])

vk_khr_external_fence_win32_1 = Extension(name='VK_KHR_external_fence_win32', version=1, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
    Command(name='ImportFenceWin32HandleKHR', dispatch='VkDevice'),
    Command(name='GetFenceWin32HandleKHR', dispatch='VkDevice'),
])

vk_nv_external_memory_win32_1 = Extension(name='VK_NV_external_memory_win32', version=1, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
    Command(name='GetMemoryWin32HandleNV', dispatch='VkDevice'),
])

vk_nv_win32_keyed_mutex_2 = Extension(name='VK_NV_win32_keyed_mutex', version=2, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
])

vk_ext_full_screen_exclusive_4 = Extension(name='VK_EXT_full_screen_exclusive', version=4, guard='VK_USE_PLATFORM_WIN32_KHR', commands=[
    Command(name='GetPhysicalDeviceSurfacePresentModes2EXT', dispatch='VkPhysicalDevice'),
    Command(name='AcquireFullScreenExclusiveModeEXT', dispatch='VkDevice'),
    Command(name='ReleaseFullScreenExclusiveModeEXT', dispatch='VkDevice'),
    Command(name='GetDeviceGroupSurfacePresentModes2EXT', dispatch='VkDevice'),
])

vk_khr_xcb_surface_6 = Extension(name='VK_KHR_xcb_surface', version=6, guard='VK_USE_PLATFORM_XCB_KHR', commands=[
    Command(name='CreateXcbSurfaceKHR', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceXcbPresentationSupportKHR', dispatch='VkPhysicalDevice'),
])

vk_khr_xlib_surface_6 = Extension(name='VK_KHR_xlib_surface', version=6, guard='VK_USE_PLATFORM_XLIB_KHR', commands=[
    Command(name='CreateXlibSurfaceKHR', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceXlibPresentationSupportKHR', dispatch='VkPhysicalDevice'),
])

vk_ext_directfb_surface_1 = Extension(name='VK_EXT_directfb_surface', version=1, guard='VK_USE_PLATFORM_DIRECTFB_EXT', commands=[
    Command(name='CreateDirectFBSurfaceEXT', dispatch='VkInstance'),
    Command(name='GetPhysicalDeviceDirectFBPresentationSupportEXT', dispatch='VkPhysicalDevice'),
])

vk_ext_acquire_xlib_display_1 = Extension(name='VK_EXT_acquire_xlib_display', version=1, guard='VK_USE_PLATFORM_XLIB_XRANDR_EXT', commands=[
    Command(name='AcquireXlibDisplayEXT', dispatch='VkPhysicalDevice'),
    Command(name='GetRandROutputDisplayEXT', dispatch='VkPhysicalDevice'),
])

vk_ggp_stream_descriptor_surface_1 = Extension(name='VK_GGP_stream_descriptor_surface', version=1, guard='VK_USE_PLATFORM_GGP', commands=[
    Command(name='CreateStreamDescriptorSurfaceGGP', dispatch='VkInstance'),
])

vk_ggp_frame_token_1 = Extension(name='VK_GGP_frame_token', version=1, guard='VK_USE_PLATFORM_GGP', commands=[
])

vk_khr_portability_subset_1 = Extension(name='VK_KHR_portability_subset', version=1, guard='VK_USE_PLATFORM_GGP', commands=[
])

extensions = [
    vk_core_0,
    vk_core_1,
    vk_core_2,
    vk_khr_surface_25,
    vk_khr_swapchain_70,
    vk_khr_display_23,
    vk_khr_display_swapchain_10,
    vk_khr_sampler_mirror_clamp_to_edge_3,
    vk_khr_multiview_1,
    vk_khr_get_physical_device_properties2_2,
    vk_khr_device_group_4,
    vk_khr_shader_draw_parameters_1,
    vk_khr_maintenance1_2,
    vk_khr_device_group_creation_1,
    vk_khr_external_memory_capabilities_1,
    vk_khr_external_memory_1,
    vk_khr_external_memory_fd_1,
    vk_khr_external_semaphore_capabilities_1,
    vk_khr_external_semaphore_1,
    vk_khr_external_semaphore_fd_1,
    vk_khr_push_descriptor_2,
    vk_khr_shader_float16_int8_1,
    vk_khr_16bit_storage_1,
    vk_khr_incremental_present_1,
    vk_khr_descriptor_update_template_1,
    vk_khr_imageless_framebuffer_1,
    vk_khr_create_renderpass2_1,
    vk_khr_shared_presentable_image_1,
    vk_khr_external_fence_capabilities_1,
    vk_khr_external_fence_1,
    vk_khr_external_fence_fd_1,
    vk_khr_performance_query_1,
    vk_khr_maintenance2_1,
    vk_khr_get_surface_capabilities2_1,
    vk_khr_variable_pointers_1,
    vk_khr_get_display_properties2_1,
    vk_khr_dedicated_allocation_3,
    vk_khr_storage_buffer_storage_class_1,
    vk_khr_relaxed_block_layout_1,
    vk_khr_get_memory_requirements2_1,
    vk_khr_image_format_list_1,
    vk_khr_sampler_ycbcr_conversion_14,
    vk_khr_bind_memory2_1,
    vk_khr_maintenance3_1,
    vk_khr_draw_indirect_count_1,
    vk_khr_shader_subgroup_extended_types_1,
    vk_khr_8bit_storage_1,
    vk_khr_shader_atomic_int64_1,
    vk_khr_shader_clock_1,
    vk_khr_driver_properties_1,
    vk_khr_shader_float_controls_4,
    vk_khr_depth_stencil_resolve_1,
    vk_khr_swapchain_mutable_format_1,
    vk_khr_timeline_semaphore_2,
    vk_khr_vulkan_memory_model_3,
    vk_khr_shader_terminate_invocation_1,
    vk_khr_fragment_shading_rate_1,
    vk_khr_spirv_1_4_1,
    vk_khr_surface_protected_capabilities_1,
    vk_khr_separate_depth_stencil_layouts_1,
    vk_khr_uniform_buffer_standard_layout_1,
    vk_khr_buffer_device_address_1,
    vk_khr_deferred_host_operations_4,
    vk_khr_pipeline_executable_properties_1,
    vk_khr_pipeline_library_1,
    vk_khr_shader_non_semantic_info_1,
    vk_khr_copy_commands2_1,
    vk_ext_debug_report_9,
    vk_nv_glsl_shader_1,
    vk_ext_depth_range_unrestricted_1,
    vk_img_filter_cubic_1,
    vk_amd_rasterization_order_1,
    vk_amd_shader_trinary_minmax_1,
    vk_amd_shader_explicit_vertex_parameter_1,
    vk_ext_debug_marker_4,
    vk_amd_gcn_shader_1,
    vk_nv_dedicated_allocation_1,
    vk_ext_transform_feedback_1,
    vk_nvx_image_view_handle_2,
    vk_amd_draw_indirect_count_2,
    vk_amd_negative_viewport_height_1,
    vk_amd_gpu_shader_half_float_2,
    vk_amd_shader_ballot_1,
    vk_amd_texture_gather_bias_lod_1,
    vk_amd_shader_info_1,
    vk_amd_shader_image_load_store_lod_1,
    vk_nv_corner_sampled_image_2,
    vk_img_format_pvrtc_1,
    vk_nv_external_memory_capabilities_1,
    vk_nv_external_memory_1,
    vk_ext_validation_flags_2,
    vk_ext_shader_subgroup_ballot_1,
    vk_ext_shader_subgroup_vote_1,
    vk_ext_texture_compression_astc_hdr_1,
    vk_ext_astc_decode_mode_1,
    vk_ext_conditional_rendering_2,
    vk_nv_clip_space_w_scaling_1,
    vk_ext_direct_mode_display_1,
    vk_ext_display_surface_counter_1,
    vk_ext_display_control_1,
    vk_google_display_timing_1,
    vk_nv_sample_mask_override_coverage_1,
    vk_nv_geometry_shader_passthrough_1,
    vk_nv_viewport_array2_1,
    vk_nvx_multiview_per_view_attributes_1,
    vk_nv_viewport_swizzle_1,
    vk_ext_discard_rectangles_1,
    vk_ext_conservative_rasterization_1,
    vk_ext_depth_clip_enable_1,
    vk_ext_swapchain_colorspace_4,
    vk_ext_hdr_metadata_2,
    vk_ext_external_memory_dma_buf_1,
    vk_ext_queue_family_foreign_1,
    vk_ext_debug_utils_2,
    vk_ext_sampler_filter_minmax_2,
    vk_amd_gpu_shader_int16_2,
    vk_amd_mixed_attachment_samples_1,
    vk_amd_shader_fragment_mask_1,
    vk_ext_inline_uniform_block_1,
    vk_ext_shader_stencil_export_1,
    vk_ext_sample_locations_1,
    vk_ext_blend_operation_advanced_2,
    vk_nv_fragment_coverage_to_color_1,
    vk_nv_framebuffer_mixed_samples_1,
    vk_nv_fill_rectangle_1,
    vk_nv_shader_sm_builtins_1,
    vk_ext_post_depth_coverage_1,
    vk_ext_image_drm_format_modifier_1,
    vk_ext_validation_cache_1,
    vk_ext_descriptor_indexing_2,
    vk_ext_shader_viewport_index_layer_1,
    vk_nv_shading_rate_image_3,
    vk_nv_ray_tracing_3,
    vk_nv_representative_fragment_test_2,
    vk_ext_filter_cubic_3,
    vk_qcom_render_pass_shader_resolve_4,
    vk_ext_global_priority_2,
    vk_ext_external_memory_host_1,
    vk_amd_buffer_marker_1,
    vk_amd_pipeline_compiler_control_1,
    vk_ext_calibrated_timestamps_1,
    vk_amd_shader_core_properties_2,
    vk_amd_memory_overallocation_behavior_1,
    vk_ext_vertex_attribute_divisor_3,
    vk_ext_pipeline_creation_feedback_1,
    vk_nv_shader_subgroup_partitioned_1,
    vk_nv_compute_shader_derivatives_1,
    vk_nv_mesh_shader_1,
    vk_nv_fragment_shader_barycentric_1,
    vk_nv_shader_image_footprint_2,
    vk_nv_scissor_exclusive_1,
    vk_nv_device_diagnostic_checkpoints_2,
    vk_intel_shader_integer_functions2_1,
    vk_intel_performance_query_2,
    vk_ext_pci_bus_info_2,
    vk_amd_display_native_hdr_1,
    vk_ext_fragment_density_map_1,
    vk_ext_scalar_block_layout_1,
    vk_google_hlsl_functionality1_1,
    vk_google_decorate_string_1,
    vk_ext_subgroup_size_control_2,
    vk_amd_shader_core_properties2_1,
    vk_amd_device_coherent_memory_1,
    vk_ext_shader_image_atomic_int64_1,
    vk_ext_memory_budget_1,
    vk_ext_memory_priority_1,
    vk_nv_dedicated_allocation_image_aliasing_1,
    vk_ext_buffer_device_address_2,
    vk_ext_tooling_info_1,
    vk_ext_separate_stencil_usage_1,
    vk_ext_validation_features_4,
    vk_nv_cooperative_matrix_1,
    vk_nv_coverage_reduction_mode_1,
    vk_ext_fragment_shader_interlock_1,
    vk_ext_ycbcr_image_arrays_1,
    vk_ext_headless_surface_1,
    vk_ext_line_rasterization_1,
    vk_ext_shader_atomic_float_1,
    vk_ext_host_query_reset_1,
    vk_ext_index_type_uint8_1,
    vk_ext_extended_dynamic_state_1,
    vk_ext_shader_demote_to_helper_invocation_1,
    vk_nv_device_generated_commands_3,
    vk_ext_texel_buffer_alignment_1,
    vk_qcom_render_pass_transform_1,
    vk_ext_device_memory_report_1,
    vk_ext_robustness2_1,
    vk_ext_custom_border_color_12,
    vk_google_user_type_1,
    vk_ext_private_data_1,
    vk_ext_pipeline_creation_cache_control_3,
    vk_nv_device_diagnostics_config_1,
    vk_qcom_render_pass_store_ops_2,
    vk_nv_fragment_shading_rate_enums_1,
    vk_ext_fragment_density_map2_1,
    vk_qcom_rotated_copy_commands_0,
    vk_ext_image_robustness_1,
    vk_ext_4444_formats_1,
    vk_nv_acquire_winrt_display_1,
    vk_valve_mutable_descriptor_type_1,
    vk_khr_acceleration_structure_11,
    vk_khr_ray_tracing_pipeline_1,
    vk_khr_ray_query_1,
    vk_khr_android_surface_6,
    vk_android_external_memory_android_hardware_buffer_3,
    vk_fuchsia_imagepipe_surface_1,
    vk_mvk_ios_surface_3,
    vk_mvk_macos_surface_3,
    vk_ext_metal_surface_1,
    vk_nn_vi_surface_1,
    vk_khr_wayland_surface_6,
    vk_khr_win32_surface_6,
    vk_khr_external_memory_win32_1,
    vk_khr_win32_keyed_mutex_1,
    vk_khr_external_semaphore_win32_1,
    vk_khr_external_fence_win32_1,
    vk_nv_external_memory_win32_1,
    vk_nv_win32_keyed_mutex_2,
    vk_ext_full_screen_exclusive_4,
    vk_khr_xcb_surface_6,
    vk_khr_xlib_surface_6,
    vk_ext_directfb_surface_1,
    vk_ext_acquire_xlib_display_1,
    vk_ggp_stream_descriptor_surface_1,
    vk_ggp_frame_token_1,
    vk_khr_portability_subset_1,
]
