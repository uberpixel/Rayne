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
	class D3D12Framebuffer;

	struct D3D12UniformState
	{
		D3D12UniformBuffer *vertexUniformBuffer;
		D3D12UniformBuffer *fragmentUniformBuffer;
	};

	struct D3D12DepthStencilState
	{
		D3D12DepthStencilState() = default;

		~D3D12DepthStencilState()
		{

		}

		DepthMode mode;
		bool depthWriteEnabled;

		RN_INLINE bool MatchesMaterial(Material *material) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled);
		}
	};

	struct D3D12RootSignature
	{
		~D3D12RootSignature();

		uint8 textureCount;
		Array *samplers;
		uint8 constantBufferCount;

		bool wantsDirectionalShadowTexture; //TODO: Solve better...

		ID3D12RootSignature *signature;
	};

	struct D3D12PipelineStateDescriptor
	{
		Shader::UsageHint shaderHint;

		std::vector<DXGI_FORMAT> colorFormats;
		DXGI_FORMAT depthStencilFormat;
		uint8 sampleCount;
		uint8 sampleQuality;

		Shader *vertexShader;
		Shader *fragmentShader;

		CullMode cullMode;
		bool usePolygonOffset;
		float polygonOffsetFactor;
		float polygonOffsetUnits;

		bool useAlphaToCoverage;
	};

	struct D3D12PipelineState
	{
		~D3D12PipelineState();

		D3D12PipelineStateDescriptor descriptor;
		const D3D12RootSignature *rootSignature;

		ID3D12PipelineState *state;
	};

	struct D3D12PipelineStateCollection
	{
		D3D12PipelineStateCollection() = default;
		D3D12PipelineStateCollection(const Mesh::VertexDescriptor &tdescriptor, Shader *vertex, Shader *fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex),
			fragmentShader(fragment)
		{}

		~D3D12PipelineStateCollection()
		{
			for(D3D12PipelineState *state : states)
				delete state;
		}

		Mesh::VertexDescriptor descriptor;
		Shader *vertexShader;
		Shader *fragmentShader;

		std::vector<D3D12PipelineState *> states;
	};

	class D3D12StateCoordinator
	{
	public:
		D3D12StateCoordinator();
		~D3D12StateCoordinator();

		const D3D12RootSignature *GetRootSignature(const D3D12PipelineStateDescriptor &pipelineDescriptor);
		const D3D12PipelineState *GetRenderPipelineState(Material *material, Mesh *mesh, D3D12Framebuffer *framebuffer, Shader::UsageHint shaderHint, Material *overrideMaterial);
		D3D12UniformState *GetUniformStateForPipelineState(const D3D12PipelineState *pipelineState, Material *material);

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> CreateVertexElementDescriptorsFromMesh(Mesh *mesh);
		const D3D12PipelineState *GetRenderPipelineStateInCollection(D3D12PipelineStateCollection *collection, Mesh *mesh, const D3D12PipelineStateDescriptor &pipelineDescriptor);

		std::vector<D3D12DepthStencilState *> _depthStencilStates;
		const D3D12DepthStencilState *_lastDepthStencilState;

		std::vector<D3D12PipelineStateCollection *> _renderingStates;
		std::vector<D3D12RootSignature *> _rootSignatures;
	};
}


#endif /* __RAYNE_D3D12STATECOORDINATOR_H_ */
