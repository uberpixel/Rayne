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
		MaterialDescriptor();
		~MaterialDescriptor();

		void AddTexture(Texture *texture);
		void RemoveAllTextures();

		const Array *GetTextures() const;

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

		Shader *GetFragmentShader() const { return _fragmentShader; }
		Shader *GetVertexShader() const { return _vertexShader; }

		DepthMode GetDepthMode() const { return _depthMode; }
		bool GetDepthWriteEnabled() const { return _depthWriteEnabled; }

		void SetDepthWriteEnabled(bool depthWrite);
		void SetDepthMode(DepthMode mode);

	private:
		Shader *_fragmentShader;
		Shader *_vertexShader;

		DepthMode _depthMode;
		bool _depthWriteEnabled;

		RNDeclareMeta(Material)
	};
}


#endif /* __RAYNE_MATERIAL_H_ */
