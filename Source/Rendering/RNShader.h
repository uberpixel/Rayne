//
//  RNShader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SHADER_H_
#define __RAYNE_SHADER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "../Objects/RNArray.h"
#include "RNRendererTypes.h"

namespace RN
{
	class ShaderLibrary;
	class ShaderOptions;

	class Shader : public Object
	{
	public:
		class UniformDescriptor : public Object
		{
		public:
			enum Identifier
			{
				Custom,
				ModelMatrix,
				ModelViewMatrix,
				ModelViewProjectionMatrix,
				ViewMatrix,
				ViewProjectionMatrix,
				InverseModelMatrix,
				InverseModelViewMatrix,
				InverseModelViewProjectionMatrix,
				InverseViewMatrix,
				InverseViewProjectionMatrix,
				AmbientColor,
				DiffuseColor,
				SpecularColor,
				EmissiveColor,
				TextureTileFactor,
				DiscardThreshold
			};

			RNAPI UniformDescriptor(const String *name, PrimitiveType type, size_t offset);
			RNAPI UniformDescriptor(const String *name, size_t offset);
			RNAPI virtual ~UniformDescriptor();

			RNAPI const String *GetName() const { return _name; }
			RNAPI PrimitiveType GetType() const { return _type; }
			RNAPI size_t GetOffset() const { return _offset; }
			RNAPI size_t GetSize() const;
			RNAPI Identifier GetIdentifier() const { return _identifier; }

			RNAPI const String *GetDescription() const override { return RNSTR("<ShaderUniform: name: " << _name << ", type: " << (int)_type << ">"); }

		private:
			String *_name;
			Identifier _identifier;
			PrimitiveType _type;
			size_t _offset;

			__RNDeclareMetaInternal(UniformDescriptor)
		};

		class Signature : public Object
		{
		public:
			RNAPI Signature(Array *uniformDescriptors, uint8 samplerCount, uint8 textureCount);
			RNAPI virtual ~Signature();

			RNAPI const Array *GetUniformDescriptors() const
			{
				return _uniformDescriptors;
			}

			RNAPI uint8 GetSamplerCount() const
			{
				return _samplerCount;
			}

			RNAPI uint8 GetTextureCount() const
			{
				return _textureCount;
			}

			RNAPI size_t GetTotalUniformSize() const
			{
				return _totalUniformSize;
			}

		private:
			Array *_uniformDescriptors;
			uint8 _samplerCount;
			uint8 _textureCount;
			size_t _totalUniformSize;

			__RNDeclareMetaInternal(Signature)
		};


		enum class Type
		{
			Fragment,
			Vertex,
			Compute
		};

		RNAPI virtual const String *GetName() const = 0;

		RNAPI Type GetType() const;
		RNAPI const ShaderOptions *GetShaderOptions() const;
		RNAPI const Signature *GetSignature() const;
		RNAPI ShaderLibrary *GetLibrary() const;

	protected:
		RNAPI Shader(ShaderLibrary *library, Type type, const ShaderOptions *options, const Signature *signature);
		RNAPI virtual ~Shader();

	private:
		const ShaderOptions *_options;
		WeakRef<ShaderLibrary> _library;
		Type _type;
		const Signature *_signature;

		__RNDeclareMetaInternal(Shader)
	};
}


#endif /* __RAYNE_SHADER_H_ */
