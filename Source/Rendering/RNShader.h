//
//  RNShader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SHADER_H_
#define __RAYNE_SHADER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "../Objects/RNArray.h"
#include "RNRendererTypes.h"

namespace RN
{
	class ShaderLibrary;
	class Shader : public Object
	{
	public:
		class Attribute : public Object
		{
		public:
			virtual ~Attribute();

			const String *GetName() const { return _name; }
			PrimitiveType GetType() const { return _type; }

			const String *GetDescription() const override { return RNSTR("<ShaderAttribute: name: " << _name << ", type: " << (int)_type << ">"); }

		protected:
			RNAPI Attribute(const String *name, PrimitiveType type);

		private:
			String *_name;
			PrimitiveType _type;

			__RNDeclareMetaInternal(Attribute)
		};


		enum class Type
		{
			Fragment,
			Vertex,
			Compute
		};

		RNAPI virtual const String *GetName() const = 0;
		RNAPI virtual const Array *GetAttributes() const = 0;

		Type GetType() const { return _type; }
		ShaderLibrary *GetLibrary() const { return _library; }

	protected:
		RNAPI Shader(Type type, ShaderLibrary *library);

	private:
		Type _type;
		ShaderLibrary *_library;

		__RNDeclareMetaInternal(Shader)
	};
}


#endif /* __RAYNE_SHADER_H_ */
