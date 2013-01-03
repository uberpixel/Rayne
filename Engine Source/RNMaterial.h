//
//  RNMaterial.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
	class Material : public Object, public BlockingProxy
	{
	public:
		Material(Shader *shader);
		virtual ~Material();
		
		void SetShader(Shader *shader);
		Shader *Shader() const;
		
		void AddTexture(Texture *texture);
		ObjectArray *Textures() const;
		Array<GLint> *TextureLocations() const;
		
		bool culling;
		GLenum cullmode;
		
		bool blending;
		GLenum blendSource;
		GLenum blendDestination;
		
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
		
	private:
		void GetUniforms();
		
		RN::Shader *_shader;
		ObjectArray *_textures;
		Array<GLint> *_textureLocations;
	};
}

#endif /* __RAYNE_MATERIAL_H__ */
