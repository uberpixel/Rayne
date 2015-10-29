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
#include "../Rendering/RNTexture.h"
#include "RNShader.h"

namespace RN
{
	class MaterialDescriptor
	{
	public:
		RNAPI MaterialDescriptor();
		RNAPI ~MaterialDescriptor();

		RNAPI void AddTexture(Texture *texture);
		RNAPI void RemoveAllTextures();

		RNAPI const Array *GetTextures() const;

		Shader *fragmentShader;
		Shader *vertexShader;

	private:
		Array *_textures;
	};

	class Material : public Object
	{
	public:
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

		RNAPI Material(const MaterialDescriptor &descriptor);
		RNAPI ~Material() override;

		RNAPI void SetDepthWriteEnabled(bool depthWrite);
		RNAPI void SetDepthMode(DepthMode mode);

		RNAPI void SetAmbientColor(const Color &color);
		RNAPI void SetDiffuseColor(const Color &color);
		RNAPI void SetSpecularColor(const Color &color);
		RNAPI void SetEmissiveColor(const Color &color);

		Shader *GetFragmentShader() const { return _fragmentShader; }
		Shader *GetVertexShader() const { return _vertexShader; }

		DepthMode GetDepthMode() const { return _depthMode; }
		bool GetDepthWriteEnabled() const { return _depthWriteEnabled; }

		const Color &GetAmbientColor() const { return _ambientColor; }
		const Color &GetDiffuseColor() const { return _diffuseColor; }
		const Color &GetSpecularColor() const { return _specularColor; }
		const Color &GetEmissiveColor() const { return _emissiveColor; }

		const Array *GetTextures() const { return _textures; }

	private:
		Shader *_fragmentShader;
		Shader *_vertexShader;

		Array *_textures;

		DepthMode _depthMode;
		bool _depthWriteEnabled;

		Color _ambientColor;
		Color _diffuseColor;
		Color _specularColor;
		Color _emissiveColor;

		RNDeclareMeta(Material)
	};
}


#endif /* __RAYNE_MATERIAL_H_ */
