//
//  RNShader.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHADER_H__
#define __RAYNE_SHADER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNFile.h"
#include "RNArray.h"
#include "RNShaderLookup.h"

namespace RN
{
	class ShaderProgram
	{
	public:
		enum
		{
			TypeNormal = 0,
			TypeInstanced = (1 << 1),
			TypeAnimated  = (1 << 2),
			TypeLighting = (1 << 3),
			TypeDiscard   = (1 << 4),
			TypeDirectionalShadows = (1 << 5)
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
		
		GLuint instancingData;
		
		GLuint attPosition;
		GLuint attNormal;
		GLuint attTangent;
		GLuint attTexcoord0;
		GLuint attTexcoord1;
		GLuint attColor0;
		GLuint attColor1;
		GLuint attBoneWeights;
		GLuint attBoneIndices;
		
		GLuint time;
		GLuint frameSize;
		GLuint clipPlanes;
		GLuint discardThreshold;
		
		GLuint ambient;
		GLuint diffuse;
		GLuint specular;
		GLuint emissive;
		GLuint shininess;
		
		GLuint viewPosition;
		
		GLuint lightPointCount;
		GLuint lightPointPosition;
		GLuint lightPointColor;
		GLuint lightPointList;
		GLuint lightPointListOffset;
		GLuint lightPointListData;
		
		GLuint lightSpotCount;
		GLuint lightSpotPosition;
		GLuint lightSpotColor;
		GLuint lightSpotDirection;
		GLuint lightSpotList;
		GLuint lightSpotListOffset;
		GLuint lightSpotListData;
		
		GLuint lightDirectionalDirection;
		GLuint lightDirectionalColor;
		GLuint lightDirectionalCount;
		GLuint lightDirectionalMatrix;
		GLuint lightDirectionalDepth;
		
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
		RNAPI ShaderProgram *ProgramWithLookup(const ShaderLookup& lookup);
		
		RNAPI bool SupportsProgramOfType(uint32 type);
		
	private:
		void SetShaderForType(const std::string& path, GLenum type);
		void SetShaderForType(File *file, GLenum type);
		void AddDefines();
		
		std::string PreProcessedShaderSource(const std::string& source);
		std::string IncludeShader(File *source, const std::string& name);
		std::string PreProcessFile(File *file);
		
		void CompileShader(GLenum type, GLuint *outShader);
		void DumpLinkStatusAndDie(ShaderProgram *program);
		
		std::vector<ShaderDefine> _defines;
		std::vector<ShaderDefine> _temporaryDefines;
		
		uint32 _supportedPrograms;
		std::unordered_map<ShaderLookup, ShaderProgram *> _programs;
		
		std::string _vertexFile;
		std::string _vertexShader;
		
		std::string _fragmentFile;
		std::string _fragmentShader;
		
		std::string _geometryFile;
		std::string _geometryShader;
		
		RNDefineMeta(Shader, Object)
	};
}

#endif /* __RAYNE_SHADER_H__ */
