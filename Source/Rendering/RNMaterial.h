//
//  RNMaterial.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_MATERIAL_H_
#define __RAYNE_MATERIAL_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNValue.h"
#include "../Objects/RNNumber.h"
#include "RNTexture.h"
#include "RNShader.h"
#include "RNShaderLibrary.h"

namespace RN
{
	enum class DepthMode
	{
		Never,
		Always,
		Less,
		LessOrEqual,
		Equal,
		NotEqual,
		GreaterOrEqual,
		Greater
	};
	
	enum class BlendFactor
	{
		Zero,
		One,
		SourceColor,
		OneMinusSourceColor,
		SourceAlpha,
		OneMinusSourceAlpha,
		DestinationColor,
		OneMinusDestinationColor,
		DestinationAlpha,
		OneMinusDestinationAlpha,
		SourceAlphaSaturated,
		BlendColor,
		OneMinusBlendColor,
		BlendAlpha,
		OneMinusBlendAlpha
	};
	
	enum class BlendOperation
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
		None
	};

	enum class CullMode
	{
		None,
		BackFace,
		FrontFace
	};

	class Material : public Object
	{
	public:
		
		class Properties
		{
		friend Material;
		public:
			RNAPI Properties();
			RNAPI Properties(const Properties &properties);
			RNAPI ~Properties();
			
			uint8 colorWriteMask;
			
			DepthMode depthMode;
			bool depthWriteEnabled;
			
			Color ambientColor;
			Color diffuseColor;
			Color specularColor;
			Color emissiveColor;
			
			//TODO: Hide these behind a define...
			//Used as an optimization for GRAB, will usually be a waste of memory though...
			Matrix customMatrix1;
			Matrix customMatrix2;
			
			//Used by the UI system to set additional shader data as with lots of changing views using custom uniforms gets slow
			Vector4 uiClippingRect;
			Vector2 uiOffset;
			
			//TODO: Changing anything but colors and custom uniforms should only be allowed on creation, but not after already used for rendering as the renderers will only switch the pipeline state if the material was changed and these properties require a new pipeline state
			bool usePolygonOffset;
			float polygonOffsetFactor;
			float polygonOffsetUnits;
			
			bool useAlphaToCoverage;
			Vector2 alphaToCoverageClamp;
			float textureTileFactor;
			CullMode cullMode;
			
			BlendOperation blendOperationRGB;
			BlendOperation blendOperationAlpha;
			BlendFactor blendFactorSourceRGB;
			BlendFactor blendFactorDestinationRGB;
			BlendFactor blendFactorSourceAlpha;
			BlendFactor blendFactorDestinationAlpha;
			
			RNAPI void CopyFromProperties(const Properties &properties);
			
			RNAPI void SetCustomShaderUniform(const String *name, Value *value);
			RNAPI void SetCustomShaderUniform(const String *name, Number *number);
			RNAPI Object *GetCustomShaderUniform(const String *name) const;
			RNAPI Object *GetCustomShaderUniform(size_t nameHash) const;
			
		private:
			std::map<const size_t, Object *> _customShaderUniforms; //WARNING: The key is the hash of the string, which could potentially have conflicts!
		};

		RN_OPTIONS(Override, uint32,
			GroupDepth = (1 << 0),
			GroupColors = (1 << 1),
			GroupPolygonOffset = (1 << 2),
			GroupShaders = (1 << 3),
			GroupAlphaToCoverage = (1 << 4),
			GroupBlending = (1 << 5),
			TextureTileFactor = (1 << 6),
			CullMode = (1 << 7),
		   	ColorWriteMask = (1 << 8),
			CustomUniforms = (1 << 9),

			DefaultDepth = (0xffffffff & ~(GroupPolygonOffset|ColorWriteMask|CustomUniforms))
		);

		RNAPI Material(Shader *vertexShader, Shader *fragmentShader);
		RNAPI Material(const Material *other);
		RNAPI ~Material() override;
		
		RNAPI static Material *WithShaders(Shader *vertexShader, Shader *fragmentShader);
		RNAPI static Material *WithMaterial(const Material *material);
		
		RNAPI void SetTextures(const Array *textures);
		RNAPI void AddTexture(Texture *texture);
		RNAPI void RemoveAllTextures();
		
		RNAPI void SetFragmentShader(Shader *shader, Shader::UsageHint type = Shader::UsageHint::Default);
		RNAPI void SetVertexShader(Shader *shader, Shader::UsageHint type = Shader::UsageHint::Default);
		
		RNAPI void SetOverride(Override override);
		
		RNAPI void SetColorWriteMask(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha);

		RNAPI void SetDepthWriteEnabled(bool depthWrite);
		RNAPI void SetDepthMode(DepthMode mode);
		
		RNAPI void SetBlendOperation(BlendOperation blendOperationRGB, BlendOperation blendOperationAlpha);
		RNAPI void SetBlendFactorSource(BlendFactor blendFactorRGB, BlendFactor blendFactorAlpha);
		RNAPI void SetBlendFactorDestination(BlendFactor blendFactorRGB, BlendFactor blendFactorAlpha);

		RNAPI void SetAmbientColor(const Color &color);
		RNAPI void SetDiffuseColor(const Color &color);
		RNAPI void SetSpecularColor(const Color &color);
		RNAPI void SetEmissiveColor(const Color &color);
		
		RNAPI void SetCustomMatrix1(const Matrix &matrix);
		RNAPI void SetCustomMatrix2(const Matrix &matrix);

		RNAPI void SetTextureTileFactor(float factor);
		RNAPI void SetCullMode(CullMode mode);

		RNAPI void SetFragmentBuffers(const Array *buffers);
		RNAPI void SetVertexBuffers(const Array *buffers);

		RNAPI void SetPolygonOffset(bool enable, float factor = 1.1f, float units = 0.1f);
		RNAPI void SetAlphaToCoverage(bool enabled, float min = 0.3, float max = 0.8);
		
		RNAPI void SetUIClippingRect(Vector4 rect);
		RNAPI void SetUIOffset(Vector2 offset);
		
		RNAPI void SetCustomShaderUniform(const String *name, Value *value);
		RNAPI void SetCustomShaderUniform(const String *name, Number *number);
		RNAPI Object *GetCustomShaderUniform(const String *name) const;
		
		RNAPI void SetSkipRendering(bool skip);
		bool GetSkipRendering() const { return _skipRendering; }

		uint32 GetOverride() const { return _override; }

		RNAPI Shader *GetFragmentShader(Shader::UsageHint type = Shader::UsageHint::Default) const;
		RNAPI Shader *GetVertexShader(Shader::UsageHint type = Shader::UsageHint::Default) const;
		
		uint8 GetColorWriteMask() const { return _properties.colorWriteMask; }

		DepthMode GetDepthMode() const { return _properties.depthMode; }
		bool GetDepthWriteEnabled() const { return _properties.depthWriteEnabled; }

		const Color &GetAmbientColor() const { return _properties.ambientColor; }
		const Color &GetDiffuseColor() const { return _properties.diffuseColor; }
		const Color &GetSpecularColor() const { return _properties.specularColor; }
		const Color &GetEmissiveColor() const { return _properties.emissiveColor; }
		
		const Matrix &GetCustomMatrix1() const { return _properties.customMatrix1; }
		const Matrix &GetCustomMatrix2() const { return _properties.customMatrix2; }

		const Array *GetTextures() const { return _textures; }
		float GetTextureTileFactor() const { return _properties.textureTileFactor; }
		CullMode GetCullMode() const { return _properties.cullMode; }

		const Array *GetFragmentBuffers() const { return _fragmentBuffers; }
		const Array *GetVertexBuffers() const { return _vertexBuffers; }

		bool GetUsePolygonOffset() const { return _properties.usePolygonOffset; }
		float GetPolygonOffsetFactor() const { return _properties.polygonOffsetFactor; }
		float GetPolygonOffsetUnits() const { return _properties.polygonOffsetUnits; }

		bool GetUseAlphaToCoverage() const { return _properties.useAlphaToCoverage; }
		Vector2 GetAlphaToCoverageClamp() const { return _properties.alphaToCoverageClamp; }

		const Properties &GetProperties() const { return _properties; }
		
		RNAPI const Properties &GetMergedProperties(Material *overrideMaterial);

	private:
		Override _override;

		Shader *_fragmentShader[static_cast<uint8>(Shader::UsageHint::COUNT)];
		Shader *_vertexShader[static_cast<uint8>(Shader::UsageHint::COUNT)];

		Array *_textures;
		Array *_vertexBuffers;
		Array *_fragmentBuffers;
		
		bool _skipRendering;

		Properties _properties;
		Properties _mergedProperties; //Used as temporary storage for the merged properties, to prevent reallocation, but could be reused between materials too...

		__RNDeclareMetaInternal(Material)
	};
}


#endif /* __RAYNE_MATERIAL_H_ */
