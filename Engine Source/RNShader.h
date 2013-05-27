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
		GLuint matNormal;
		GLuint matNormalInverse;
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
		
		std::vector<GLuint> texlocations;
		std::vector<GLuint> texinfolocations;
		std::vector<GLuint> targetmaplocations;
		std::vector<GLuint> targetmapinfolocations;
		std::vector<GLuint> fraglocations;
		GLuint depthmap;
		GLuint depthmapinfo;
	};
	
	class Shader : public Object
	{
	public:
		enum class ShaderType
		{
			VertexShader,
			FragmentShader,
			GeometryShader,
			TessellationControlShader,
			TessellationEvaluationShader
		};
		
		RNAPI Shader();
		RNAPI Shader(const std::string& shader);
		RNAPI virtual ~Shader();
		
		static Shader *WithFile(const std::string& shader);
		
		RNAPI void Define(const std::string& define);
		RNAPI void Define(const std::string& define, const std::string& value);
		RNAPI void Define(const std::string& define, int32 value);
		RNAPI void Define(const std::string& define, float value);
		RNAPI void Undefine(const std::string& define);
		
		RNAPI void SetShaderForType(const std::string& path, ShaderType type);
		RNAPI void SetShaderForType(File *file, ShaderType type);
		
		RNAPI ShaderProgram *ProgramOfType(uint32 type);
		RNAPI ShaderProgram *ProgramWithLookup(const ShaderLookup& lookup);
		
		RNAPI bool SupportsProgramOfType(uint32 type);
		
	private:
		struct DebugMarker
		{
			DebugMarker(uint32 tline, uint32 toffset, File *tfile)
			{
				line   = tline;
				offset = toffset;
				file   = tfile->Name() + "." + tfile->Extension();
			}
			
			DebugMarker(uint32 tline, uint32 toffset, const std::string& tfile)
			{
				line   = tline;
				offset = toffset;
				file   = tfile;
			}
			
			uint32 line;
			uint32 offset;
			std::string file;
		};
		
		struct PreProcessedFile
		{
			std::string data;
			std::string fullpath;
			std::vector<DebugMarker> marker;
			uint32 lines;
			uint32 offset;
		};
		
		enum IncludeMode
		{
			CurrentDir,
			IncludeDir
		};
		
		void AddDefines();
		
		std::string PreProcessedShaderSource(const std::string& source);
		
		void IncludeShader(const std::string& name, IncludeMode mode, File *parent, PreProcessedFile& output);
		void PreProcessFile(File *file, PreProcessedFile& output);
		
		void CompileShader(ShaderType type, const std::string& file, GLuint *outShader);
		void DumpLinkStatusAndDie(ShaderProgram *program);
		
		GLenum GLTypeForShaderType(ShaderType type);
		DebugMarker ResolveFileForLine(ShaderType type, uint32 line);
		
		std::vector<ShaderDefine> _defines;
		std::vector<ShaderDefine> _temporaryDefines;
		
		uint32 _supportedPrograms;
		std::unordered_map<ShaderLookup, ShaderProgram *> _programs;
		
		std::string _vertexFile;
		std::string _vertexShader;
		std::vector<DebugMarker> _vertexMarker;
		
		std::string _fragmentFile;
		std::string _fragmentShader;
		std::vector<DebugMarker> _fragmentMarker;
		
		std::string _geometryFile;
		std::string _geometryShader;
		std::vector<DebugMarker> _geometryMarker;
		
		RNDefineMeta(Shader, Object)
	};
}

#endif /* __RAYNE_SHADER_H__ */
