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

			RNAPI Options(const Options *options);
			RNAPI ~Options();

			RNAPI Options *EnableAlpha();
			RNAPI Options *EnablePointLights();
			RNAPI Options *EnableDirectionalLights();
			RNAPI Options *EnableDirectionalShadows();
			RNAPI Options *EnableMultiview();

			RNAPI Options *AddDefine(const String *name, const String *value);
			RNAPI Options *AddDefine(const char *name, const char *value);

			RNAPI bool IsEqual(const Object *other) const override;
			RNAPI size_t GetHash() const override;
			
			RNAPI bool HasValue(const char *key, const char *value);
			RNAPI const String *GetValue(const char *key) const;
			RNAPI size_t GetCount() const;
			RNAPI void Enumerate(const std::function<void (const std::string &value, const std::string &key, bool &stop)>& callback) const;

		private:
			RNAPI Options();
			RNAPI Options(Mesh *mesh);

			std::map<std::string, std::string> _defines;

			__RNDeclareMetaInternal(Options)
		};

		class UniformDescriptor : public Object
		{
		public:
			enum Identifier
			{
				Custom,
				ModelMatrix,
				InverseModelMatrix,
				
				NormalMatrix,

				ModelViewMatrix,
				ModelViewMatrixMultiview,
				ModelViewProjectionMatrix,
				ModelViewProjectionMatrixMultiview,
				ViewMatrix,
				ViewMatrixMultiview,
				ViewProjectionMatrix,
				ViewProjectionMatrixMultiview,
				ProjectionMatrix,
				ProjectionMatrixMultiview,

				InverseModelViewMatrix,
				InverseModelViewMatrixMultiview,
				InverseModelViewProjectionMatrix,
				InverseModelViewProjectionMatrixMultiview,
				InverseViewMatrix,
				InverseViewMatrixMultiview,
				InverseViewProjectionMatrix,
				InverseViewProjectionMatrixMultiview,
				InverseProjectionMatrix,
				InverseProjectionMatrixMultiview,

				CameraPosition,
				CameraPositionMultiview,
				CameraClipDistance,
				CameraFogDistance,

				AmbientColor,
				DiffuseColor,
				SpecularColor,
				EmissiveColor,
				TextureTileFactor,
				AlphaToCoverageClamp,
				Time,
				CameraAmbientColor,
				CameraFogColor0,
				CameraFogColor1,
				DirectionalLightsCount,
				DirectionalLights,
				DirectionalShadowMatrices,
				DirectionalShadowMatricesCount,
				DirectionalShadowInfo,
				PointLights,
				SpotLights,
				BoneMatrices
			};

			RNAPI UniformDescriptor(const String *name, PrimitiveType type, size_t offset, size_t elementCount, size_t location = -1);
			RNAPI virtual ~UniformDescriptor();

			const String *GetName() const { return _name; }
			PrimitiveType GetType() const { return _type; }
			size_t GetOffset() const { return _offset; }
			Identifier GetIdentifier() const { return _identifier; }
			RNAPI size_t GetSize() const;
			
			size_t GetElementCount() const { return _elementCount; }
            size_t GetAttributeLocation() const { return _location; }

			const String *GetDescription() const override { return RNSTR("<ShaderUniform: name: " << _name << ", type: " << (int)_type << ">"); }
			
			RNAPI static bool IsKnownStructName(RN::String *structName);

		private:
			String *_name;
			Identifier _identifier;
			PrimitiveType _type;
			size_t _offset;
			
			size_t _elementCount; //Usually 1, but higher if this an array type with more than one element
			size_t _location; //Used for per instance vertex attributes TODO: Move those into a separate descriptor type!?

			__RNDeclareMetaInternal(UniformDescriptor)
		};
		
		class Argument : public Object
		{
		public:
			String *GetName() const { return _name; }
			uint32 GetIndex() const { return _index; }
			void SetIndex(uint32 index) { _index = index; }
		
		protected:
			RNAPI Argument(String *name, uint32 index);
			RNAPI Argument(const Argument *other);
			RNAPI ~Argument();
			
			uint32 _index;
			String *_name;
			
			__RNDeclareMetaInternal(Argument)
		};
		
		class ArgumentBuffer : public Argument
		{
		public:
			enum class Type
			{
				UniformBuffer,
				StorageBuffer,
				InstanceAttributesBuffer
			};
			RNAPI ArgumentBuffer(String *name, uint32 index, Array *uniformDescriptors, Type type, size_t maxInstanceCount);
			RNAPI ArgumentBuffer(const ArgumentBuffer *other);
			RNAPI ~ArgumentBuffer();
			
			size_t GetTotalUniformSize() const { return _totalUniformSize; }
			const Array *GetUniformDescriptors() const { return _uniformDescriptors; }
			Type GetType() const { return _type; }
			
			size_t GetMaxInstanceCount() const { return _maxInstanceCount; }
			
		private:
			Array *_uniformDescriptors;
			size_t _totalUniformSize;
			Type _type;
			
			size_t _maxInstanceCount; //If this buffer contains per instance uniform data, it just contains an array of a struct, this is the number of elements of that array. 1 otherwise. 0 if this a storage buffer as they don't have any tight size limits and can be indexed more freely.
			
			__RNDeclareMetaInternal(ArgumentBuffer)
		};

		class ArgumentSampler : public Argument
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

			RNAPI ArgumentSampler(String *name, uint32 index, WrapMode wrapMode = WrapMode::Repeat, Filter filter = Filter::Anisotropic, ComparisonFunction comparisonFunction = ComparisonFunction::Never, uint8 anisotropy = GetDefaultAnisotropy());
			RNAPI ArgumentSampler(const ArgumentSampler *other);
			RNAPI ~ArgumentSampler();

			bool operator== (const ArgumentSampler &other) const
			{
				return (_filter == other._filter && _wrapMode == other._wrapMode && _anisotropy == other._anisotropy && _comparisonFunction == other._comparisonFunction);
			}

			RNAPI static uint32 GetDefaultAnisotropy();
			RNAPI static void SetDefaultAnisotropy(uint32 anisotropy);

			WrapMode GetWrapMode() const { return _wrapMode; }
			Filter GetFilter() const { return _filter; }
			ComparisonFunction GetComparisonFunction() const { return _comparisonFunction; }
			uint8 GetAnisotropy() const { return _anisotropy; }

		private:
			WrapMode _wrapMode;
			Filter _filter;
			ComparisonFunction _comparisonFunction;
			uint8 _anisotropy;

			__RNDeclareMetaInternal(ArgumentSampler)
		};
		
		class ArgumentTexture : public Argument
		{
		public:
			enum Index
			{
				IndexDirectionalShadowTexture = 255,
				IndexFramebufferTexture = 254
			};
			
			RNAPI ArgumentTexture(String *name, uint32 index, uint8 materialTextureIndex);
			RNAPI ArgumentTexture(const ArgumentTexture *other);
			RNAPI ~ArgumentTexture();
			
			uint8 GetMaterialTextureIndex() const { return _materialTextureIndex; }
			
		private:
			uint8 _materialTextureIndex;
			
			__RNDeclareMetaInternal(ArgumentTexture)
		};

		class Signature : public Object
		{
		public:
			RNAPI Signature(Array *buffers, Array *samplers, Array *textures);
			RNAPI virtual ~Signature();

			const Array *GetBuffers() const { return _buffers; }
			const Array *GetSamplers() const { return _samplers; }
			const Array *GetTextures() const { return _textures; }

		private:
			Array *_buffers;
			Array *_samplers;
			Array *_textures;

			__RNDeclareMetaInternal(Signature)
		};

		enum class Type
		{
			Fragment,
			Vertex,
			Compute
		};

		enum UsageHint
		{
			Default,
			Depth,
			Instancing,
			Multiview,
			DepthMultiview,

			COUNT
		};

		RNAPI virtual const String *GetName() const = 0;

		RNAPI Type GetType() const;
		RNAPI const Shader::Options *GetOptions() const;
		RNAPI const Signature *GetSignature() const;
		RNAPI ShaderLibrary *GetLibrary() const;

		RNAPI bool GetHasInstancing() const { return _hasInstancing; }
		size_t GetMaxInstanceCount() const { return _maxInstanceCount; }

	protected:
		RNAPI Shader(ShaderLibrary *library, Type type, bool hasInstancing, const Shader::Options *options, const Signature *signature);
		RNAPI Shader(ShaderLibrary *library, Type type, bool hasInstancing, const Shader::Options *options);
		RNAPI virtual ~Shader();

		RNAPI void SetSignature(const Signature *signature);

	private:
		const Shader::Options *_options;
		WeakRef<ShaderLibrary> _library;
		Type _type;
		bool _hasInstancing;
		const Signature *_signature;
		size_t _maxInstanceCount;

		__RNDeclareMetaInternal(Shader)
	};
}


#endif /* __RAYNE_SHADER_H_ */
