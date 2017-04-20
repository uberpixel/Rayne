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
	class Mesh;

	class Shader : public Object
	{
	public:
		class Options : public Object
		{
		public:
			RNAPI static Options *WithMesh(Mesh *mesh);
			RNAPI static Options *WithNone();

			RNAPI void EnableDiscard();

			RNAPI void AddDefine(String *name, String *value);

			RNAPI bool IsEqual(const Object *other) const override;
			RNAPI size_t GetHash() const override;

			const Dictionary *GetDefines() const { return _defines; }
		private:
			RNAPI Options();
			RNAPI Options(Mesh *mesh);

			Dictionary *_defines;

			__RNDeclareMetaInternal(Options)
		};

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
				ProjectionMatrix,
				InverseModelMatrix,
				InverseModelViewMatrix,
				InverseModelViewProjectionMatrix,
				InverseViewMatrix,
				InverseViewProjectionMatrix,
				InverseProjectionMatrix,
				AmbientColor,
				DiffuseColor,
				SpecularColor,
				EmissiveColor,
				TextureTileFactor,
				DiscardThreshold,
				Time,
				CameraPosition,
				DirectionalLights,
				DirectionalLightsCount,
				DirectionalShadowMatrices,
				DirectionalShadowMatricesCount,
				PointLights,
				SpotLights
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

		class Sampler : public Object
		{
		public:
			enum class WrapMode
			{
				Clamp,
				Repeat
			};

			enum class Filter
			{
				Linear,
				Nearest,
				Anisotropic
			};

			enum class ComparisonFunction
			{
				Never,
				Less,
				LessEqual,
				Equal,
				NotEqual,
				GreaterEqual,
				Greater,
				Always
			};

			RNAPI Sampler(WrapMode wrapMode = WrapMode::Repeat, Filter filter = Filter::Anisotropic, ComparisonFunction comparisonFunction = ComparisonFunction::Never, uint8 anisotropy = GetDefaultAnisotropy());
			RNAPI ~Sampler();

			bool operator== (const Sampler &other) const
			{
				return (_filter == other._filter && _wrapMode == other._wrapMode && _anisotropy == other._anisotropy && _comparisonFunction == other._comparisonFunction);
			}

			RNAPI static uint32 GetDefaultAnisotropy();
			RNAPI static void SetDefaultAnisotropy(uint32 anisotropy);

			RNAPI WrapMode GetWrapMode() const { return _wrapMode; }
			RNAPI Filter GetFilter() const { return _filter; }
			RNAPI ComparisonFunction GetComparisonFunction() const { return _comparisonFunction; }
			RNAPI uint8 GetAnisotropy() const { return _anisotropy; }

		private:
			WrapMode _wrapMode;
			Filter _filter;
			ComparisonFunction _comparisonFunction;
			uint8 _anisotropy;

			__RNDeclareMetaInternal(Sampler)
		};

		class Signature : public Object
		{
		public:
			RNAPI Signature(Array *uniformDescriptors, Array *samplers, uint8 textureCount);
			RNAPI virtual ~Signature();

			RNAPI const Array *GetUniformDescriptors() const
			{
				return _uniformDescriptors;
			}

			RNAPI const Array *GetSamplers() const
			{
				return _samplers;
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
			Array *_samplers;
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

		enum Default
		{
			Gouraud,
			Sky,
			Depth
		};

		RNAPI virtual const String *GetName() const = 0;

		RNAPI Type GetType() const;
		RNAPI const Shader::Options *GetOptions() const;
		RNAPI const Signature *GetSignature() const;
		RNAPI ShaderLibrary *GetLibrary() const;

	protected:
		RNAPI Shader(ShaderLibrary *library, Type type, const Shader::Options *options, const Signature *signature);
		RNAPI Shader(ShaderLibrary *library, Type type, const Shader::Options *options);
		RNAPI virtual ~Shader();

		RNAPI void SetSignature(const Signature *signature);

	private:
		const Shader::Options *_options;
		WeakRef<ShaderLibrary> _library;
		Type _type;
		const Signature *_signature;

		__RNDeclareMetaInternal(Shader)
	};
}


#endif /* __RAYNE_SHADER_H_ */
