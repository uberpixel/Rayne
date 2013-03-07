//
//  RNShader.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHADER_H__
#define __RAYNE_SHADER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNFile.h"
#include "RNArray.h"

namespace RN
{
	class ShaderProgram
	{
	public:
		enum
		{
			TypeNormal = 0,
			TypeInstanced = (1 << 1),
			TypeAnimated = (1 << 2),
			TypeLightning = (1 << 3)
		};
		
		GLuint program;
		
		GLuint matProj;
		GLuint matProjInverse;
		GLuint matView;
		GLuint matViewInverse;
		GLuint matModel;
		GLuint matModelInverse;
		GLuint matViewModel;
		GLuint matViewModelInverse;
		GLuint matProjView;
		GLuint matProjViewInverse;
		GLuint matProjViewModel;
		GLuint matProjViewModelInverse;
		GLuint matBones;
		
		GLuint imatModel;
		GLuint imatModelInverse;
		
		GLuint vertPosition;
		GLuint vertNormal;
		GLuint vertTangent;
		GLuint vertTexcoord0;
		GLuint vertTexcoord1;
		GLuint vertColor0;
		GLuint vertColor1;
		GLuint vertBoneWeights;
		GLuint vertBoneIndices;
		
		GLuint time;
		GLuint frameSize;
		GLuint clipPlanes;
		
		GLuint lightPosition;
		GLuint lightColor;
		GLuint lightCount;
		GLuint lightList;
		GLuint lightListOffset;
		GLuint lightListPosition;
		GLuint lightListColor;
		GLuint lightTileSize;
		
		Array<GLuint> texlocations;
		Array<GLuint> targetmaplocations;
		Array<GLuint> fraglocations;
		GLuint depthmap;
	};
	
	class Shader : public Object
	{
	public:		
		RNAPI Shader();
		RNAPI Shader(const std::string& shader);
		RNAPI virtual ~Shader();
		
		static Shader *WithFile(const std::string& shader);
		
		RNAPI void Define(const std::string& define);
		RNAPI void Define(const std::string& define, const std::string& value);
		RNAPI void Define(const std::string& define, int32 value);
		RNAPI void Define(const std::string& define, float value);
		RNAPI void Undefine(const std::string& define);
		
		RNAPI void SetVertexShader(const std::string& path);
		RNAPI void SetVertexShader(File *file);
		
		RNAPI void SetFragmentShader(const std::string& path);
		RNAPI void SetFragmentShader(File *file);
		
		RNAPI void SetGeometryShader(const std::string& path);
		RNAPI void SetGeometryShader(File *file);
		
		RNAPI ShaderProgram *ProgramOfType(uint32 type);
		RNAPI bool SupportsProgramOfType(uint32 type);
		
	private:
		struct ShaderDefine
		{
			std::string name;
			std::string value;
		};
		
		void SetShaderForType(const std::string& path, GLenum type);
		void SetShaderForType(File *file, GLenum type);
		void AddDefines();
		
		std::string PreProcessedShaderSource(const std::string& source);
		void CompileShader(GLenum type, GLuint *outShader);
		
		Array<ShaderDefine> _defines;
		
		uint32 _supportedPrograms;
		std::unordered_map<uint32, ShaderProgram *> _programs;
		
		std::string _vertexFile;
		std::string _vertexShader;
		
		std::string _fragmentFile;
		std::string _fragmentShader;
		
		std::string _geometryFile;
		std::string _geometryShader;
	};
}

#endif /* __RAYNE_SHADER_H__ */
