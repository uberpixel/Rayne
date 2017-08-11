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

	enum class CullMode
	{
		None,
		BackFace,
		FrontFace
	};

	class Material : public Object
	{
	public:
		
		struct Properties
		{
			DepthMode depthMode;
			bool depthWriteEnabled;
			
			Color ambientColor;
			Color diffuseColor;
			Color specularColor;
			Color emissiveColor;
			
			bool usePolygonOffset;
			float polygonOffsetFactor;
			float polygonOffsetUnits;
			
			float discardThreshold;
			bool useAlphaToCoverage;
			float alphaToCoverageClamp;
			float textureTileFactor;
			CullMode cullMode;
		};

		RN_OPTIONS(Override, uint32,
			GroupDepth = (1 << 0),
			GroupColors = (1 << 1),
			GroupPolygonOffset = (1 << 2),
			GroupShaders = (1 << 3),
			DiscardThreshold = (1 << 4),
			TextureTileFactor = (1 << 5),
			CullMode = (1 << 6),
			GroupAlphaToCoverage = (1 << 7),

			DefaultDepth = (0xffffffff & ~GroupPolygonOffset)
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

		RNAPI void SetDepthWriteEnabled(bool depthWrite);
		RNAPI void SetDepthMode(DepthMode mode);

		RNAPI void SetAmbientColor(const Color &color);
		RNAPI void SetDiffuseColor(const Color &color);
		RNAPI void SetSpecularColor(const Color &color);
		RNAPI void SetEmissiveColor(const Color &color);

		RNAPI void SetDiscardThreshold(float threshold);
		RNAPI void SetTextureTileFactor(float factor);
		RNAPI void SetCullMode(CullMode mode);

		RNAPI void SetFragmentBuffers(const Array *buffers);
		RNAPI void SetVertexBuffers(const Array *buffers);

		RNAPI void SetPolygonOffset(bool enable, float factor = 1.1f, float units = 0.1f);
		RNAPI void SetAlphaToCoverage(bool enabled, float clamp = 1.0);

		uint32 GetOverride() const { return _override; }

		RNAPI Shader *GetFragmentShader(Shader::UsageHint type = Shader::UsageHint::Default) const;
		RNAPI Shader *GetVertexShader(Shader::UsageHint type = Shader::UsageHint::Default) const;

		DepthMode GetDepthMode() const { return _properties.depthMode; }
		bool GetDepthWriteEnabled() const { return _properties.depthWriteEnabled; }

		const Color &GetAmbientColor() const { return _properties.ambientColor; }
		const Color &GetDiffuseColor() const { return _properties.diffuseColor; }
		const Color &GetSpecularColor() const { return _properties.specularColor; }
		const Color &GetEmissiveColor() const { return _properties.emissiveColor; }

		const Array *GetTextures() const { return _textures; }
		float GetDiscardThreshold() const { return _properties.discardThreshold; }
		float GetTextureTileFactor() const { return _properties.textureTileFactor; }
		CullMode GetCullMode() const { return _properties.cullMode; }

		const Array *GetFragmentBuffers() const { return _fragmentBuffers; }
		const Array *GetVertexBuffers() const { return _vertexBuffers; }

		bool GetUsePolygonOffset() const { return _properties.usePolygonOffset; }
		float GetPolygonOffsetFactor() const { return _properties.polygonOffsetFactor; }
		float GetPolygonOffsetUnits() const { return _properties.polygonOffsetUnits; }

		bool GetUseAlphaToCoverage() const { return _properties.useAlphaToCoverage; }
		float GetAlphaToCoverageClamp() const { return _properties.alphaToCoverageClamp; }
		
		RNAPI const Properties GetMergedProperties(Material *overrideMaterial) const;

	private:
		Override _override;

		Shader *_fragmentShader[static_cast<uint8>(Shader::UsageHint::COUNT)];
		Shader *_vertexShader[static_cast<uint8>(Shader::UsageHint::COUNT)];

		Array *_textures;
		Array *_vertexBuffers;
		Array *_fragmentBuffers;

		Properties _properties;

		__RNDeclareMetaInternal(Material)
	};
}


#endif /* __RAYNE_MATERIAL_H_ */
