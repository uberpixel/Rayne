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

	class MaterialDescriptor
	{
	public:
		RNAPI MaterialDescriptor();
		RNAPI MaterialDescriptor(const MaterialDescriptor &other);
		RNAPI ~MaterialDescriptor();

		RNAPI void SetTextures(const Array *textures);
		RNAPI void AddTexture(Texture *texture);
		RNAPI void RemoveAllTextures();

		RNAPI const Array *GetTextures() const;

		Shader *fragmentShader;
		Shader *vertexShader;

		DepthMode depthMode;
		bool depthWriteEnabled;

		bool usePolygonOffset;
		float polygonOffsetFactor;
		float polygonOffsetUnits;

		Color ambientColor;
		Color diffuseColor;
		Color specularColor;
		Color emissiveColor;

		float discardThreshold;
		float textureTileFactor;
		CullMode cullMode;

	private:
		Array *_textures;
	};

	class Material : public Object
	{
	public:
		RNAPI Material(const MaterialDescriptor &descriptor);
		RNAPI Material(const Material *other);
		RNAPI ~Material() override;

		RNAPI static Material *WithDescriptor(const MaterialDescriptor &descriptor);

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

		RNAPI MaterialDescriptor GetDescriptor() const;

		Shader *GetFragmentShader() const { return _fragmentShader; }
		Shader *GetVertexShader() const { return _vertexShader; }

		DepthMode GetDepthMode() const { return _depthMode; }
		bool GetDepthWriteEnabled() const { return _depthWriteEnabled; }

		const Color &GetAmbientColor() const { return _ambientColor; }
		const Color &GetDiffuseColor() const { return _diffuseColor; }
		const Color &GetSpecularColor() const { return _specularColor; }
		const Color &GetEmissiveColor() const { return _emissiveColor; }

		const Array *GetTextures() const { return _textures; }
		float GetDiscardThreshold() const { return _discardThreshold; }
		float GetTextureTileFactor() const { return _textureTileFactor; }
		CullMode GetCullMode() const { return _cullMode; }

		const Array *GetFragmentBuffers() const { return _fragmentBuffers; }
		const Array *GetVertexBuffers() const { return _vertexBuffers; }

		float GetUsePolygonOffset() const { return _usePolygonOffset; }
		float GetPolygonOffsetFactor() const { return _polygonOffsetFactor; }
		float GetPolygonOffsetUnits() const { return _polygonOffsetUnits; }

	private:
		Shader *_fragmentShader;
		Shader *_vertexShader;

		Array *_textures;
		Array *_vertexBuffers;
		Array *_fragmentBuffers;

		DepthMode _depthMode;
		bool _depthWriteEnabled;

		Color _ambientColor;
		Color _diffuseColor;
		Color _specularColor;
		Color _emissiveColor;

		bool _usePolygonOffset;
		float _polygonOffsetFactor;
		float _polygonOffsetUnits;

		float _discardThreshold;
		float _textureTileFactor;
		CullMode _cullMode;

		__RNDeclareMetaInternal(Material)
	};
}


#endif /* __RAYNE_MATERIAL_H_ */
