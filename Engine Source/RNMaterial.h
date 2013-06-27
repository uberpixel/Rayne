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
		enum class BlendMode
		{
			Additive,
			Multiplicative,
			Interpolative,
			Cutout
		};
		
		enum
		{
			OverrideCulling = (1 << 0),
			OverrideCullmode = (1 << 1),
			OverrideBlending = (1 << 2),
			OverrideBlendmode = (1 << 3),
			OverrideShininess = (1 << 4),
			OverrideAmbient = (1 << 5),
			OverrideDiffuse = (1 << 6),
			OverrideSpecular = (1 << 7),
			OverrideEmissive = (1 << 8),
			OverrideDepthtest = (1 << 9),
			OverrideDepthwrite = (1 << 10),
			OverrideDepthtestMode = (1 << 11),
			OverrideDiscard = (1 << 12),
			OverrideDiscardThreshold = (1 << 13),
			OverrideTextures = (1 << 14),
			OverridePolygonOffset = (1 << 15)
			
			OverrideGroupDiscard = OverrideDiscard | OverrideDiscardThreshold | OverrideTextures
		};
		
		RNAPI Material();
		RNAPI Material(Shader *shader);
		RNAPI virtual ~Material();
		
		RNAPI void SetShader(Shader *shader);
		RNAPI void SetBlendMode(BlendMode mode);
		
		RNAPI Shader *Shader() const;
		
		RNAPI void AddTexture(Texture *texture);
		RNAPI void RemoveTexture(Texture *texture);
		RNAPI void RemoveTextures();
		
		RNAPI void Define(const std::string& define);
		RNAPI void Define(const std::string& define, const std::string& value);
		RNAPI void Define(const std::string& define, int32 value);
		RNAPI void Define(const std::string& define, float value);
		RNAPI void Undefine(const std::string& define);
		
		RNAPI const Array& Textures() const { return _textures; }
		RNAPI const ShaderLookup& Lookup() const { return _lookup; }
		
		bool culling;
		bool lighting;
		
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
		
		bool depthtest;
		bool depthwrite;
		GLenum depthtestmode;
		
		bool discard;
		float discardThreshold;
		
		uint64 override;
		
	private:
		void Initialize();
		void UpdateLookupRequest();
		
		RN::Shader *_shader;
		Array _textures;
		
		ShaderLookup _lookup;
		std::vector<ShaderDefine> _defines;
		
		RNDefineMetaWithTraits(Material, Object, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_MATERIAL_H__ */
