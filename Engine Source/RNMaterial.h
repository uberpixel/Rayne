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
#include "RNMatrixQuaternion.h"
#include "RNVector.h"

namespace RN
{
	class Material : public Object
	{
	public:
		class ShaderUniform
		{
		public:
			friend class Material;
			
			enum class Type
			{
				Float1,
				Float2,
				Float3,
				Float4,
				
				Int1,
				Int2,
				Int3,
				Int4,
				
				UInt1,
				UInt2,
				UInt3,
				UInt4,
				
				Matrix
			};
			
			ShaderUniform(const std::string& name, Type type, void *data, size_t size, bool copy=true);
			ShaderUniform(const std::string& name, const Matrix& matrix);
			ShaderUniform(const std::string& name, const Vector2& vec2);
			ShaderUniform(const std::string& name, const Vector3& vec3);
			ShaderUniform(const std::string& name, const Vector4& vec4);
			
			explicit ShaderUniform(const std::string& name, float fValue);
			explicit ShaderUniform(const std::string& name, int32 iValue);
			explicit ShaderUniform(const std::string& name, uint32 uiValue);
			
			void SetFloatValue(float value);
			void SetIntegerValue(int32 value);
			void SetUIntValue(uint32 value);
			
			void SetMatrix(const Matrix& matrix);
			void SetVector(const Vector2& vector);
			void SetVector(const Vector3& vector);
			void SetVector(const Vector4& vector);
			
			void SetData(const void *data);
			
			void Apply(ShaderProgram *program);
			
		private:
			void *GetPointerValue();
			void StoreData(Type type, void *data, size_t size, bool copy);
			
			std::string _name;
			Type _type;
			bool _rawStorage;
			size_t _size;
			std::vector<uint8> _storage;
		};
		
		enum class BlendMode
		{
			Additive,
			Multiplicative,
			Interpolative,
			InterpolativePremultiplied,
			Cutout
		};
		
		enum
		{
			OverrideCulling = (1 << 0),
			OverrideCullmode = (1 << 1),
			OverrideBlending = (1 << 2),
			OverrideBlendmode = (1 << 3),
			OverrideAmbient = (1 << 4),
			OverrideDiffuse = (1 << 5),
			OverrideSpecular = (1 << 6),
			OverrideEmissive = (1 << 7),
			OverrideDepthtest = (1 << 8),
			OverrideDepthwrite = (1 << 9),
			OverrideDepthtestMode = (1 << 10),
			OverrideDiscard = (1 << 11),
			OverrideDiscardThreshold = (1 << 12),
			OverrideTextures = (1 << 13),
			OverridePolygonOffset = (1 << 14),
			
			OverrideGroupDiscard = OverrideDiscard | OverrideDiscardThreshold | OverrideTextures
		};
		
		RNAPI Material();
		RNAPI Material(Shader *shader);
		RNAPI virtual ~Material();
		
		RNAPI void SetShader(Shader *shader);
		RNAPI void SetBlendMode(BlendMode mode);
		
		RNAPI Shader *GetShader() const;
		RNAPI void ApplyUniforms(ShaderProgram *program);
		
		RNAPI void AddTexture(Texture *texture);
		RNAPI void RemoveTexture(Texture *texture);
		RNAPI void RemoveTextures();
		
		RNAPI void Define(const std::string& define);
		RNAPI void Define(const std::string& define, const std::string& value);
		RNAPI void Define(const std::string& define, int32 value);
		RNAPI void Define(const std::string& define, float value);
		RNAPI void Undefine(const std::string& define);
		
		RNAPI const Array& GetTextures() const { return _textures; }
		RNAPI const ShaderLookup& GetLookup() const { return _lookup; }
		
		template<class ... Args>
		ShaderUniform *AddShaderUniform(Args&&... args)
		{
			ShaderUniform *uniform = new ShaderUniform(std::forward<Args>(args)...);
			InsertShaderUniform(uniform);
			
			return uniform;
		}
		
		bool culling;
		bool lighting;
		
		GLenum cullmode;
		
		bool blending;
		GLenum blendSource;
		GLenum blendDestination;
		
		bool polygonOffset;
		float polygonOffsetFactor;
		float polygonOffsetUnits;
		
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
		void InsertShaderUniform(ShaderUniform *uniform);
		
		RN::Shader *_shader;
		Array _textures;
		
		ShaderLookup _lookup;
		std::vector<ShaderDefine> _defines;
		std::vector<ShaderUniform *> _uniforms;
		
		RNDefineMetaWithTraits(Material, Object, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_MATERIAL_H__ */
