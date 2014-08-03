//
//  RNMaterial.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
#include "RNKVOImplementation.h"
#include "RNEnum.h"

namespace RN
{
	class Material : public Object
	{
	public:
		friend class Renderer;
		
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
			
			RNAPI ShaderUniform(const std::string &name, Type type, void *data, size_t count, bool copy=true);
			RNAPI ShaderUniform(const std::string &name, const Matrix &matrix);
			RNAPI ShaderUniform(const std::string &name, const Vector2 &vec2);
			RNAPI ShaderUniform(const std::string &name, const Vector3 &vec3);
			RNAPI ShaderUniform(const std::string &name, const Vector4 &vec4);
			
			RNAPI explicit ShaderUniform(const std::string &name, float fValue);
			RNAPI explicit ShaderUniform(const std::string &name, int32 iValue);
			RNAPI explicit ShaderUniform(const std::string &name, uint32 uiValue);
			
			RNAPI void SetFloatValue(float value);
			RNAPI void SetIntegerValue(int32 value);
			RNAPI void SetUIntValue(uint32 value);
			
			RNAPI void SetMatrix(const Matrix &matrix);
			RNAPI void SetVector(const Vector2 &vector);
			RNAPI void SetVector(const Vector3 &vector);
			RNAPI void SetVector(const Vector4 &vector);
			
			RNAPI void SetData(const void *data);
			
			RNAPI void Apply(ShaderProgram *program);
			
		private:
			void *GetPointerValue();
			void StoreData(Type type, void *data, size_t size, bool copy);
			size_t GetSizeForType(Type type) const;
			
			std::string _name;
			Type _type;
			bool _rawStorage;
			size_t _count;
			std::vector<uint8> _storage;
		};
		
		struct Override : public Enum<uint32>
		{
			Override()
			{}
			
			Override(int value) :
				Enum(value)
			{}
			
			enum
			{
				Culling = (1 << 0),
				Blending = (1 << 2),
				Blendmode = (1 << 3),
				Blendequation = (1 << 16),
				Ambient = (1 << 4),
				Diffuse = (1 << 5),
				Specular = (1 << 6),
				Emissive = (1 << 7),
				Depthtest = (1 << 8),
				Depthwrite = (1 << 9),
				DepthtestMode = (1 << 10),
				Discard = (1 << 11),
				DiscardThreshold = (1 << 12),
				Textures = (1 << 13),
				PolygonOffset = (1 << 14),
				PolygonMode = (1 << 15),
				Shader = (1 << 16),
				
				GroupBlending = (Blending | Blendmode | Blendequation),
				GroupDiscard = (Discard | DiscardThreshold | Textures)
			};
		};
		
		enum class CullMode : GLenum
		{
			None = 0,
			BackFace = GL_CCW,
			FrontFace = GL_CW
		};
		
		enum class DepthMode : GLenum
		{
			Never = GL_NEVER,
			Always = GL_ALWAYS,
			Less = GL_LESS,
			LessOrEqual = GL_LEQUAL,
			Equal = GL_EQUAL,
			NotEqual = GL_NOTEQUAL,
			GreaterOrEqual = GL_GEQUAL,
			Greater = GL_GREATER
		};
		
		enum class PolygonMode : GLenum
		{
			Points = GL_POINT,
			Lines = GL_LINE,
			Fill = GL_FILL
		};
		
		enum class BlendMode : GLenum
		{
			Zero = GL_ZERO,
			One  = GL_ONE,
			
			SourceColor = GL_SRC_COLOR,
			OneMinusSourceColor = GL_ONE_MINUS_SRC_COLOR,
			SourceAlpha = GL_SRC_ALPHA,
			OneMinusSourceAlpha = GL_ONE_MINUS_SRC_ALPHA,
			
			DestinationColor = GL_DST_COLOR,
			OneMinusDestinationColor = GL_ONE_MINUS_DST_COLOR,
			DestinationAlpha = GL_DST_ALPHA,
			OneMinusDestinationAlpha = GL_ONE_MINUS_DST_ALPHA
		};
		
		enum class BlendEquation : GLenum
		{
			Add = GL_FUNC_ADD,
			Subtract = GL_FUNC_SUBTRACT,
			ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
			Min = GL_MIN,
			Max = GL_MAX
		};
		
		RNAPI Material();
		RNAPI Material(Shader *shader);
		RNAPI Material(const Material *other);
		RNAPI Material(Deserializer *deserializer);
		RNAPI ~Material() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI void SetShader(Shader *shader);
		
		RNAPI Shader *GetShader() const;
		RNAPI void ApplyUniforms(ShaderProgram *program);
		
		RNAPI void AddTexture(Texture *texture);
		RNAPI void InsertTexture(Texture *texture, size_t index);
		RNAPI void ReplaceTexture(Texture *texture, size_t index);
		RNAPI void RemoveTexture(Texture *texture);
		RNAPI void RemoveTextures();
		
		RNAPI void Define(const std::string &define);
		RNAPI void Define(const std::string &define, const std::string &value);
		RNAPI void Define(const std::string &define, int32 value);
		RNAPI void Define(const std::string &define, float value);
		RNAPI void Undefine(const std::string &define);
		
		RNAPI const Array *GetTextures() const { return _textures; }
		RNAPI const ShaderLookup &GetLookup() const { return _lookup; }
		
		template<class ... Args>
		ShaderUniform *AddShaderUniform(Args&&... args)
		{
			ShaderUniform *uniform = new ShaderUniform(std::forward<Args>(args)...);
			InsertShaderUniform(uniform);
			
			return uniform;
		}
		
		void SetLighting(bool enabled) { lighting = enabled; }
		void SetCullMode(CullMode mode) { cullMode = mode; }
		void SetPolygonMode(PolygonMode mode) { polygonMode = mode; }
		
		void SetBlending(bool enabled) { blending = enabled; }
		void SetBlendEquation(BlendEquation equation) { SetRGBBlendEquation(equation); SetAlphaBlendEquation(equation); }
		void SetRGBBlendEquation(BlendEquation equation) { blendEquation = equation; }
		void SetAlphaBlendEquation(BlendEquation equation) { alphaBlendEquation = equation; }
		void SetBlendMode(BlendMode source, BlendMode destination) { SetRGBBlendMode(source, destination); SetAlphaBlendMode(source, destination); }
		void SetRGBBlendMode(BlendMode source, BlendMode destination)  { blendSource = source; blendDestination = destination; }
		void SetAlphaBlendMode(BlendMode source, BlendMode destination) { alphaBlendSource = source; alphaBlendDestination = destination; }
		
		void SetPolygonOffset(bool enabled) { polygonOffset = enabled; }
		void SetPolygonOffsetFactor(float factor) { polygonOffsetFactor = factor; }
		void SetPolygonOffsetUnits(float units) { polygonOffsetUnits = units; }
		
		void SetAmbientColor(const Color &color) { ambient = color; }
		void SetDiffuseColor(const Color &color) { diffuse = color; }
		void SetSpecularColor(const Color &color) { specular = color; }
		void SetEmissiveColor(const Color &color) { emissive = color; }
		
		void SetDepthTest(bool enabled) { depthTest = enabled; }
		void SetDepthWrite(bool enabled) { depthWrite = enabled; }
		void SetDepthTestMode(DepthMode mode) { depthTestMode = mode; }
		
		void SetDiscard(bool enabled) { discard = enabled; }
		void SetDiscardThreshold(float threshold) { discardThreshold = threshold; }
		
		void SetOverride(Override toverride) { override = toverride; }
		
		bool GetLighting() const { return lighting; }
		CullMode GetCullMode() const { return cullMode; }
		PolygonMode GetPolygonMode() const { return polygonMode; }
		
		bool GetBlending() const { return blending; }
		BlendEquation GetBlendEquation() const { return blendEquation; }
		BlendEquation GetAlphaBlendEquation() const { return alphaBlendEquation; }
		BlendMode GetBlendSource() const { return blendSource; }
		BlendMode GetAlphaBlendSource() const { return alphaBlendSource; }
		BlendMode GetBlendDestination() const { return blendDestination; }
		BlendMode GetAlphaBlendDestination() const { return alphaBlendDestination; }
		
		bool GetPolygonOffset() const { return polygonOffset; }
		float GetPolygonOffsetFactor() const { return polygonOffsetFactor; }
		float GetPolygonOffsetUnits() const { return polygonOffsetUnits; }
		
		const Color &GetAmbientColor() const { return ambient; }
		const Color &GetDiffuseColor() const { return diffuse; }
		const Color &GetSpecularColor() const { return specular; }
		const Color &GetEmissiveColor() const { return emissive; }
		
		bool GetDepthTest() const { return depthTest; }
		bool GetDepthWrite() const { return depthWrite; }
		DepthMode GetDepthTestMode() const { return depthTestMode; }
		
		bool GetDiscard() const { return discard; }
		float GetDiscardThreshold() const { return discardThreshold; }
		
		Override GetOverride() const { return override; }
		
		bool HasSeparateBlendEquation() const { return blendEquation != alphaBlendEquation; }
		bool HasSeparateBlendFunction() const { return blendSource != alphaBlendSource || blendDestination != alphaBlendDestination; }
		
	protected:
		bool lighting;
		
		CullMode cullMode;
		PolygonMode polygonMode;
		
		bool blending;
		BlendEquation blendEquation;
		BlendEquation alphaBlendEquation;
		
		BlendMode blendSource;
		BlendMode blendDestination;
		BlendMode alphaBlendSource;
		BlendMode alphaBlendDestination;
		
		bool polygonOffset;
		float polygonOffsetFactor;
		float polygonOffsetUnits;
		
		Color ambient;
		Color diffuse;
		Color specular;
		Color emissive;
		
		bool depthTest;
		bool depthWrite;
		DepthMode depthTestMode;
		
		bool discard;
		float discardThreshold;
		
		Override override;
		
	private:
		void Initialize();
		void UpdateLookupRequest();
		RNAPI void InsertShaderUniform(ShaderUniform *uniform);
		
		Shader *_shader;
		Array *_textures;
		
		ShaderLookup _lookup;
		std::vector<ShaderDefine> _defines;
		std::vector<ShaderUniform *> _uniforms;
		
		RNDeclareMeta(Material)
	};
	
	RNObjectClass(Material)
}

#endif /* __RAYNE_MATERIAL_H__ */
