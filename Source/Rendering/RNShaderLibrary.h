//
//  RNShaderLibrary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SHADERLIBRARY_H_
#define __RAYNE_SHADERLIBRARY_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "../Objects/RNDictionary.h"
#include "RNShader.h"

namespace RN
{
	class Mesh;
	class Material;
	class ShaderLibrary : public Object
	{
	public:
		RNAPI virtual Shader *GetShaderWithName(const String *name, const Shader::Options *options = nullptr) = 0;
		RNAPI virtual Shader *GetInstancedShaderForShader(Shader *shader) = 0;

	protected:
		RNAPI ShaderLibrary();
		RNAPI virtual ~ShaderLibrary();

		__RNDeclareMetaInternal(ShaderLibrary)
	};
}


#endif /* __RAYNE_SHADERLIBRARY_H_ */
