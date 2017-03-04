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

	class D3D12RenderingStateUniformBufferMember
	{
	public:
/*		D3D12RenderingStateUniformBufferMember(MTLStructMember *member)
		{
			_name = RNSTR([[member name] UTF8String])->Retain();
			_offset = [member offset];

			switch([member dataType])
			{
				case MTLDataTypeUChar:
					_type = PrimitiveType::Uint8;
					break;
				case MTLDataTypeUShort:
					_type = PrimitiveType::Uint16;
					break;
				case MTLDataTypeUInt:
					_type = PrimitiveType::Uint32;
					break;
				case MTLDataTypeChar:
					_type = PrimitiveType::Int8;
					break;
				case MTLDataTypeShort:
					_type = PrimitiveType::Int16;
					break;
				case MTLDataTypeInt:
					_type = PrimitiveType::Int32;
					break;
				case MTLDataTypeFloat:
					_type = PrimitiveType::Float;
					break;
				case MTLDataTypeFloat4x4:
					_type = PrimitiveType::Matrix;
					break;
				case MTLDataTypeFloat2:
					_type = PrimitiveType::Vector2;
					break;
				case MTLDataTypeFloat3:
					_type = PrimitiveType::Vector3;
					break;
				case MTLDataTypeFloat4:
					_type = PrimitiveType::Vector4;
					break;
				default:
					throw D3D12StructArgumentUnsupportedException("Unknown argument");
			}
		}*/

		~D3D12RenderingStateUniformBufferMember()
		{
			SafeRelease(_name);
		}

		const String *GetName() const { return _name; }
		size_t GetOffset() const { return _offset; }

	private:
		String *_name;
		PrimitiveType _type;
		size_t _offset;
	};

	struct D3D12RenderingStateArgument
	{
		enum class Type
		{
			Unknown,
			Buffer,
			Texture,
			Sampler
		};

/*		D3D12RenderingStateArgument(MTLArgument *argument) :
			index([argument index])
		{
			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
					type = Type::Buffer;
					break;
				case MTLArgumentTypeTexture:
					type = Type::Texture;
					break;
				case MTLArgumentTypeSampler:
					type = Type::Sampler;
					break;
				default:
					type = Type::Unknown;
					break;
			}
		}*/

		virtual ~D3D12RenderingStateArgument()
		{}

		size_t index;
		Type type;
	};

	struct D3D12RenderingStateUniformBufferArgument : public D3D12RenderingStateArgument
	{
/*		D3D12RenderingStateUniformBufferArgument(MTLArgument *argument) :
			D3D12RenderingStateArgument(argument),
			size([argument bufferDataSize])
		{
			MTLStructType *structType = [argument bufferStructType];

			for(MTLStructMember *member in [structType members])
			{
				members.emplace_back(new D3D12RenderingStateUniformBufferMember(member));
			}
		}*/

		~D3D12RenderingStateUniformBufferArgument()
		{
			for(auto member : members)
				delete member;
		}

		std::vector<D3D12RenderingStateUniformBufferMember *> members;
		size_t size;
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

		std::vector<D3D12RenderingStateArgument *> vertexArguments;
		std::vector<D3D12RenderingStateArgument *> fragmentArguments;
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
