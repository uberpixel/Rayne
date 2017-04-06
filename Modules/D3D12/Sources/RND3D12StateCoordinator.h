//
//  RND3D12StateCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12STATECOORDINATOR_H_
#define __RAYNE_D3D12STATECOORDINATOR_H_

#include "RND3D12.h"
#include "RND3D12Shader.h"

struct D3D12_INPUT_LAYOUT_DESC;
struct ID3D12PipelineState;

namespace RN
{
	RNExceptionType(D3D12StructArgumentUnsupported)

	class D3D12UniformBuffer;

	struct D3D12UniformState
	{
		//VkDescriptorSet descriptorSet;
		D3D12UniformBuffer *uniformBuffer;
	};

	struct D3D12DepthStencilState
	{
		D3D12DepthStencilState() = default;
/*		D3D12DepthStencilState(Material *material, id<MTLDepthStencilState> tstate) :
			mode(material->GetDepthMode()),
			depthWriteEnabled(material->GetDepthWriteEnabled()),
			state(tstate)
		{}*/

		~D3D12DepthStencilState()
		{
//			[state release];
		}

		DepthMode mode;
		bool depthWriteEnabled;
//		id<MTLDepthStencilState> state;

		RN_INLINE bool MatchesMaterial(Material *material) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled);
		}
	};

	struct D3D12PipelineState
	{
		~D3D12PipelineState();

		size_t pixelFormat;
		size_t depthStencilFormat;
		ID3D12PipelineState *state;
	};

	struct D3D12RenderingStateCollection
	{
		D3D12RenderingStateCollection() = default;
		D3D12RenderingStateCollection(const Mesh::VertexDescriptor &tdescriptor, void *vertex, void *fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex),
			fragmentShader(fragment)
		{}

		~D3D12RenderingStateCollection()
		{
			for(D3D12PipelineState *state : states)
				delete state;
		}

		Mesh::VertexDescriptor descriptor;
		void *vertexShader;
		void *fragmentShader;

		std::vector<D3D12PipelineState *> states;
	};

	class D3D12StateCoordinator
	{
	public:
		D3D12StateCoordinator();
		~D3D12StateCoordinator();

/*		id<MTLDepthStencilState> GetDepthStencilStateForMaterial(Material *material);
		id<MTLSamplerState> GetSamplerStateForTextureParameter(const Texture::Parameter &parameter);*/

		const D3D12PipelineState *GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera);
		D3D12UniformState *GetUniformStateForPipelineState(const D3D12PipelineState *pipelineState, Material *material);

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> CreateVertexElementDescriptorsFromMesh(Mesh *mesh);
		const D3D12PipelineState *GetRenderPipelineStateInCollection(D3D12RenderingStateCollection *collection, Mesh *mesh, Camera *camera);

		std::mutex _samplerLock;
//		std::vector<std::pair<id<MTLSamplerState>, Texture::Parameter>> _samplers;

		std::vector<D3D12DepthStencilState *> _depthStencilStates;
		const D3D12DepthStencilState *_lastDepthStencilState;

		std::vector<D3D12RenderingStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_D3D12STATECOORDINATOR_H_ */
