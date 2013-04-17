//
//  RNMaterial.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATERIAL_H__
#define __RAYNE_MATERIAL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNShader.h"
#include "RNTexture.h"
#include "RNColor.h"

namespace RN
{
	class Material : public Object
	{
	public:		
		RNAPI Material();
		RNAPI Material(Shader *shader);
		RNAPI virtual ~Material();
		
		RNAPI void SetShader(Shader *shader);
		RNAPI Shader *Shader() const;
		
		RNAPI void AddTexture(Texture *texture);
		RNAPI void RemoveTexture(Texture *texture);
		RNAPI void RemoveTextures();
		
		RNAPI Array<Texture> *Textures() const;
		
		bool culling;
		GLenum cullmode;
		
		bool blending;
		GLenum blendSource;
		GLenum blendDestination;
		
		bool polygonOffset;
		float polygonOffsetFactor;
		float polygonOffsetUnits;
		
		float shininess;
		
		Color ambient;
		Color diffuse;
		Color specular;
		Color emissive;
		
		bool alphatest;
		float alphatestvalue;
		
		bool depthtest;
		bool depthwrite;
		GLenum depthtestmode;
		
		bool discard;
		float discardThreshold;
		
	private:
		void SetDefaultProperties();
		
		RN::Shader *_shader;
		Array<Texture> *_textures;
		
		RNDefineMeta(Material, Object)
	};
}

#endif /* __RAYNE_MATERIAL_H__ */
