//
//  RNOpenGLBinder.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenGL.h"
#include "RNBase.h"

namespace RN
{
#if RN_PLATFORM_WINDOWS
	namespace wgl
	{
		PFNWGLCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB = nullptr;
		PFNWGLCHOOSEPIXELFORMATARBPROC ChoosePixelFormatARB = nullptr;
		PFNWGLSWAPINTERVALEXTPROC SwapIntervalEXT = nullptr;
		PFNWGLGETEXTENSIONSSTRINGARBPROC GetExtensionsStringARB = nullptr;
	}
#endif

	namespace gl
	{
		// 1.0
		PFNGLCULLFACEPROC CullFace = nullptr;
		PFNGLFRONTFACEPROC FrontFace = nullptr;
		PFNGLHINTPROC Hint = nullptr;
		PFNGLLINEWIDTHPROC LineWidth = nullptr;
		PFNGLPOINTSIZEPROC PointSize = nullptr;
		PFNGLPOLYGONMODEPROC PolygonMode = nullptr;
		PFNGLSCISSORPROC Scissor = nullptr;
		PFNGLTEXPARAMETERFPROC TexParameterf = nullptr;
		PFNGLTEXPARAMETERFVPROC TexParameterfv = nullptr;
		PFNGLTEXPARAMETERIPROC TexParameteri = nullptr;
		PFNGLTEXPARAMETERIVPROC TexParameteriv = nullptr;
		PFNGLTEXIMAGE1DPROC TexImage1D = nullptr;
		PFNGLTEXIMAGE2DPROC TexImage2D = nullptr;
		PFNGLDRAWBUFFERPROC DrawBuffer = nullptr;
		PFNGLCLEARPROC Clear = nullptr;
		PFNGLCLEARCOLORPROC ClearColor = nullptr;
		PFNGLCLEARSTENCILPROC ClearStencil = nullptr;
		PFNGLCLEARDEPTHPROC ClearDepth = nullptr;
		PFNGLSTENCILMASKPROC StencilMask = nullptr;
		PFNGLCOLORMASKPROC ColorMask = nullptr;
		PFNGLDEPTHMASKPROC DepthMask = nullptr;
		PFNGLDISABLEPROC Disable = nullptr;
		PFNGLENABLEPROC Enable = nullptr;
		PFNGLFINISHPROC Finish = nullptr;
		PFNGLFLUSHPROC Flush = nullptr;
		PFNGLBLENDFUNCPROC BlendFunc = nullptr;
		PFNGLLOGICOPPROC LogicOp = nullptr;
		PFNGLSTENCILFUNCPROC StencilFunc = nullptr;
		PFNGLSTENCILOPPROC StencilOp = nullptr;
		PFNGLDEPTHFUNCPROC DepthFunc = nullptr;
		PFNGLPIXELSTOREFPROC PixelStoref = nullptr;
		PFNGLPIXELSTOREIPROC PixelStorei = nullptr;
		PFNGLREADBUFFERPROC ReadBuffer = nullptr;
		PFNGLREADPIXELSPROC ReadPixels = nullptr;
		PFNGLGETBOOLEANVPROC GetBooleanv = nullptr;
		PFNGLGETDOUBLEVPROC GetDoublev = nullptr;
		PFNGLGETERRORPROC GetError = nullptr;
		PFNGLGETFLOATVPROC GetFloatv = nullptr;
		PFNGLGETINTEGERVPROC GetIntegerv = nullptr;
		PFNGLGETSTRINGPROC GetString = nullptr;
		PFNGLGETTEXIMAGEPROC GetTexImage = nullptr;
		PFNGLGETTEXPARAMETERFVPROC GetTexParameterfv = nullptr;
		PFNGLGETTEXPARAMETERIVPROC GetTexParameteriv = nullptr;
		PFNGLGETTEXLEVELPARAMETERFVPROC GetTexLevelParameterfv = nullptr;
		PFNGLGETTEXLEVELPARAMETERIVPROC GetTexLevelParameteriv = nullptr;
		PFNGLISENABLEDPROC IsEnabled = nullptr;
		PFNGLDEPTHRANGEPROC DepthRange = nullptr;
		PFNGLVIEWPORTPROC Viewport = nullptr;

		// 1.1
		PFNGLDRAWARRAYSPROC DrawArrays = nullptr;
		PFNGLDRAWELEMENTSPROC DrawElements = nullptr;
		PFNGLGETPOINTERVPROC GetPointerv = nullptr;
		PFNGLPOLYGONOFFSETPROC PolygonOffset = nullptr;
		PFNGLCOPYTEXIMAGE1DPROC CopyTexImage1D = nullptr;
		PFNGLCOPYTEXIMAGE2DPROC CopyTexImage2D = nullptr;
		PFNGLCOPYTEXSUBIMAGE1DPROC CopyTexSubImage1D = nullptr;
		PFNGLCOPYTEXSUBIMAGE2DPROC CopyTexSubImage2D = nullptr;
		PFNGLTEXSUBIMAGE1DPROC TexSubImage1D = nullptr;
		PFNGLTEXSUBIMAGE2DPROC TexSubImage2D = nullptr;
		PFNGLBINDTEXTUREPROC BindTexture = nullptr;
		PFNGLDELETETEXTURESPROC DeleteTextures = nullptr;
		PFNGLGENTEXTURESPROC GenTextures = nullptr;
		PFNGLISTEXTUREPROC IsTexture = nullptr;

		// 1.2
		PFNGLDRAWRANGEELEMENTSPROC DrawRangeElements = nullptr;
		PFNGLTEXIMAGE3DPROC TexImage3D = nullptr;
		PFNGLTEXSUBIMAGE3DPROC TexSubImage3D = nullptr;
		PFNGLCOPYTEXSUBIMAGE3DPROC CopyTexSubImage3D = nullptr;

		// 1.3
		PFNGLACTIVETEXTUREPROC ActiveTexture = nullptr;
		PFNGLSAMPLECOVERAGEPROC SampleCoverage = nullptr;
		PFNGLCOMPRESSEDTEXIMAGE3DPROC CompressedTexImage3D = nullptr;
		PFNGLCOMPRESSEDTEXIMAGE2DPROC CompressedTexImage2D = nullptr;
		PFNGLCOMPRESSEDTEXIMAGE1DPROC CompressedTexImage1D = nullptr;
		PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC CompressedTexSubImage3D = nullptr;
		PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC CompressedTexSubImage2D = nullptr;
		PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC CompressedTexSubImage1D = nullptr;
		PFNGLGETCOMPRESSEDTEXIMAGEPROC GetCompressedTexImage = nullptr;

		// 1.4
		PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate = nullptr;
		PFNGLMULTIDRAWARRAYSPROC MultiDrawArrays = nullptr;
		PFNGLMULTIDRAWELEMENTSPROC MultiDrawElements = nullptr;
		PFNGLPOINTPARAMETERFPROC PointParameterf = nullptr;
		PFNGLPOINTPARAMETERFVPROC PointParameterfv = nullptr;
		PFNGLPOINTPARAMETERIPROC PointParameteri = nullptr;
		PFNGLPOINTPARAMETERIVPROC PointParameteriv = nullptr;
		PFNGLBLENDCOLORPROC BlendColor = nullptr;
		PFNGLBLENDEQUATIONPROC BlendEquation = nullptr;

		// 1.5
		PFNGLGENQUERIESPROC GenQueries = nullptr;
		PFNGLDELETEQUERIESPROC DeleteQueries = nullptr;
		PFNGLISQUERYPROC IsQuery = nullptr;
		PFNGLBEGINQUERYPROC BeginQuery = nullptr;
		PFNGLENDQUERYPROC EndQuery = nullptr;
		PFNGLGETQUERYIVPROC GetQueryiv = nullptr;
		PFNGLGETQUERYOBJECTIVPROC GetQueryObjectiv = nullptr;
		PFNGLGETQUERYOBJECTUIVPROC GetQueryObjectuiv = nullptr;
		PFNGLBINDBUFFERPROC BindBuffer = nullptr;
		PFNGLDELETEBUFFERSPROC DeleteBuffers = nullptr;
		PFNGLGENBUFFERSPROC GenBuffers = nullptr;
		PFNGLISBUFFERPROC IsBuffer = nullptr;
		PFNGLBUFFERDATAPROC BufferData = nullptr;
		PFNGLBUFFERSUBDATAPROC BufferSubData = nullptr;
		PFNGLGETBUFFERSUBDATAPROC GetBufferSubData = nullptr;
		PFNGLMAPBUFFERPROC MapBuffer = nullptr;
		PFNGLUNMAPBUFFERPROC UnmapBuffer = nullptr;
		PFNGLGETBUFFERPARAMETERIVPROC GetBufferParameteriv = nullptr;
		PFNGLGETBUFFERPOINTERVPROC GetBufferPointerv = nullptr;

		// 2.0
		PFNGLBLENDEQUATIONSEPARATEPROC BlendEquationSeparate = nullptr;
		PFNGLDRAWBUFFERSPROC DrawBuffers = nullptr;
		PFNGLSTENCILOPSEPARATEPROC StencilOpSeparate = nullptr;
		PFNGLSTENCILFUNCSEPARATEPROC StencilFuncSeparate = nullptr;
		PFNGLSTENCILMASKSEPARATEPROC StencilMaskSeparate = nullptr;
		PFNGLATTACHSHADERPROC AttachShader = nullptr;
		PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation = nullptr;
		PFNGLCOMPILESHADERPROC CompileShader = nullptr;
		PFNGLCREATEPROGRAMPROC CreateProgram = nullptr;
		PFNGLCREATESHADERPROC CreateShader = nullptr;
		PFNGLDELETEPROGRAMPROC DeleteProgram = nullptr;
		PFNGLDELETESHADERPROC DeleteShader = nullptr;
		PFNGLDETACHSHADERPROC DetachShader = nullptr;
		PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray = nullptr;
		PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;
		PFNGLGETACTIVEATTRIBPROC GetActiveAttrib = nullptr;
		PFNGLGETACTIVEUNIFORMPROC GetActiveUniform = nullptr;
		PFNGLGETATTACHEDSHADERSPROC GetAttachedShaders = nullptr;
		PFNGLGETATTRIBLOCATIONPROC GetAttribLocation = nullptr;
		PFNGLGETPROGRAMIVPROC GetProgramiv = nullptr;
		PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = nullptr;
		PFNGLGETSHADERIVPROC GetShaderiv = nullptr;
		PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = nullptr;
		PFNGLGETSHADERSOURCEPROC GetShaderSource = nullptr;
		PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = nullptr;
		PFNGLGETUNIFORMFVPROC GetUniformfv = nullptr;
		PFNGLGETUNIFORMIVPROC GetUniformiv = nullptr;
		PFNGLGETVERTEXATTRIBDVPROC GetVertexAttribdv = nullptr;
		PFNGLGETVERTEXATTRIBFVPROC GetVertexAttribfv = nullptr;
		PFNGLGETVERTEXATTRIBIVPROC GetVertexAttribiv = nullptr;
		PFNGLGETVERTEXATTRIBPOINTERVPROC GetVertexAttribPointerv = nullptr;
		PFNGLISPROGRAMPROC IsProgram = nullptr;
		PFNGLISSHADERPROC IsShader = nullptr;
		PFNGLLINKPROGRAMPROC LinkProgram = nullptr;
		PFNGLSHADERSOURCEPROC ShaderSource = nullptr;
		PFNGLUSEPROGRAMPROC UseProgram = nullptr;
		PFNGLUNIFORM1FPROC Uniform1f = nullptr;
		PFNGLUNIFORM2FPROC Uniform2f = nullptr;
		PFNGLUNIFORM3FPROC Uniform3f = nullptr;
		PFNGLUNIFORM4FPROC Uniform4f = nullptr;
		PFNGLUNIFORM1IPROC Uniform1i = nullptr;
		PFNGLUNIFORM2IPROC Uniform2i = nullptr;
		PFNGLUNIFORM3IPROC Uniform3i = nullptr;
		PFNGLUNIFORM4IPROC Uniform4i = nullptr;
		PFNGLUNIFORM1FVPROC Uniform1fv = nullptr;
		PFNGLUNIFORM2FVPROC Uniform2fv = nullptr;
		PFNGLUNIFORM3FVPROC Uniform3fv = nullptr;
		PFNGLUNIFORM4FVPROC Uniform4fv = nullptr;
		PFNGLUNIFORM1IVPROC Uniform1iv = nullptr;
		PFNGLUNIFORM2IVPROC Uniform2iv = nullptr;
		PFNGLUNIFORM3IVPROC Uniform3iv = nullptr;
		PFNGLUNIFORM4IVPROC Uniform4iv = nullptr;
		PFNGLUNIFORMMATRIX2FVPROC UniformMatrix2fv = nullptr;
		PFNGLUNIFORMMATRIX3FVPROC UniformMatrix3fv = nullptr;
		PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv = nullptr;
		PFNGLVALIDATEPROGRAMPROC ValidateProgram = nullptr;
		PFNGLVERTEXATTRIB1DPROC VertexAttrib1d = nullptr;
		PFNGLVERTEXATTRIB1DVPROC VertexAttrib1dv = nullptr;
		PFNGLVERTEXATTRIB1FPROC VertexAttrib1f = nullptr;
		PFNGLVERTEXATTRIB1FVPROC VertexAttrib1fv = nullptr;
		PFNGLVERTEXATTRIB1SPROC VertexAttrib1s = nullptr;
		PFNGLVERTEXATTRIB1SVPROC VertexAttrib1sv = nullptr;
		PFNGLVERTEXATTRIB2DPROC VertexAttrib2d = nullptr;
		PFNGLVERTEXATTRIB2DVPROC VertexAttrib2dv = nullptr;
		PFNGLVERTEXATTRIB2FPROC VertexAttrib2f = nullptr;
		PFNGLVERTEXATTRIB2FVPROC VertexAttrib2fv = nullptr;
		PFNGLVERTEXATTRIB2SPROC VertexAttrib2s = nullptr;
		PFNGLVERTEXATTRIB2SVPROC VertexAttrib2sv = nullptr;
		PFNGLVERTEXATTRIB3DPROC VertexAttrib3d = nullptr;
		PFNGLVERTEXATTRIB3DVPROC VertexAttrib3dv = nullptr;
		PFNGLVERTEXATTRIB3FPROC VertexAttrib3f = nullptr;
		PFNGLVERTEXATTRIB3FVPROC VertexAttrib3fv = nullptr;
		PFNGLVERTEXATTRIB3SPROC VertexAttrib3s = nullptr;
		PFNGLVERTEXATTRIB3SVPROC VertexAttrib3sv = nullptr;
		PFNGLVERTEXATTRIB4NBVPROC VertexAttrib4Nbv = nullptr;
		PFNGLVERTEXATTRIB4NIVPROC VertexAttrib4Niv = nullptr;
		PFNGLVERTEXATTRIB4NSVPROC VertexAttrib4Nsv = nullptr;
		PFNGLVERTEXATTRIB4NUBPROC VertexAttrib4Nub = nullptr;
		PFNGLVERTEXATTRIB4NUBVPROC VertexAttrib4Nubv = nullptr;
		PFNGLVERTEXATTRIB4NUIVPROC VertexAttrib4Nuiv = nullptr;
		PFNGLVERTEXATTRIB4NUSVPROC VertexAttrib4Nusv = nullptr;
		PFNGLVERTEXATTRIB4BVPROC VertexAttrib4bv = nullptr;
		PFNGLVERTEXATTRIB4DPROC VertexAttrib4d = nullptr;
		PFNGLVERTEXATTRIB4DVPROC VertexAttrib4dv = nullptr;
		PFNGLVERTEXATTRIB4FPROC VertexAttrib4f = nullptr;
		PFNGLVERTEXATTRIB4FVPROC VertexAttrib4fv = nullptr;
		PFNGLVERTEXATTRIB4IVPROC VertexAttrib4iv = nullptr;
		PFNGLVERTEXATTRIB4SPROC VertexAttrib4s = nullptr;
		PFNGLVERTEXATTRIB4SVPROC VertexAttrib4sv = nullptr;
		PFNGLVERTEXATTRIB4UBVPROC VertexAttrib4ubv = nullptr;
		PFNGLVERTEXATTRIB4UIVPROC VertexAttrib4uiv = nullptr;
		PFNGLVERTEXATTRIB4USVPROC VertexAttrib4usv = nullptr;
		PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer = nullptr;

		// 2.1
		PFNGLUNIFORMMATRIX2X3FVPROC UniformMatrix2x3fv = nullptr;
		PFNGLUNIFORMMATRIX3X2FVPROC UniformMatrix3x2fv = nullptr;
		PFNGLUNIFORMMATRIX2X4FVPROC UniformMatrix2x4fv = nullptr;
		PFNGLUNIFORMMATRIX4X2FVPROC UniformMatrix4x2fv = nullptr;
		PFNGLUNIFORMMATRIX3X4FVPROC UniformMatrix3x4fv = nullptr;
		PFNGLUNIFORMMATRIX4X3FVPROC UniformMatrix4x3fv = nullptr;

		// 3.0
		PFNGLCOLORMASKIPROC ColorMaski = nullptr;
		PFNGLGETBOOLEANI_VPROC GetBooleani_v = nullptr;
		PFNGLGETINTEGERI_VPROC GetIntegeri_v = nullptr;
		PFNGLENABLEIPROC Enablei = nullptr;
		PFNGLDISABLEIPROC Disablei = nullptr;
		PFNGLISENABLEDIPROC IsEnabledi = nullptr;
		PFNGLBEGINTRANSFORMFEEDBACKPROC BeginTransformFeedback = nullptr;
		PFNGLENDTRANSFORMFEEDBACKPROC EndTransformFeedback = nullptr;
		PFNGLBINDBUFFERRANGEPROC BindBufferRange = nullptr;
		PFNGLBINDBUFFERBASEPROC BindBufferBase = nullptr;
		PFNGLTRANSFORMFEEDBACKVARYINGSPROC TransformFeedbackVaryings = nullptr;
		PFNGLGETTRANSFORMFEEDBACKVARYINGPROC GetTransformFeedbackVarying = nullptr;
		PFNGLCLAMPCOLORPROC ClampColor = nullptr;
		PFNGLBEGINCONDITIONALRENDERPROC BeginConditionalRender = nullptr;
		PFNGLENDCONDITIONALRENDERPROC EndConditionalRender = nullptr;
		PFNGLVERTEXATTRIBIPOINTERPROC VertexAttribIPointer = nullptr;
		PFNGLGETVERTEXATTRIBIIVPROC GetVertexAttribIiv = nullptr;
		PFNGLGETVERTEXATTRIBIUIVPROC GetVertexAttribIuiv = nullptr;
		PFNGLVERTEXATTRIBI1IPROC VertexAttribI1i = nullptr;
		PFNGLVERTEXATTRIBI2IPROC VertexAttribI2i = nullptr;
		PFNGLVERTEXATTRIBI3IPROC VertexAttribI3i = nullptr;
		PFNGLVERTEXATTRIBI4IPROC VertexAttribI4i = nullptr;
		PFNGLVERTEXATTRIBI1UIPROC VertexAttribI1ui = nullptr;
		PFNGLVERTEXATTRIBI2UIPROC VertexAttribI2ui = nullptr;
		PFNGLVERTEXATTRIBI3UIPROC VertexAttribI3ui = nullptr;
		PFNGLVERTEXATTRIBI4UIPROC VertexAttribI4ui = nullptr;
		PFNGLVERTEXATTRIBI1IVPROC VertexAttribI1iv = nullptr;
		PFNGLVERTEXATTRIBI2IVPROC VertexAttribI2iv = nullptr;
		PFNGLVERTEXATTRIBI3IVPROC VertexAttribI3iv = nullptr;
		PFNGLVERTEXATTRIBI4IVPROC VertexAttribI4iv = nullptr;
		PFNGLVERTEXATTRIBI1UIVPROC VertexAttribI1uiv = nullptr;
		PFNGLVERTEXATTRIBI2UIVPROC VertexAttribI2uiv = nullptr;
		PFNGLVERTEXATTRIBI3UIVPROC VertexAttribI3uiv = nullptr;
		PFNGLVERTEXATTRIBI4UIVPROC VertexAttribI4uiv = nullptr;
		PFNGLVERTEXATTRIBI4BVPROC VertexAttribI4bv = nullptr;
		PFNGLVERTEXATTRIBI4SVPROC VertexAttribI4sv = nullptr;
		PFNGLVERTEXATTRIBI4UBVPROC VertexAttribI4ubv = nullptr;
		PFNGLVERTEXATTRIBI4USVPROC VertexAttribI4usv = nullptr;
		PFNGLGETUNIFORMUIVPROC GetUniformuiv = nullptr;
		PFNGLBINDFRAGDATALOCATIONPROC BindFragDataLocation = nullptr;
		PFNGLGETFRAGDATALOCATIONPROC GetFragDataLocation = nullptr;
		PFNGLUNIFORM1UIPROC Uniform1ui = nullptr;
		PFNGLUNIFORM2UIPROC Uniform2ui = nullptr;
		PFNGLUNIFORM3UIPROC Uniform3ui = nullptr;
		PFNGLUNIFORM4UIPROC Uniform4ui = nullptr;
		PFNGLUNIFORM1UIVPROC Uniform1uiv = nullptr;
		PFNGLUNIFORM2UIVPROC Uniform2uiv = nullptr;
		PFNGLUNIFORM3UIVPROC Uniform3uiv = nullptr;
		PFNGLUNIFORM4UIVPROC Uniform4uiv = nullptr;
		PFNGLTEXPARAMETERIIVPROC TexParameterIiv = nullptr;
		PFNGLTEXPARAMETERIUIVPROC TexParameterIuiv = nullptr;
		PFNGLGETTEXPARAMETERIIVPROC GetTexParameterIiv = nullptr;
		PFNGLGETTEXPARAMETERIUIVPROC GetTexParameterIuiv = nullptr;
		PFNGLCLEARBUFFERIVPROC ClearBufferiv = nullptr;
		PFNGLCLEARBUFFERUIVPROC ClearBufferuiv = nullptr;
		PFNGLCLEARBUFFERFVPROC ClearBufferfv = nullptr;
		PFNGLCLEARBUFFERFIPROC ClearBufferfi = nullptr;
		PFNGLGETSTRINGIPROC GetStringi = nullptr;
		PFNGLISRENDERBUFFERPROC IsRenderbuffer = nullptr;
		PFNGLBINDRENDERBUFFERPROC BindRenderbuffer = nullptr;
		PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers = nullptr;
		PFNGLGENRENDERBUFFERSPROC GenRenderbuffers = nullptr;
		PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage = nullptr;
		PFNGLGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv = nullptr;
		PFNGLISFRAMEBUFFERPROC IsFramebuffer = nullptr;
		PFNGLBINDFRAMEBUFFERPROC BindFramebuffer = nullptr;
		PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers = nullptr;
		PFNGLGENFRAMEBUFFERSPROC GenFramebuffers = nullptr;
		PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus = nullptr;
		PFNGLFRAMEBUFFERTEXTURE1DPROC FramebufferTexture1D = nullptr;
		PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = nullptr;
		PFNGLFRAMEBUFFERTEXTURE3DPROC FramebufferTexture3D = nullptr;
		PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer = nullptr;
		PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC GetFramebufferAttachmentParameteriv = nullptr;
		PFNGLGENERATEMIPMAPPROC GenerateMipmap = nullptr;
		PFNGLBLITFRAMEBUFFERPROC BlitFramebuffer = nullptr;
		PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC RenderbufferStorageMultisample = nullptr;
		PFNGLFRAMEBUFFERTEXTURELAYERPROC FramebufferTextureLayer = nullptr;
		PFNGLMAPBUFFERRANGEPROC MapBufferRange = nullptr;
		PFNGLFLUSHMAPPEDBUFFERRANGEPROC FlushMappedBufferRange = nullptr;
		PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
		PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = nullptr;
		PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
		PFNGLISVERTEXARRAYPROC IsVertexArray = nullptr;

		// 3.1
		PFNGLDRAWARRAYSINSTANCEDPROC DrawArraysInstanced = nullptr;
		PFNGLDRAWELEMENTSINSTANCEDPROC DrawElementsInstanced = nullptr;
		PFNGLTEXBUFFERPROC TexBuffer = nullptr;
		PFNGLPRIMITIVERESTARTINDEXPROC PrimitiveRestartIndex = nullptr;
		PFNGLCOPYBUFFERSUBDATAPROC CopyBufferSubData = nullptr;
		PFNGLGETUNIFORMINDICESPROC GetUniformIndices = nullptr;
		PFNGLGETACTIVEUNIFORMSIVPROC GetActiveUniformsiv = nullptr;
		PFNGLGETACTIVEUNIFORMNAMEPROC GetActiveUniformName = nullptr;
		PFNGLGETUNIFORMBLOCKINDEXPROC GetUniformBlockIndex = nullptr;
		PFNGLGETACTIVEUNIFORMBLOCKIVPROC GetActiveUniformBlockiv = nullptr;
		PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC GetActiveUniformBlockName = nullptr;
		PFNGLUNIFORMBLOCKBINDINGPROC UniformBlockBinding = nullptr;

		// 3.2
		PFNGLDRAWELEMENTSBASEVERTEXPROC DrawElementsBaseVertex = nullptr;
		PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC DrawRangeElementsBaseVertex = nullptr;
		PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC DrawElementsInstancedBaseVertex = nullptr;
		PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC MultiDrawElementsBaseVertex = nullptr;
		PFNGLPROVOKINGVERTEXPROC ProvokingVertex = nullptr;
		PFNGLFENCESYNCPROC FenceSync = nullptr;
		PFNGLISSYNCPROC IsSync = nullptr;
		PFNGLDELETESYNCPROC DeleteSync = nullptr;
		PFNGLCLIENTWAITSYNCPROC ClientWaitSync = nullptr;
		PFNGLWAITSYNCPROC WaitSync = nullptr;
		PFNGLGETINTEGER64VPROC GetInteger64v = nullptr;
		PFNGLGETSYNCIVPROC GetSynciv = nullptr;
		PFNGLGETINTEGER64I_VPROC GetInteger64i_v = nullptr;
		PFNGLGETBUFFERPARAMETERI64VPROC GetBufferParameteri64v = nullptr;
		PFNGLFRAMEBUFFERTEXTUREPROC FramebufferTexture = nullptr;
		PFNGLTEXIMAGE2DMULTISAMPLEPROC TexImage2DMultisample = nullptr;
		PFNGLTEXIMAGE3DMULTISAMPLEPROC TexImage3DMultisample = nullptr;
		PFNGLGETMULTISAMPLEFVPROC GetMultisamplefv = nullptr;
		PFNGLSAMPLEMASKIPROC SampleMaski = nullptr;

		// 3.3
		PFNGLBINDFRAGDATALOCATIONINDEXEDPROC BindFragDataLocationIndexed = nullptr;
		PFNGLGETFRAGDATAINDEXPROC GetFragDataIndex = nullptr;
		PFNGLGENSAMPLERSPROC GenSamplers = nullptr;
		PFNGLDELETESAMPLERSPROC DeleteSamplers = nullptr;
		PFNGLISSAMPLERPROC IsSampler = nullptr;
		PFNGLBINDSAMPLERPROC BindSampler = nullptr;
		PFNGLSAMPLERPARAMETERIPROC SamplerParameteri = nullptr;
		PFNGLSAMPLERPARAMETERIVPROC SamplerParameteriv = nullptr;
		PFNGLSAMPLERPARAMETERFPROC SamplerParameterf = nullptr;
		PFNGLSAMPLERPARAMETERFVPROC SamplerParameterfv = nullptr;
		PFNGLSAMPLERPARAMETERIIVPROC SamplerParameterIiv = nullptr;
		PFNGLSAMPLERPARAMETERIUIVPROC SamplerParameterIuiv = nullptr;
		PFNGLGETSAMPLERPARAMETERIVPROC GetSamplerParameteriv = nullptr;
		PFNGLGETSAMPLERPARAMETERIIVPROC GetSamplerParameterIiv = nullptr;
		PFNGLGETSAMPLERPARAMETERFVPROC GetSamplerParameterfv = nullptr;
		PFNGLGETSAMPLERPARAMETERIUIVPROC GetSamplerParameterIuiv = nullptr;
		PFNGLQUERYCOUNTERPROC QueryCounter = nullptr;
		PFNGLGETQUERYOBJECTI64VPROC GetQueryObjecti64v = nullptr;
		PFNGLGETQUERYOBJECTUI64VPROC GetQueryObjectui64v = nullptr;
		PFNGLVERTEXATTRIBDIVISORPROC VertexAttribDivisor = nullptr;
		PFNGLVERTEXATTRIBP1UIPROC VertexAttribP1ui = nullptr;
		PFNGLVERTEXATTRIBP1UIVPROC VertexAttribP1uiv = nullptr;
		PFNGLVERTEXATTRIBP2UIPROC VertexAttribP2ui = nullptr;
		PFNGLVERTEXATTRIBP2UIVPROC VertexAttribP2uiv = nullptr;
		PFNGLVERTEXATTRIBP3UIPROC VertexAttribP3ui = nullptr;
		PFNGLVERTEXATTRIBP3UIVPROC VertexAttribP3uiv = nullptr;
		PFNGLVERTEXATTRIBP4UIPROC VertexAttribP4ui = nullptr;
		PFNGLVERTEXATTRIBP4UIVPROC VertexAttribP4uiv = nullptr;

		// 4.0
		PFNGLMINSAMPLESHADINGPROC MinSampleShading = nullptr;
		PFNGLBLENDEQUATIONIPROC BlendEquationi = nullptr;
		PFNGLBLENDEQUATIONSEPARATEIPROC BlendEquationSeparatei = nullptr;
		PFNGLBLENDFUNCIPROC BlendFunci = nullptr;
		PFNGLBLENDFUNCSEPARATEIPROC BlendFuncSeparatei = nullptr;
		PFNGLDRAWARRAYSINDIRECTPROC DrawArraysIndirect = nullptr;
		PFNGLDRAWELEMENTSINDIRECTPROC DrawElementsIndirect = nullptr;
		PFNGLUNIFORM1DPROC Uniform1d = nullptr;
		PFNGLUNIFORM2DPROC Uniform2d = nullptr;
		PFNGLUNIFORM3DPROC Uniform3d = nullptr;
		PFNGLUNIFORM4DPROC Uniform4d = nullptr;
		PFNGLUNIFORM1DVPROC Uniform1dv = nullptr;
		PFNGLUNIFORM2DVPROC Uniform2dv = nullptr;
		PFNGLUNIFORM3DVPROC Uniform3dv = nullptr;
		PFNGLUNIFORM4DVPROC Uniform4dv = nullptr;
		PFNGLUNIFORMMATRIX2DVPROC UniformMatrix2dv = nullptr;
		PFNGLUNIFORMMATRIX3DVPROC UniformMatrix3dv = nullptr;
		PFNGLUNIFORMMATRIX4DVPROC UniformMatrix4dv = nullptr;
		PFNGLUNIFORMMATRIX2X3DVPROC UniformMatrix2x3dv = nullptr;
		PFNGLUNIFORMMATRIX2X4DVPROC UniformMatrix2x4dv = nullptr;
		PFNGLUNIFORMMATRIX3X2DVPROC UniformMatrix3x2dv = nullptr;
		PFNGLUNIFORMMATRIX3X4DVPROC UniformMatrix3x4dv = nullptr;
		PFNGLUNIFORMMATRIX4X2DVPROC UniformMatrix4x2dv = nullptr;
		PFNGLUNIFORMMATRIX4X3DVPROC UniformMatrix4x3dv = nullptr;
		PFNGLGETUNIFORMDVPROC GetUniformdv = nullptr;
		PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC GetSubroutineUniformLocation = nullptr;
		PFNGLGETSUBROUTINEINDEXPROC GetSubroutineIndex = nullptr;
		PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC GetActiveSubroutineUniformiv = nullptr;
		PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC GetActiveSubroutineUniformName = nullptr;
		PFNGLGETACTIVESUBROUTINENAMEPROC GetActiveSubroutineName = nullptr;
		PFNGLUNIFORMSUBROUTINESUIVPROC UniformSubroutinesuiv = nullptr;
		PFNGLGETUNIFORMSUBROUTINEUIVPROC GetUniformSubroutineuiv = nullptr;
		PFNGLGETPROGRAMSTAGEIVPROC GetProgramStageiv = nullptr;
		PFNGLPATCHPARAMETERIPROC PatchParameteri = nullptr;
		PFNGLPATCHPARAMETERFVPROC PatchParameterfv = nullptr;
		PFNGLBINDTRANSFORMFEEDBACKPROC BindTransformFeedback = nullptr;
		PFNGLDELETETRANSFORMFEEDBACKSPROC DeleteTransformFeedbacks = nullptr;
		PFNGLGENTRANSFORMFEEDBACKSPROC GenTransformFeedbacks = nullptr;
		PFNGLISTRANSFORMFEEDBACKPROC IsTransformFeedback = nullptr;
		PFNGLPAUSETRANSFORMFEEDBACKPROC PauseTransformFeedback = nullptr;
		PFNGLRESUMETRANSFORMFEEDBACKPROC ResumeTransformFeedback = nullptr;
		PFNGLDRAWTRANSFORMFEEDBACKPROC DrawTransformFeedback = nullptr;
		PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC DrawTransformFeedbackStream = nullptr;
		PFNGLBEGINQUERYINDEXEDPROC BeginQueryIndexed = nullptr;
		PFNGLENDQUERYINDEXEDPROC EndQueryIndexed = nullptr;
		PFNGLGETQUERYINDEXEDIVPROC GetQueryIndexediv = nullptr;

		// 4.1
		PFNGLRELEASESHADERCOMPILERPROC ReleaseShaderCompiler = nullptr;
		PFNGLSHADERBINARYPROC ShaderBinary = nullptr;
		PFNGLGETSHADERPRECISIONFORMATPROC GetShaderPrecisionFormat = nullptr;
		PFNGLDEPTHRANGEFPROC DepthRangef = nullptr;
		PFNGLCLEARDEPTHFPROC ClearDepthf = nullptr;
		PFNGLGETPROGRAMBINARYPROC GetProgramBinary = nullptr;
		PFNGLPROGRAMBINARYPROC ProgramBinary = nullptr;
		PFNGLPROGRAMPARAMETERIPROC ProgramParameteri = nullptr;
		PFNGLUSEPROGRAMSTAGESPROC UseProgramStages = nullptr;
		PFNGLACTIVESHADERPROGRAMPROC ActiveShaderProgram = nullptr;
		PFNGLCREATESHADERPROGRAMVPROC CreateShaderProgramv = nullptr;
		PFNGLBINDPROGRAMPIPELINEPROC BindProgramPipeline = nullptr;
		PFNGLDELETEPROGRAMPIPELINESPROC DeleteProgramPipelines = nullptr;
		PFNGLGENPROGRAMPIPELINESPROC GenProgramPipelines = nullptr;
		PFNGLISPROGRAMPIPELINEPROC IsProgramPipeline = nullptr;
		PFNGLGETPROGRAMPIPELINEIVPROC GetProgramPipelineiv = nullptr;
		PFNGLPROGRAMUNIFORM1IPROC ProgramUniform1i = nullptr;
		PFNGLPROGRAMUNIFORM1IVPROC ProgramUniform1iv = nullptr;
		PFNGLPROGRAMUNIFORM1FPROC ProgramUniform1f = nullptr;
		PFNGLPROGRAMUNIFORM1FVPROC ProgramUniform1fv = nullptr;
		PFNGLPROGRAMUNIFORM1DPROC ProgramUniform1d = nullptr;
		PFNGLPROGRAMUNIFORM1DVPROC ProgramUniform1dv = nullptr;
		PFNGLPROGRAMUNIFORM1UIPROC ProgramUniform1ui = nullptr;
		PFNGLPROGRAMUNIFORM1UIVPROC ProgramUniform1uiv = nullptr;
		PFNGLPROGRAMUNIFORM2IPROC ProgramUniform2i = nullptr;
		PFNGLPROGRAMUNIFORM2IVPROC ProgramUniform2iv = nullptr;
		PFNGLPROGRAMUNIFORM2FPROC ProgramUniform2f = nullptr;
		PFNGLPROGRAMUNIFORM2FVPROC ProgramUniform2fv = nullptr;
		PFNGLPROGRAMUNIFORM2DPROC ProgramUniform2d = nullptr;
		PFNGLPROGRAMUNIFORM2DVPROC ProgramUniform2dv = nullptr;
		PFNGLPROGRAMUNIFORM2UIPROC ProgramUniform2ui = nullptr;
		PFNGLPROGRAMUNIFORM2UIVPROC ProgramUniform2uiv = nullptr;
		PFNGLPROGRAMUNIFORM3IPROC ProgramUniform3i = nullptr;
		PFNGLPROGRAMUNIFORM3IVPROC ProgramUniform3iv = nullptr;
		PFNGLPROGRAMUNIFORM3FPROC ProgramUniform3f = nullptr;
		PFNGLPROGRAMUNIFORM3FVPROC ProgramUniform3fv = nullptr;
		PFNGLPROGRAMUNIFORM3DPROC ProgramUniform3d = nullptr;
		PFNGLPROGRAMUNIFORM3DVPROC ProgramUniform3dv = nullptr;
		PFNGLPROGRAMUNIFORM3UIPROC ProgramUniform3ui = nullptr;
		PFNGLPROGRAMUNIFORM3UIVPROC ProgramUniform3uiv = nullptr;
		PFNGLPROGRAMUNIFORM4IPROC ProgramUniform4i = nullptr;
		PFNGLPROGRAMUNIFORM4IVPROC ProgramUniform4iv = nullptr;
		PFNGLPROGRAMUNIFORM4FPROC ProgramUniform4f = nullptr;
		PFNGLPROGRAMUNIFORM4FVPROC ProgramUniform4fv = nullptr;
		PFNGLPROGRAMUNIFORM4DPROC ProgramUniform4d = nullptr;
		PFNGLPROGRAMUNIFORM4DVPROC ProgramUniform4dv = nullptr;
		PFNGLPROGRAMUNIFORM4UIPROC ProgramUniform4ui = nullptr;
		PFNGLPROGRAMUNIFORM4UIVPROC ProgramUniform4uiv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX2FVPROC ProgramUniformMatrix2fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX3FVPROC ProgramUniformMatrix3fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX4FVPROC ProgramUniformMatrix4fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX2DVPROC ProgramUniformMatrix2dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX3DVPROC ProgramUniformMatrix3dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX4DVPROC ProgramUniformMatrix4dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC ProgramUniformMatrix2x3fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC ProgramUniformMatrix3x2fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC ProgramUniformMatrix2x4fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC ProgramUniformMatrix4x2fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC ProgramUniformMatrix3x4fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC ProgramUniformMatrix4x3fv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC ProgramUniformMatrix2x3dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC ProgramUniformMatrix3x2dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC ProgramUniformMatrix2x4dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC ProgramUniformMatrix4x2dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC ProgramUniformMatrix3x4dv = nullptr;
		PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC ProgramUniformMatrix4x3dv = nullptr;
		PFNGLVALIDATEPROGRAMPIPELINEPROC ValidateProgramPipeline = nullptr;
		PFNGLGETPROGRAMPIPELINEINFOLOGPROC GetProgramPipelineInfoLog = nullptr;
		PFNGLVERTEXATTRIBL1DPROC VertexAttribL1d = nullptr;
		PFNGLVERTEXATTRIBL2DPROC VertexAttribL2d = nullptr;
		PFNGLVERTEXATTRIBL3DPROC VertexAttribL3d = nullptr;
		PFNGLVERTEXATTRIBL4DPROC VertexAttribL4d = nullptr;
		PFNGLVERTEXATTRIBL1DVPROC VertexAttribL1dv = nullptr;
		PFNGLVERTEXATTRIBL2DVPROC VertexAttribL2dv = nullptr;
		PFNGLVERTEXATTRIBL3DVPROC VertexAttribL3dv = nullptr;
		PFNGLVERTEXATTRIBL4DVPROC VertexAttribL4dv = nullptr;
		PFNGLVERTEXATTRIBLPOINTERPROC VertexAttribLPointer = nullptr;
		PFNGLGETVERTEXATTRIBLDVPROC GetVertexAttribLdv = nullptr;
		PFNGLVIEWPORTARRAYVPROC ViewportArrayv = nullptr;
		PFNGLVIEWPORTINDEXEDFPROC ViewportIndexedf = nullptr;
		PFNGLVIEWPORTINDEXEDFVPROC ViewportIndexedfv = nullptr;
		PFNGLSCISSORARRAYVPROC ScissorArrayv = nullptr;
		PFNGLSCISSORINDEXEDPROC ScissorIndexed = nullptr;
		PFNGLSCISSORINDEXEDVPROC ScissorIndexedv = nullptr;
		PFNGLDEPTHRANGEARRAYVPROC DepthRangeArrayv = nullptr;
		PFNGLDEPTHRANGEINDEXEDPROC DepthRangeIndexed = nullptr;
		PFNGLGETFLOATI_VPROC GetFloati_v = nullptr;
		PFNGLGETDOUBLEI_VPROC GetDoublei_v = nullptr;
	}

// Binding
#if RN_PLATFORM_MAC_OS
	#define BindOpenGL(name) gl::name = reinterpret_cast<decltype(gl::name)>(dlsym(RTLD_DEFAULT, "gl" #name))
	#define BindOpenGLExtension(name, ext) gl::name = reinterpret_cast<decltype(gl::name)>(dlsym(RTLD_DEFAULT, "gl" #name ext))
#endif
	
#if RN_PLATFORM_WINDOWS
	#define BindOpenGL(name) gl::name = reinterpret_cast<decltype(gl::name)>(wglGetProcAddress("gl" #name))
	#define BindOpenGLExtension(name, ext) gl::name = reinterpret_cast<decltype(gl::name)>(wglGetProcAddress("gl" #name ext))
#endif
	
#if RN_PLATFORM_LINUX
	#define BindOpenGL(name) gl::name = reinterpret_cast<decltype(gl::name)>(glXGetProcAddressARB(reintrpret_cast<const GLubyte *>("gl" #name)))
	#define BindOpenGLExtension(name, ext) gl::name = reinterpret_cast<decltype(gl::name)>(glXGetProcAddressARB(reintrpret_cast<const GLubyte *>("gl" #name ext)))
#endif
	
	void BindOpenGLCore()
	{
		// 1.0
		BindOpenGL(CullFace);
		BindOpenGL(FrontFace);
		BindOpenGL(Hint);
		BindOpenGL(LineWidth);
		BindOpenGL(PointSize);
		BindOpenGL(PolygonMode);
		BindOpenGL(Scissor);
		BindOpenGL(TexParameterf);
		BindOpenGL(TexParameterfv);
		BindOpenGL(TexParameteri);
		BindOpenGL(TexParameteriv);
		BindOpenGL(TexImage1D);
		BindOpenGL(TexImage2D);
		BindOpenGL(DrawBuffer);
		BindOpenGL(Clear);
		BindOpenGL(ClearColor);
		BindOpenGL(ClearStencil);
		BindOpenGL(ClearDepth);
		BindOpenGL(StencilMask);
		BindOpenGL(ColorMask);
		BindOpenGL(DepthMask);
		BindOpenGL(Disable);
		BindOpenGL(Enable);
		BindOpenGL(Finish);
		BindOpenGL(Flush);
		BindOpenGL(BlendFunc);
		BindOpenGL(LogicOp);
		BindOpenGL(StencilFunc);
		BindOpenGL(StencilOp);
		BindOpenGL(DepthFunc);
		BindOpenGL(PixelStoref);
		BindOpenGL(PixelStorei);
		BindOpenGL(ReadBuffer);
		BindOpenGL(ReadPixels);
		BindOpenGL(GetBooleanv);
		BindOpenGL(GetDoublev);
		BindOpenGL(GetError);
		BindOpenGL(GetFloatv);
		BindOpenGL(GetIntegerv);
		BindOpenGL(GetString);
		BindOpenGL(GetTexImage);
		BindOpenGL(GetTexParameterfv);
		BindOpenGL(GetTexParameteriv);
		BindOpenGL(GetTexLevelParameterfv);
		BindOpenGL(GetTexLevelParameteriv);
		BindOpenGL(IsEnabled);
		BindOpenGL(DepthRange);
		BindOpenGL(Viewport);
		
		// 1.1
		BindOpenGL(DrawArrays);
		BindOpenGL(DrawElements);
		BindOpenGL(GetPointerv);
		BindOpenGL(PolygonOffset);
		BindOpenGL(CopyTexImage1D);
		BindOpenGL(CopyTexImage2D);
		BindOpenGL(CopyTexSubImage1D);
		BindOpenGL(CopyTexSubImage2D);
		BindOpenGL(TexSubImage1D);
		BindOpenGL(TexSubImage2D);
		BindOpenGL(BindTexture);
		BindOpenGL(DeleteTextures);
		BindOpenGL(GenTextures);
		BindOpenGL(IsTexture);
		
		// 1.2
		BindOpenGL(DrawRangeElements);
		BindOpenGL(TexImage3D);
		BindOpenGL(TexSubImage3D);
		BindOpenGL(CopyTexSubImage3D);
		
		// 1.3
		BindOpenGL(ActiveTexture);
		BindOpenGL(SampleCoverage);
		BindOpenGL(CompressedTexImage3D);
		BindOpenGL(CompressedTexImage2D);
		BindOpenGL(CompressedTexImage1D);
		BindOpenGL(CompressedTexSubImage3D);
		BindOpenGL(CompressedTexSubImage2D);
		BindOpenGL(CompressedTexSubImage1D);
		BindOpenGL(GetCompressedTexImage);
		
		// 1.4
		BindOpenGL(BlendFuncSeparate);
		BindOpenGL(MultiDrawArrays);
		BindOpenGL(MultiDrawElements);
		BindOpenGL(PointParameterf);
		BindOpenGL(PointParameterfv);
		BindOpenGL(PointParameteri);
		BindOpenGL(PointParameteriv);
		BindOpenGL(BlendColor);
		BindOpenGL(BlendEquation);
		
		// 1.5
		BindOpenGL(GenQueries);
		BindOpenGL(DeleteQueries);
		BindOpenGL(IsQuery);
		BindOpenGL(BeginQuery);
		BindOpenGL(EndQuery);
		BindOpenGL(GetQueryiv);
		BindOpenGL(GetQueryObjectiv);
		BindOpenGL(GetQueryObjectuiv);
		BindOpenGL(BindBuffer);
		BindOpenGL(DeleteBuffers);
		BindOpenGL(GenBuffers);
		BindOpenGL(IsBuffer);
		BindOpenGL(BufferData);
		BindOpenGL(BufferSubData);
		BindOpenGL(GetBufferSubData);
		BindOpenGL(MapBuffer);
		BindOpenGL(UnmapBuffer);
		BindOpenGL(GetBufferParameteriv);
		BindOpenGL(GetBufferPointerv);
		
		// 2.0
		BindOpenGL(BlendEquationSeparate);
		BindOpenGL(DrawBuffers);
		BindOpenGL(StencilOpSeparate);
		BindOpenGL(StencilFuncSeparate);
		BindOpenGL(StencilMaskSeparate);
		BindOpenGL(AttachShader);
		BindOpenGL(BindAttribLocation);
		BindOpenGL(CompileShader);
		BindOpenGL(CreateProgram);
		BindOpenGL(CreateShader);
		BindOpenGL(DeleteProgram);
		BindOpenGL(DeleteShader);
		BindOpenGL(DetachShader);
		BindOpenGL(DisableVertexAttribArray);
		BindOpenGL(EnableVertexAttribArray);
		BindOpenGL(GetActiveAttrib);
		BindOpenGL(GetActiveUniform);
		BindOpenGL(GetAttachedShaders);
		BindOpenGL(GetAttribLocation);
		BindOpenGL(GetProgramiv);
		BindOpenGL(GetProgramInfoLog);
		BindOpenGL(GetShaderiv);
		BindOpenGL(GetShaderInfoLog);
		BindOpenGL(GetShaderSource);
		BindOpenGL(GetUniformLocation);
		BindOpenGL(GetUniformfv);
		BindOpenGL(GetUniformiv);
		BindOpenGL(GetVertexAttribdv);
		BindOpenGL(GetVertexAttribfv);
		BindOpenGL(GetVertexAttribiv);
		BindOpenGL(GetVertexAttribPointerv);
		BindOpenGL(IsProgram);
		BindOpenGL(IsShader);
		BindOpenGL(LinkProgram);
		BindOpenGL(ShaderSource);
		BindOpenGL(UseProgram);
		BindOpenGL(Uniform1f);
		BindOpenGL(Uniform2f);
		BindOpenGL(Uniform3f);
		BindOpenGL(Uniform4f);
		BindOpenGL(Uniform1i);
		BindOpenGL(Uniform2i);
		BindOpenGL(Uniform3i);
		BindOpenGL(Uniform4i);
		BindOpenGL(Uniform1fv);
		BindOpenGL(Uniform2fv);
		BindOpenGL(Uniform3fv);
		BindOpenGL(Uniform4fv);
		BindOpenGL(Uniform1iv);
		BindOpenGL(Uniform2iv);
		BindOpenGL(Uniform3iv);
		BindOpenGL(Uniform4iv);
		BindOpenGL(UniformMatrix2fv);
		BindOpenGL(UniformMatrix3fv);
		BindOpenGL(UniformMatrix4fv);
		BindOpenGL(ValidateProgram);
		BindOpenGL(VertexAttrib1d);
		BindOpenGL(VertexAttrib1dv);
		BindOpenGL(VertexAttrib1f);
		BindOpenGL(VertexAttrib1fv);
		BindOpenGL(VertexAttrib1s);
		BindOpenGL(VertexAttrib1sv);
		BindOpenGL(VertexAttrib2d);
		BindOpenGL(VertexAttrib2dv);
		BindOpenGL(VertexAttrib2f);
		BindOpenGL(VertexAttrib2fv);
		BindOpenGL(VertexAttrib2s);
		BindOpenGL(VertexAttrib2sv);
		BindOpenGL(VertexAttrib3d);
		BindOpenGL(VertexAttrib3dv);
		BindOpenGL(VertexAttrib3f);
		BindOpenGL(VertexAttrib3fv);
		BindOpenGL(VertexAttrib3s);
		BindOpenGL(VertexAttrib3sv);
		BindOpenGL(VertexAttrib4Nbv);
		BindOpenGL(VertexAttrib4Niv);
		BindOpenGL(VertexAttrib4Nsv);
		BindOpenGL(VertexAttrib4Nub);
		BindOpenGL(VertexAttrib4Nubv);
		BindOpenGL(VertexAttrib4Nuiv);
		BindOpenGL(VertexAttrib4Nusv);
		BindOpenGL(VertexAttrib4bv);
		BindOpenGL(VertexAttrib4d);
		BindOpenGL(VertexAttrib4dv);
		BindOpenGL(VertexAttrib4f);
		BindOpenGL(VertexAttrib4fv);
		BindOpenGL(VertexAttrib4iv);
		BindOpenGL(VertexAttrib4s);
		BindOpenGL(VertexAttrib4sv);
		BindOpenGL(VertexAttrib4ubv);
		BindOpenGL(VertexAttrib4uiv);
		BindOpenGL(VertexAttrib4usv);
		BindOpenGL(VertexAttribPointer);
		
		// 2.1
		BindOpenGL(UniformMatrix2x3fv);
		BindOpenGL(UniformMatrix3x2fv);
		BindOpenGL(UniformMatrix2x4fv);
		BindOpenGL(UniformMatrix4x2fv);
		BindOpenGL(UniformMatrix3x4fv);
		BindOpenGL(UniformMatrix4x3fv);
	}
	
	void BindOpenGLFunctions(gl::Version version)
	{
		if(version >= gl::Version::Core3_2)
		{
			// 3.0
			BindOpenGL(ColorMaski);
			BindOpenGL(GetBooleani_v);
			BindOpenGL(GetIntegeri_v);
			BindOpenGL(Enablei);
			BindOpenGL(Disablei);
			BindOpenGL(IsEnabledi);
			BindOpenGL(BeginTransformFeedback);
			BindOpenGL(EndTransformFeedback);
			BindOpenGL(BindBufferRange);
			BindOpenGL(BindBufferBase);
			BindOpenGL(TransformFeedbackVaryings);
			BindOpenGL(GetTransformFeedbackVarying);
			BindOpenGL(ClampColor);
			BindOpenGL(BeginConditionalRender);
			BindOpenGL(EndConditionalRender);
			BindOpenGL(VertexAttribIPointer);
			BindOpenGL(GetVertexAttribIiv);
			BindOpenGL(GetVertexAttribIuiv);
			BindOpenGL(VertexAttribI1i);
			BindOpenGL(VertexAttribI2i);
			BindOpenGL(VertexAttribI3i);
			BindOpenGL(VertexAttribI4i);
			BindOpenGL(VertexAttribI1ui);
			BindOpenGL(VertexAttribI2ui);
			BindOpenGL(VertexAttribI3ui);
			BindOpenGL(VertexAttribI4ui);
			BindOpenGL(VertexAttribI1iv);
			BindOpenGL(VertexAttribI2iv);
			BindOpenGL(VertexAttribI3iv);
			BindOpenGL(VertexAttribI4iv);
			BindOpenGL(VertexAttribI1uiv);
			BindOpenGL(VertexAttribI2uiv);
			BindOpenGL(VertexAttribI3uiv);
			BindOpenGL(VertexAttribI4uiv);
			BindOpenGL(VertexAttribI4bv);
			BindOpenGL(VertexAttribI4sv);
			BindOpenGL(VertexAttribI4ubv);
			BindOpenGL(VertexAttribI4usv);
			BindOpenGL(GetUniformuiv);
			BindOpenGL(BindFragDataLocation);
			BindOpenGL(GetFragDataLocation);
			BindOpenGL(Uniform1ui);
			BindOpenGL(Uniform2ui);
			BindOpenGL(Uniform3ui);
			BindOpenGL(Uniform4ui);
			BindOpenGL(Uniform1uiv);
			BindOpenGL(Uniform2uiv);
			BindOpenGL(Uniform3uiv);
			BindOpenGL(Uniform4uiv);
			BindOpenGL(TexParameterIiv);
			BindOpenGL(TexParameterIuiv);
			BindOpenGL(GetTexParameterIiv);
			BindOpenGL(GetTexParameterIuiv);
			BindOpenGL(ClearBufferiv);
			BindOpenGL(ClearBufferuiv);
			BindOpenGL(ClearBufferfv);
			BindOpenGL(ClearBufferfi);
			BindOpenGL(GetStringi);
			BindOpenGL(IsRenderbuffer);
			BindOpenGL(BindRenderbuffer);
			BindOpenGL(DeleteRenderbuffers);
			BindOpenGL(GenRenderbuffers);
			BindOpenGL(RenderbufferStorage);
			BindOpenGL(GetRenderbufferParameteriv);
			BindOpenGL(IsFramebuffer);
			BindOpenGL(BindFramebuffer);
			BindOpenGL(DeleteFramebuffers);
			BindOpenGL(GenFramebuffers);
			BindOpenGL(CheckFramebufferStatus);
			BindOpenGL(FramebufferTexture1D);
			BindOpenGL(FramebufferTexture2D);
			BindOpenGL(FramebufferTexture3D);
			BindOpenGL(FramebufferRenderbuffer);
			BindOpenGL(GetFramebufferAttachmentParameteriv);
			BindOpenGL(GenerateMipmap);
			BindOpenGL(BlitFramebuffer);
			BindOpenGL(RenderbufferStorageMultisample);
			BindOpenGL(FramebufferTextureLayer);
			BindOpenGL(MapBufferRange);
			BindOpenGL(FlushMappedBufferRange);
			BindOpenGL(BindVertexArray);
			BindOpenGL(DeleteVertexArrays);
			BindOpenGL(GenVertexArrays);
			BindOpenGL(IsVertexArray);

			// 3.1
			BindOpenGL(DrawArraysInstanced);
			BindOpenGL(DrawElementsInstanced);
			BindOpenGL(TexBuffer);
			BindOpenGL(PrimitiveRestartIndex);
			BindOpenGL(CopyBufferSubData);
			BindOpenGL(GetUniformIndices);
			BindOpenGL(GetActiveUniformsiv);
			BindOpenGL(GetActiveUniformName);
			BindOpenGL(GetUniformBlockIndex);
			BindOpenGL(GetActiveUniformBlockiv);
			BindOpenGL(GetActiveUniformBlockName);
			BindOpenGL(UniformBlockBinding);

			// 3.2
			BindOpenGL(DrawElementsBaseVertex);
			BindOpenGL(DrawRangeElementsBaseVertex);
			BindOpenGL(DrawElementsInstancedBaseVertex);
			BindOpenGL(MultiDrawElementsBaseVertex);
			BindOpenGL(ProvokingVertex);
			BindOpenGL(FenceSync);
			BindOpenGL(IsSync);
			BindOpenGL(DeleteSync);
			BindOpenGL(ClientWaitSync);
			BindOpenGL(WaitSync);
			BindOpenGL(GetInteger64v);
			BindOpenGL(GetSynciv);
			BindOpenGL(GetInteger64i_v);
			BindOpenGL(GetBufferParameteri64v);
			BindOpenGL(FramebufferTexture);
			BindOpenGL(TexImage2DMultisample);
			BindOpenGL(TexImage3DMultisample);
			BindOpenGL(GetMultisamplefv);
			BindOpenGL(SampleMaski);
		}

		if(version >= gl::Version::Core4_1)
		{
			// 3.3
			BindOpenGL(BindFragDataLocationIndexed);
			BindOpenGL(GetFragDataIndex);
			BindOpenGL(GenSamplers);
			BindOpenGL(DeleteSamplers);
			BindOpenGL(IsSampler);
			BindOpenGL(BindSampler);
			BindOpenGL(SamplerParameteri);
			BindOpenGL(SamplerParameteriv);
			BindOpenGL(SamplerParameterf);
			BindOpenGL(SamplerParameterfv);
			BindOpenGL(SamplerParameterIiv);
			BindOpenGL(SamplerParameterIuiv);
			BindOpenGL(GetSamplerParameteriv);
			BindOpenGL(GetSamplerParameterIiv);
			BindOpenGL(GetSamplerParameterfv);
			BindOpenGL(GetSamplerParameterIuiv);
			BindOpenGL(QueryCounter);
			BindOpenGL(GetQueryObjecti64v);
			BindOpenGL(GetQueryObjectui64v);
			BindOpenGL(VertexAttribDivisor);
			BindOpenGL(VertexAttribP1ui);
			BindOpenGL(VertexAttribP1uiv);
			BindOpenGL(VertexAttribP2ui);
			BindOpenGL(VertexAttribP2uiv);
			BindOpenGL(VertexAttribP3ui);
			BindOpenGL(VertexAttribP3uiv);
			BindOpenGL(VertexAttribP4ui);
			BindOpenGL(VertexAttribP4uiv);

			// 4.0
			BindOpenGL(MinSampleShading);
			BindOpenGL(BlendEquationi);
			BindOpenGL(BlendEquationSeparatei);
			BindOpenGL(BlendFunci);
			BindOpenGL(BlendFuncSeparatei);
			BindOpenGL(DrawArraysIndirect);
			BindOpenGL(DrawElementsIndirect);
			BindOpenGL(Uniform1d);
			BindOpenGL(Uniform2d);
			BindOpenGL(Uniform3d);
			BindOpenGL(Uniform4d);
			BindOpenGL(Uniform1dv);
			BindOpenGL(Uniform2dv);
			BindOpenGL(Uniform3dv);
			BindOpenGL(Uniform4dv);
			BindOpenGL(UniformMatrix2dv);
			BindOpenGL(UniformMatrix3dv);
			BindOpenGL(UniformMatrix4dv);
			BindOpenGL(UniformMatrix2x3dv);
			BindOpenGL(UniformMatrix2x4dv);
			BindOpenGL(UniformMatrix3x2dv);
			BindOpenGL(UniformMatrix3x4dv);
			BindOpenGL(UniformMatrix4x2dv);
			BindOpenGL(UniformMatrix4x3dv);
			BindOpenGL(GetUniformdv);
			BindOpenGL(GetSubroutineUniformLocation);
			BindOpenGL(GetSubroutineIndex);
			BindOpenGL(GetActiveSubroutineUniformiv);
			BindOpenGL(GetActiveSubroutineUniformName);
			BindOpenGL(GetActiveSubroutineName);
			BindOpenGL(UniformSubroutinesuiv);
			BindOpenGL(GetUniformSubroutineuiv);
			BindOpenGL(GetProgramStageiv);
			BindOpenGL(PatchParameteri);
			BindOpenGL(PatchParameterfv);
			BindOpenGL(BindTransformFeedback);
			BindOpenGL(DeleteTransformFeedbacks);
			BindOpenGL(GenTransformFeedbacks);
			BindOpenGL(IsTransformFeedback);
			BindOpenGL(PauseTransformFeedback);
			BindOpenGL(ResumeTransformFeedback);
			BindOpenGL(DrawTransformFeedback);
			BindOpenGL(DrawTransformFeedbackStream);
			BindOpenGL(BeginQueryIndexed);
			BindOpenGL(EndQueryIndexed);
			BindOpenGL(GetQueryIndexediv);

			// 4.1
			BindOpenGL(ReleaseShaderCompiler);
			BindOpenGL(ShaderBinary);
			BindOpenGL(GetShaderPrecisionFormat);
			BindOpenGL(DepthRangef);
			BindOpenGL(ClearDepthf);
			BindOpenGL(GetProgramBinary);
			BindOpenGL(ProgramBinary);
			BindOpenGL(ProgramParameteri);
			BindOpenGL(UseProgramStages);
			BindOpenGL(ActiveShaderProgram);
			BindOpenGL(CreateShaderProgramv);
			BindOpenGL(BindProgramPipeline);
			BindOpenGL(DeleteProgramPipelines);
			BindOpenGL(GenProgramPipelines);
			BindOpenGL(IsProgramPipeline);
			BindOpenGL(GetProgramPipelineiv);
			BindOpenGL(ProgramUniform1i);
			BindOpenGL(ProgramUniform1iv);
			BindOpenGL(ProgramUniform1f);
			BindOpenGL(ProgramUniform1fv);
			BindOpenGL(ProgramUniform1d);
			BindOpenGL(ProgramUniform1dv);
			BindOpenGL(ProgramUniform1ui);
			BindOpenGL(ProgramUniform1uiv);
			BindOpenGL(ProgramUniform2i);
			BindOpenGL(ProgramUniform2iv);
			BindOpenGL(ProgramUniform2f);
			BindOpenGL(ProgramUniform2fv);
			BindOpenGL(ProgramUniform2d);
			BindOpenGL(ProgramUniform2dv);
			BindOpenGL(ProgramUniform2ui);
			BindOpenGL(ProgramUniform2uiv);
			BindOpenGL(ProgramUniform3i);
			BindOpenGL(ProgramUniform3iv);
			BindOpenGL(ProgramUniform3f);
			BindOpenGL(ProgramUniform3fv);
			BindOpenGL(ProgramUniform3d);
			BindOpenGL(ProgramUniform3dv);
			BindOpenGL(ProgramUniform3ui);
			BindOpenGL(ProgramUniform3uiv);
			BindOpenGL(ProgramUniform4i);
			BindOpenGL(ProgramUniform4iv);
			BindOpenGL(ProgramUniform4f);
			BindOpenGL(ProgramUniform4fv);
			BindOpenGL(ProgramUniform4d);
			BindOpenGL(ProgramUniform4dv);
			BindOpenGL(ProgramUniform4ui);
			BindOpenGL(ProgramUniform4uiv);
			BindOpenGL(ProgramUniformMatrix2fv);
			BindOpenGL(ProgramUniformMatrix3fv);
			BindOpenGL(ProgramUniformMatrix4fv);
			BindOpenGL(ProgramUniformMatrix2dv);
			BindOpenGL(ProgramUniformMatrix3dv);
			BindOpenGL(ProgramUniformMatrix4dv);
			BindOpenGL(ProgramUniformMatrix2x3fv);
			BindOpenGL(ProgramUniformMatrix3x2fv);
			BindOpenGL(ProgramUniformMatrix2x4fv);
			BindOpenGL(ProgramUniformMatrix4x2fv);
			BindOpenGL(ProgramUniformMatrix3x4fv);
			BindOpenGL(ProgramUniformMatrix4x3fv);
			BindOpenGL(ProgramUniformMatrix2x3dv);
			BindOpenGL(ProgramUniformMatrix3x2dv);
			BindOpenGL(ProgramUniformMatrix2x4dv);
			BindOpenGL(ProgramUniformMatrix4x2dv);
			BindOpenGL(ProgramUniformMatrix3x4dv);
			BindOpenGL(ProgramUniformMatrix4x3dv);
			BindOpenGL(ValidateProgramPipeline);
			BindOpenGL(GetProgramPipelineInfoLog);
			BindOpenGL(VertexAttribL1d);
			BindOpenGL(VertexAttribL2d);
			BindOpenGL(VertexAttribL3d);
			BindOpenGL(VertexAttribL4d);
			BindOpenGL(VertexAttribL1dv);
			BindOpenGL(VertexAttribL2dv);
			BindOpenGL(VertexAttribL3dv);
			BindOpenGL(VertexAttribL4dv);
			BindOpenGL(VertexAttribLPointer);
			BindOpenGL(GetVertexAttribLdv);
			BindOpenGL(ViewportArrayv);
			BindOpenGL(ViewportIndexedf);
			BindOpenGL(ViewportIndexedfv);
			BindOpenGL(ScissorArrayv);
			BindOpenGL(ScissorIndexed);
			BindOpenGL(ScissorIndexedv);
			BindOpenGL(DepthRangeArrayv);
			BindOpenGL(DepthRangeIndexed);
			BindOpenGL(GetFloati_v);
			BindOpenGL(GetDoublei_v);
		}
	}
	
	void BindOpenGLExtensions(gl::Version version)
	{
		if(version < gl::Version::Core4_1)
		{
			if(gl::SupportsFeature(gl::Feature::ShaderBinary))
			{
				BindOpenGLExtension(GetProgramBinary, "ARB");
				BindOpenGLExtension(ProgramBinary, "ARB");
				BindOpenGLExtension(ProgramParameteri, "ARB");
			}
		}
	}
}
