//
//  RNMetalStateCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALSTATECOORDINATOR_H_
#define __RAYNE_METALSTATECOORDINATOR_H_

#include <Rayne.h>
#import <Metal/Metal.h>

namespace RN
{
	RNExceptionType(MetalStructArgumentUnsupported)

	class MetalRenderingStateUniformBufferMember
	{
	public:
		MetalRenderingStateUniformBufferMember(MTLStructMember *member)
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
					throw MetalStructArgumentUnsupportedException("Unknown argument");
			}
		}

		~MetalRenderingStateUniformBufferMember()
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

	struct MetalRenderingStateArgument
	{
		enum class Type
		{
			Unknown,
			Buffer,
			Texture,
			Sampler
		};

		MetalRenderingStateArgument(MTLArgument *argument) :
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
		}

		virtual ~MetalRenderingStateArgument()
		{}

		size_t index;
		Type type;
	};

	struct MetalRenderingStateUniformBufferArgument : public MetalRenderingStateArgument
	{
		MetalRenderingStateUniformBufferArgument(MTLArgument *argument) :
			MetalRenderingStateArgument(argument),
			size([argument bufferDataSize])
		{
			MTLStructType *structType = [argument bufferStructType];

			for(MTLStructMember *member in [structType members])
			{
				members.emplace_back(new MetalRenderingStateUniformBufferMember(member));
			}
		}

		~MetalRenderingStateUniformBufferArgument()
		{
			for(auto member : members)
				delete member;
		}

		std::vector<MetalRenderingStateUniformBufferMember *> members;
		size_t size;
	};


	struct MetalDepthStencilState
	{
		MetalDepthStencilState() = default;
		MetalDepthStencilState(Material *material, id<MTLDepthStencilState> tstate) :
			mode(material->GetDepthMode()),
			depthWriteEnabled(material->GetDepthWriteEnabled()),
			state(tstate)
		{}

		~MetalDepthStencilState()
		{
			[state release];
		}

		Material::DepthMode mode;
		bool depthWriteEnabled;
		id<MTLDepthStencilState> state;

		RN_INLINE bool MatchesMaterial(Material *material) const
		{
			return (material->GetDepthMode() == mode && material->GetDepthWriteEnabled() == depthWriteEnabled);
		}
	};

	struct MetalRenderingState
	{
		~MetalRenderingState()
		{
			for(MetalRenderingStateArgument *argument : vertexArguments)
				delete argument;
			for(MetalRenderingStateArgument *argument : fragmentArguments)
				delete argument;

			[state release];
		}

		MTLPixelFormat pixelFormat;
		MTLPixelFormat depthFormat;
		MTLPixelFormat stencilFormat;
		id<MTLRenderPipelineState> state;

		std::vector<MetalRenderingStateArgument *> vertexArguments;
		std::vector<MetalRenderingStateArgument *> fragmentArguments;
	};

	struct MetalRenderingStateCollection
	{
		MetalRenderingStateCollection() = default;
		MetalRenderingStateCollection(const Mesh::VertexDescriptor &tdescriptor, id<MTLFunction> vertex, id<MTLFunction> fragment) :
			descriptor(tdescriptor),
			vertexShader(vertex),
			fragmentShader(fragment)
		{}

		~MetalRenderingStateCollection()
		{
			for(MetalRenderingState *state : states)
				delete state;
		}

		Mesh::VertexDescriptor descriptor;
		id<MTLFunction> vertexShader;
		id<MTLFunction> fragmentShader;

		std::vector<MetalRenderingState *> states;
	};


	class MetalStateCoordinator
	{
	public:
		MetalStateCoordinator();
		~MetalStateCoordinator();

		void SetDevice(id<MTLDevice> device);

		id<MTLDepthStencilState> GetDepthStencilStateForMaterial(Material *material);
		id<MTLSamplerState> GetSamplerStateForTextureParameter(const Texture::Parameter &parameter);

		const MetalRenderingState *GetRenderPipelineState(Material *material, Mesh *mesh, Camera *camera);

	private:
		MTLVertexDescriptor *CreateVertexDescriptorFromMesh(Mesh *mesh);
		const MetalRenderingState *GetRenderPipelineStateInCollection(MetalRenderingStateCollection *collection, Mesh *mesh, Camera *camera);

		id<MTLDevice> _device;

		std::mutex _samplerLock;
		std::vector<std::pair<id<MTLSamplerState>, Texture::Parameter>> _samplers;

		std::vector<MetalDepthStencilState *> _depthStencilStates;
		const MetalDepthStencilState *_lastDepthStencilState;

		std::vector<MetalRenderingStateCollection *> _renderingStates;
	};
}


#endif /* __RAYNE_METALSTATECOORDINATOR_H_ */
