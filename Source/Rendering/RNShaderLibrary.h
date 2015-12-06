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
	class ShaderLookupRequest : public Object
	{
	public:
		RNAPI ShaderLookupRequest();

		bool receiveShadows;
		bool castShadows;

		bool discard;

	private:
		RNDeclareMeta(ShaderLookupRequest)
	};

	class ShaderCompileOptions : public Object
	{
	public:
		RNAPI ShaderCompileOptions();
		RNAPI ~ShaderCompileOptions();

		RNAPI void SetDefines(const Dictionary *defines);
		RNAPI void SetBasePath(const String *basePath);

		RNAPI bool IsEqual(const Object *other) const override;
		RNAPI size_t GetHash() const override;

		const Dictionary *GetDefines() const { return _defines; }
		const String *GetBasePath() const { return _basePath; }

	private:
		String *_basePath;
		Dictionary *_defines;

		RNDeclareMeta(ShaderCompileOptions)
	};

	class ShaderProgram : public Object
	{
	public:
		RNAPI ShaderProgram(Shader *vertexShader, Shader *fragmentShader);
		RNAPI ~ShaderProgram();

		Shader *GetVertexShader() const { return _vertexShader; }
		Shader *GetFragmentShader() const { return _fragmentShader; }

	private:
		Shader *_vertexShader;
		Shader *_fragmentShader;

		RNDeclareMeta(ShaderProgram)
	};

	class ShaderLibrary : public Object
	{
	public:
		RNAPI virtual Shader *GetShaderWithName(const String *name) = 0;
		RNAPI virtual Array *GetShaderNames() const = 0;

		RNDeclareMeta(ShaderLibrary)
	};
}


#endif /* __RAYNE_SHADERLIBRARY_H_ */
