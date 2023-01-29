#ifndef glxw_es3_h
#define glxw_es3_h

#include <GLES3/gl3.h>
#include <GLES3/gl3platform.h>


#ifndef __gl_h_
#define __gl_h_
#endif

#ifdef __cplusplus
extern "C" {
#endif

int load_gles3_procs(void);

struct glxw_es3 {
    PFNGLACTIVETEXTUREPROC _glActiveTexture;
    PFNGLATTACHSHADERPROC _glAttachShader;
    PFNGLBINDATTRIBLOCATIONPROC _glBindAttribLocation;
    PFNGLBINDBUFFERPROC _glBindBuffer;
    PFNGLBINDFRAMEBUFFERPROC _glBindFramebuffer;
    PFNGLBINDRENDERBUFFERPROC _glBindRenderbuffer;
    PFNGLBINDTEXTUREPROC _glBindTexture;
    PFNGLBLENDCOLORPROC _glBlendColor;
    PFNGLBLENDEQUATIONPROC _glBlendEquation;
    PFNGLBLENDEQUATIONSEPARATEPROC _glBlendEquationSeparate;
    PFNGLBLENDFUNCPROC _glBlendFunc;
    PFNGLBLENDFUNCSEPARATEPROC _glBlendFuncSeparate;
    PFNGLBUFFERDATAPROC _glBufferData;
    PFNGLBUFFERSUBDATAPROC _glBufferSubData;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC _glCheckFramebufferStatus;
    PFNGLCLEARPROC _glClear;
    PFNGLCLEARCOLORPROC _glClearColor;
    PFNGLCLEARDEPTHFPROC _glClearDepthf;
    PFNGLCLEARSTENCILPROC _glClearStencil;
    PFNGLCOLORMASKPROC _glColorMask;
    PFNGLCOMPILESHADERPROC _glCompileShader;
    PFNGLCOMPRESSEDTEXIMAGE2DPROC _glCompressedTexImage2D;
    PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC _glCompressedTexSubImage2D;
    PFNGLCOPYTEXIMAGE2DPROC _glCopyTexImage2D;
    PFNGLCOPYTEXSUBIMAGE2DPROC _glCopyTexSubImage2D;
    PFNGLCREATEPROGRAMPROC _glCreateProgram;
    PFNGLCREATESHADERPROC _glCreateShader;
    PFNGLCULLFACEPROC _glCullFace;
    PFNGLDELETEBUFFERSPROC _glDeleteBuffers;
    PFNGLDELETEFRAMEBUFFERSPROC _glDeleteFramebuffers;
    PFNGLDELETEPROGRAMPROC _glDeleteProgram;
    PFNGLDELETERENDERBUFFERSPROC _glDeleteRenderbuffers;
    PFNGLDELETESHADERPROC _glDeleteShader;
    PFNGLDELETETEXTURESPROC _glDeleteTextures;
    PFNGLDEPTHFUNCPROC _glDepthFunc;
    PFNGLDEPTHMASKPROC _glDepthMask;
    PFNGLDEPTHRANGEFPROC _glDepthRangef;
    PFNGLDETACHSHADERPROC _glDetachShader;
    PFNGLDISABLEPROC _glDisable;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC _glDisableVertexAttribArray;
    PFNGLDRAWARRAYSPROC _glDrawArrays;
    PFNGLDRAWELEMENTSPROC _glDrawElements;
    PFNGLENABLEPROC _glEnable;
    PFNGLENABLEVERTEXATTRIBARRAYPROC _glEnableVertexAttribArray;
    PFNGLFINISHPROC _glFinish;
    PFNGLFLUSHPROC _glFlush;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC _glFramebufferRenderbuffer;
    PFNGLFRAMEBUFFERTEXTURE2DPROC _glFramebufferTexture2D;
    PFNGLFRONTFACEPROC _glFrontFace;
    PFNGLGENBUFFERSPROC _glGenBuffers;
    PFNGLGENERATEMIPMAPPROC _glGenerateMipmap;
    PFNGLGENFRAMEBUFFERSPROC _glGenFramebuffers;
    PFNGLGENRENDERBUFFERSPROC _glGenRenderbuffers;
    PFNGLGENTEXTURESPROC _glGenTextures;
    PFNGLGETACTIVEATTRIBPROC _glGetActiveAttrib;
    PFNGLGETACTIVEUNIFORMPROC _glGetActiveUniform;
    PFNGLGETATTACHEDSHADERSPROC _glGetAttachedShaders;
    PFNGLGETATTRIBLOCATIONPROC _glGetAttribLocation;
    PFNGLGETBOOLEANVPROC _glGetBooleanv;
    PFNGLGETBUFFERPARAMETERIVPROC _glGetBufferParameteriv;
    PFNGLGETERRORPROC _glGetError;
    PFNGLGETFLOATVPROC _glGetFloatv;
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC _glGetFramebufferAttachmentParameteriv;
    PFNGLGETINTEGERVPROC _glGetIntegerv;
    PFNGLGETPROGRAMIVPROC _glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC _glGetProgramInfoLog;
    PFNGLGETRENDERBUFFERPARAMETERIVPROC _glGetRenderbufferParameteriv;
    PFNGLGETSHADERIVPROC _glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC _glGetShaderInfoLog;
    PFNGLGETSHADERPRECISIONFORMATPROC _glGetShaderPrecisionFormat;
    PFNGLGETSHADERSOURCEPROC _glGetShaderSource;
    PFNGLGETSTRINGPROC _glGetString;
    PFNGLGETTEXPARAMETERFVPROC _glGetTexParameterfv;
    PFNGLGETTEXPARAMETERIVPROC _glGetTexParameteriv;
    PFNGLGETUNIFORMFVPROC _glGetUniformfv;
    PFNGLGETUNIFORMIVPROC _glGetUniformiv;
    PFNGLGETUNIFORMLOCATIONPROC _glGetUniformLocation;
    PFNGLGETVERTEXATTRIBFVPROC _glGetVertexAttribfv;
    PFNGLGETVERTEXATTRIBIVPROC _glGetVertexAttribiv;
    PFNGLGETVERTEXATTRIBPOINTERVPROC _glGetVertexAttribPointerv;
    PFNGLHINTPROC _glHint;
    PFNGLISBUFFERPROC _glIsBuffer;
    PFNGLISENABLEDPROC _glIsEnabled;
    PFNGLISFRAMEBUFFERPROC _glIsFramebuffer;
    PFNGLISPROGRAMPROC _glIsProgram;
    PFNGLISRENDERBUFFERPROC _glIsRenderbuffer;
    PFNGLISSHADERPROC _glIsShader;
    PFNGLISTEXTUREPROC _glIsTexture;
    PFNGLLINEWIDTHPROC _glLineWidth;
    PFNGLLINKPROGRAMPROC _glLinkProgram;
    PFNGLPIXELSTOREIPROC _glPixelStorei;
    PFNGLPOLYGONOFFSETPROC _glPolygonOffset;
    PFNGLREADPIXELSPROC _glReadPixels;
    PFNGLRELEASESHADERCOMPILERPROC _glReleaseShaderCompiler;
    PFNGLRENDERBUFFERSTORAGEPROC _glRenderbufferStorage;
    PFNGLSAMPLECOVERAGEPROC _glSampleCoverage;
    PFNGLSCISSORPROC _glScissor;
    PFNGLSHADERBINARYPROC _glShaderBinary;
    PFNGLSHADERSOURCEPROC _glShaderSource;
    PFNGLSTENCILFUNCPROC _glStencilFunc;
    PFNGLSTENCILFUNCSEPARATEPROC _glStencilFuncSeparate;
    PFNGLSTENCILMASKPROC _glStencilMask;
    PFNGLSTENCILMASKSEPARATEPROC _glStencilMaskSeparate;
    PFNGLSTENCILOPPROC _glStencilOp;
    PFNGLSTENCILOPSEPARATEPROC _glStencilOpSeparate;
    PFNGLTEXIMAGE2DPROC _glTexImage2D;
    PFNGLTEXPARAMETERFPROC _glTexParameterf;
    PFNGLTEXPARAMETERFVPROC _glTexParameterfv;
    PFNGLTEXPARAMETERIPROC _glTexParameteri;
    PFNGLTEXPARAMETERIVPROC _glTexParameteriv;
    PFNGLTEXSUBIMAGE2DPROC _glTexSubImage2D;
    PFNGLUNIFORM1FPROC _glUniform1f;
    PFNGLUNIFORM1FVPROC _glUniform1fv;
    PFNGLUNIFORM1IPROC _glUniform1i;
    PFNGLUNIFORM1IVPROC _glUniform1iv;
    PFNGLUNIFORM2FPROC _glUniform2f;
    PFNGLUNIFORM2FVPROC _glUniform2fv;
    PFNGLUNIFORM2IPROC _glUniform2i;
    PFNGLUNIFORM2IVPROC _glUniform2iv;
    PFNGLUNIFORM3FPROC _glUniform3f;
    PFNGLUNIFORM3FVPROC _glUniform3fv;
    PFNGLUNIFORM3IPROC _glUniform3i;
    PFNGLUNIFORM3IVPROC _glUniform3iv;
    PFNGLUNIFORM4FPROC _glUniform4f;
    PFNGLUNIFORM4FVPROC _glUniform4fv;
    PFNGLUNIFORM4IPROC _glUniform4i;
    PFNGLUNIFORM4IVPROC _glUniform4iv;
    PFNGLUNIFORMMATRIX2FVPROC _glUniformMatrix2fv;
    PFNGLUNIFORMMATRIX3FVPROC _glUniformMatrix3fv;
    PFNGLUNIFORMMATRIX4FVPROC _glUniformMatrix4fv;
    PFNGLUSEPROGRAMPROC _glUseProgram;
    PFNGLVALIDATEPROGRAMPROC _glValidateProgram;
    PFNGLVERTEXATTRIB1FPROC _glVertexAttrib1f;
    PFNGLVERTEXATTRIB1FVPROC _glVertexAttrib1fv;
    PFNGLVERTEXATTRIB2FPROC _glVertexAttrib2f;
    PFNGLVERTEXATTRIB2FVPROC _glVertexAttrib2fv;
    PFNGLVERTEXATTRIB3FPROC _glVertexAttrib3f;
    PFNGLVERTEXATTRIB3FVPROC _glVertexAttrib3fv;
    PFNGLVERTEXATTRIB4FPROC _glVertexAttrib4f;
    PFNGLVERTEXATTRIB4FVPROC _glVertexAttrib4fv;
    PFNGLVERTEXATTRIBPOINTERPROC _glVertexAttribPointer;
    PFNGLVIEWPORTPROC _glViewport;
    PFNGLREADBUFFERPROC _glReadBuffer;
    PFNGLDRAWRANGEELEMENTSPROC _glDrawRangeElements;
    PFNGLTEXIMAGE3DPROC _glTexImage3D;
    PFNGLTEXSUBIMAGE3DPROC _glTexSubImage3D;
    PFNGLCOPYTEXSUBIMAGE3DPROC _glCopyTexSubImage3D;
    PFNGLCOMPRESSEDTEXIMAGE3DPROC _glCompressedTexImage3D;
    PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC _glCompressedTexSubImage3D;
    PFNGLGENQUERIESPROC _glGenQueries;
    PFNGLDELETEQUERIESPROC _glDeleteQueries;
    PFNGLISQUERYPROC _glIsQuery;
    PFNGLBEGINQUERYPROC _glBeginQuery;
    PFNGLENDQUERYPROC _glEndQuery;
    PFNGLGETQUERYIVPROC _glGetQueryiv;
    PFNGLGETQUERYOBJECTUIVPROC _glGetQueryObjectuiv;
    PFNGLUNMAPBUFFERPROC _glUnmapBuffer;
    PFNGLGETBUFFERPOINTERVPROC _glGetBufferPointerv;
    PFNGLDRAWBUFFERSPROC _glDrawBuffers;
    PFNGLUNIFORMMATRIX2X3FVPROC _glUniformMatrix2x3fv;
    PFNGLUNIFORMMATRIX3X2FVPROC _glUniformMatrix3x2fv;
    PFNGLUNIFORMMATRIX2X4FVPROC _glUniformMatrix2x4fv;
    PFNGLUNIFORMMATRIX4X2FVPROC _glUniformMatrix4x2fv;
    PFNGLUNIFORMMATRIX3X4FVPROC _glUniformMatrix3x4fv;
    PFNGLUNIFORMMATRIX4X3FVPROC _glUniformMatrix4x3fv;
    PFNGLBLITFRAMEBUFFERPROC _glBlitFramebuffer;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC _glRenderbufferStorageMultisample;
    PFNGLFRAMEBUFFERTEXTURELAYERPROC _glFramebufferTextureLayer;
    PFNGLMAPBUFFERRANGEPROC _glMapBufferRange;
    PFNGLFLUSHMAPPEDBUFFERRANGEPROC _glFlushMappedBufferRange;
    PFNGLBINDVERTEXARRAYPROC _glBindVertexArray;
    PFNGLDELETEVERTEXARRAYSPROC _glDeleteVertexArrays;
    PFNGLGENVERTEXARRAYSPROC _glGenVertexArrays;
    PFNGLISVERTEXARRAYPROC _glIsVertexArray;
    PFNGLGETINTEGERI_VPROC _glGetIntegeri_v;
    PFNGLBEGINTRANSFORMFEEDBACKPROC _glBeginTransformFeedback;
    PFNGLENDTRANSFORMFEEDBACKPROC _glEndTransformFeedback;
    PFNGLBINDBUFFERRANGEPROC _glBindBufferRange;
    PFNGLBINDBUFFERBASEPROC _glBindBufferBase;
    PFNGLTRANSFORMFEEDBACKVARYINGSPROC _glTransformFeedbackVaryings;
    PFNGLGETTRANSFORMFEEDBACKVARYINGPROC _glGetTransformFeedbackVarying;
    PFNGLVERTEXATTRIBIPOINTERPROC _glVertexAttribIPointer;
    PFNGLGETVERTEXATTRIBIIVPROC _glGetVertexAttribIiv;
    PFNGLGETVERTEXATTRIBIUIVPROC _glGetVertexAttribIuiv;
    PFNGLVERTEXATTRIBI4IPROC _glVertexAttribI4i;
    PFNGLVERTEXATTRIBI4UIPROC _glVertexAttribI4ui;
    PFNGLVERTEXATTRIBI4IVPROC _glVertexAttribI4iv;
    PFNGLVERTEXATTRIBI4UIVPROC _glVertexAttribI4uiv;
    PFNGLGETUNIFORMUIVPROC _glGetUniformuiv;
    PFNGLGETFRAGDATALOCATIONPROC _glGetFragDataLocation;
    PFNGLUNIFORM1UIPROC _glUniform1ui;
    PFNGLUNIFORM2UIPROC _glUniform2ui;
    PFNGLUNIFORM3UIPROC _glUniform3ui;
    PFNGLUNIFORM4UIPROC _glUniform4ui;
    PFNGLUNIFORM1UIVPROC _glUniform1uiv;
    PFNGLUNIFORM2UIVPROC _glUniform2uiv;
    PFNGLUNIFORM3UIVPROC _glUniform3uiv;
    PFNGLUNIFORM4UIVPROC _glUniform4uiv;
    PFNGLCLEARBUFFERIVPROC _glClearBufferiv;
    PFNGLCLEARBUFFERUIVPROC _glClearBufferuiv;
    PFNGLCLEARBUFFERFVPROC _glClearBufferfv;
    PFNGLCLEARBUFFERFIPROC _glClearBufferfi;
    PFNGLGETSTRINGIPROC _glGetStringi;
    PFNGLCOPYBUFFERSUBDATAPROC _glCopyBufferSubData;
    PFNGLGETUNIFORMINDICESPROC _glGetUniformIndices;
    PFNGLGETACTIVEUNIFORMSIVPROC _glGetActiveUniformsiv;
    PFNGLGETUNIFORMBLOCKINDEXPROC _glGetUniformBlockIndex;
    PFNGLGETACTIVEUNIFORMBLOCKIVPROC _glGetActiveUniformBlockiv;
    PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC _glGetActiveUniformBlockName;
    PFNGLUNIFORMBLOCKBINDINGPROC _glUniformBlockBinding;
    PFNGLDRAWARRAYSINSTANCEDPROC _glDrawArraysInstanced;
    PFNGLDRAWELEMENTSINSTANCEDPROC _glDrawElementsInstanced;
    PFNGLFENCESYNCPROC _glFenceSync;
    PFNGLISSYNCPROC _glIsSync;
    PFNGLDELETESYNCPROC _glDeleteSync;
    PFNGLCLIENTWAITSYNCPROC _glClientWaitSync;
    PFNGLWAITSYNCPROC _glWaitSync;
    PFNGLGETINTEGER64VPROC _glGetInteger64v;
    PFNGLGETSYNCIVPROC _glGetSynciv;
    PFNGLGETINTEGER64I_VPROC _glGetInteger64i_v;
    PFNGLGETBUFFERPARAMETERI64VPROC _glGetBufferParameteri64v;
    PFNGLGENSAMPLERSPROC _glGenSamplers;
    PFNGLDELETESAMPLERSPROC _glDeleteSamplers;
    PFNGLISSAMPLERPROC _glIsSampler;
    PFNGLBINDSAMPLERPROC _glBindSampler;
    PFNGLSAMPLERPARAMETERIPROC _glSamplerParameteri;
    PFNGLSAMPLERPARAMETERIVPROC _glSamplerParameteriv;
    PFNGLSAMPLERPARAMETERFPROC _glSamplerParameterf;
    PFNGLSAMPLERPARAMETERFVPROC _glSamplerParameterfv;
    PFNGLGETSAMPLERPARAMETERIVPROC _glGetSamplerParameteriv;
    PFNGLGETSAMPLERPARAMETERFVPROC _glGetSamplerParameterfv;
    PFNGLVERTEXATTRIBDIVISORPROC _glVertexAttribDivisor;
    PFNGLBINDTRANSFORMFEEDBACKPROC _glBindTransformFeedback;
    PFNGLDELETETRANSFORMFEEDBACKSPROC _glDeleteTransformFeedbacks;
    PFNGLGENTRANSFORMFEEDBACKSPROC _glGenTransformFeedbacks;
    PFNGLISTRANSFORMFEEDBACKPROC _glIsTransformFeedback;
    PFNGLPAUSETRANSFORMFEEDBACKPROC _glPauseTransformFeedback;
    PFNGLRESUMETRANSFORMFEEDBACKPROC _glResumeTransformFeedback;
    PFNGLGETPROGRAMBINARYPROC _glGetProgramBinary;
    PFNGLPROGRAMBINARYPROC _glProgramBinary;
    PFNGLPROGRAMPARAMETERIPROC _glProgramParameteri;
    PFNGLINVALIDATEFRAMEBUFFERPROC _glInvalidateFramebuffer;
    PFNGLINVALIDATESUBFRAMEBUFFERPROC _glInvalidateSubFramebuffer;
    PFNGLTEXSTORAGE2DPROC _glTexStorage2D;
    PFNGLTEXSTORAGE3DPROC _glTexStorage3D;
    PFNGLGETINTERNALFORMATIVPROC _glGetInternalformativ;
};

extern struct glxw_es3 glxw_es3;

#define glActiveTexture (glxw_es3._glActiveTexture)
#define glAttachShader (glxw_es3._glAttachShader)
#define glBindAttribLocation (glxw_es3._glBindAttribLocation)
#define glBindBuffer (glxw_es3._glBindBuffer)
#define glBindFramebuffer (glxw_es3._glBindFramebuffer)
#define glBindRenderbuffer (glxw_es3._glBindRenderbuffer)
#define glBindTexture (glxw_es3._glBindTexture)
#define glBlendColor (glxw_es3._glBlendColor)
#define glBlendEquation (glxw_es3._glBlendEquation)
#define glBlendEquationSeparate (glxw_es3._glBlendEquationSeparate)
#define glBlendFunc (glxw_es3._glBlendFunc)
#define glBlendFuncSeparate (glxw_es3._glBlendFuncSeparate)
#define glBufferData (glxw_es3._glBufferData)
#define glBufferSubData (glxw_es3._glBufferSubData)
#define glCheckFramebufferStatus (glxw_es3._glCheckFramebufferStatus)
#define glClear (glxw_es3._glClear)
#define glClearColor (glxw_es3._glClearColor)
#define glClearDepthf (glxw_es3._glClearDepthf)
#define glClearStencil (glxw_es3._glClearStencil)
#define glColorMask (glxw_es3._glColorMask)
#define glCompileShader (glxw_es3._glCompileShader)
#define glCompressedTexImage2D (glxw_es3._glCompressedTexImage2D)
#define glCompressedTexSubImage2D (glxw_es3._glCompressedTexSubImage2D)
#define glCopyTexImage2D (glxw_es3._glCopyTexImage2D)
#define glCopyTexSubImage2D (glxw_es3._glCopyTexSubImage2D)
#define glCreateProgram (glxw_es3._glCreateProgram)
#define glCreateShader (glxw_es3._glCreateShader)
#define glCullFace (glxw_es3._glCullFace)
#define glDeleteBuffers (glxw_es3._glDeleteBuffers)
#define glDeleteFramebuffers (glxw_es3._glDeleteFramebuffers)
#define glDeleteProgram (glxw_es3._glDeleteProgram)
#define glDeleteRenderbuffers (glxw_es3._glDeleteRenderbuffers)
#define glDeleteShader (glxw_es3._glDeleteShader)
#define glDeleteTextures (glxw_es3._glDeleteTextures)
#define glDepthFunc (glxw_es3._glDepthFunc)
#define glDepthMask (glxw_es3._glDepthMask)
#define glDepthRangef (glxw_es3._glDepthRangef)
#define glDetachShader (glxw_es3._glDetachShader)
#define glDisable (glxw_es3._glDisable)
#define glDisableVertexAttribArray (glxw_es3._glDisableVertexAttribArray)
#define glDrawArrays (glxw_es3._glDrawArrays)
#define glDrawElements (glxw_es3._glDrawElements)
#define glEnable (glxw_es3._glEnable)
#define glEnableVertexAttribArray (glxw_es3._glEnableVertexAttribArray)
#define glFinish (glxw_es3._glFinish)
#define glFlush (glxw_es3._glFlush)
#define glFramebufferRenderbuffer (glxw_es3._glFramebufferRenderbuffer)
#define glFramebufferTexture2D (glxw_es3._glFramebufferTexture2D)
#define glFrontFace (glxw_es3._glFrontFace)
#define glGenBuffers (glxw_es3._glGenBuffers)
#define glGenerateMipmap (glxw_es3._glGenerateMipmap)
#define glGenFramebuffers (glxw_es3._glGenFramebuffers)
#define glGenRenderbuffers (glxw_es3._glGenRenderbuffers)
#define glGenTextures (glxw_es3._glGenTextures)
#define glGetActiveAttrib (glxw_es3._glGetActiveAttrib)
#define glGetActiveUniform (glxw_es3._glGetActiveUniform)
#define glGetAttachedShaders (glxw_es3._glGetAttachedShaders)
#define glGetAttribLocation (glxw_es3._glGetAttribLocation)
#define glGetBooleanv (glxw_es3._glGetBooleanv)
#define glGetBufferParameteriv (glxw_es3._glGetBufferParameteriv)
#define glGetError (glxw_es3._glGetError)
#define glGetFloatv (glxw_es3._glGetFloatv)
#define glGetFramebufferAttachmentParameteriv (glxw_es3._glGetFramebufferAttachmentParameteriv)
#define glGetIntegerv (glxw_es3._glGetIntegerv)
#define glGetProgramiv (glxw_es3._glGetProgramiv)
#define glGetProgramInfoLog (glxw_es3._glGetProgramInfoLog)
#define glGetRenderbufferParameteriv (glxw_es3._glGetRenderbufferParameteriv)
#define glGetShaderiv (glxw_es3._glGetShaderiv)
#define glGetShaderInfoLog (glxw_es3._glGetShaderInfoLog)
#define glGetShaderPrecisionFormat (glxw_es3._glGetShaderPrecisionFormat)
#define glGetShaderSource (glxw_es3._glGetShaderSource)
#define glGetString (glxw_es3._glGetString)
#define glGetTexParameterfv (glxw_es3._glGetTexParameterfv)
#define glGetTexParameteriv (glxw_es3._glGetTexParameteriv)
#define glGetUniformfv (glxw_es3._glGetUniformfv)
#define glGetUniformiv (glxw_es3._glGetUniformiv)
#define glGetUniformLocation (glxw_es3._glGetUniformLocation)
#define glGetVertexAttribfv (glxw_es3._glGetVertexAttribfv)
#define glGetVertexAttribiv (glxw_es3._glGetVertexAttribiv)
#define glGetVertexAttribPointerv (glxw_es3._glGetVertexAttribPointerv)
#define glHint (glxw_es3._glHint)
#define glIsBuffer (glxw_es3._glIsBuffer)
#define glIsEnabled (glxw_es3._glIsEnabled)
#define glIsFramebuffer (glxw_es3._glIsFramebuffer)
#define glIsProgram (glxw_es3._glIsProgram)
#define glIsRenderbuffer (glxw_es3._glIsRenderbuffer)
#define glIsShader (glxw_es3._glIsShader)
#define glIsTexture (glxw_es3._glIsTexture)
#define glLineWidth (glxw_es3._glLineWidth)
#define glLinkProgram (glxw_es3._glLinkProgram)
#define glPixelStorei (glxw_es3._glPixelStorei)
#define glPolygonOffset (glxw_es3._glPolygonOffset)
#define glReadPixels (glxw_es3._glReadPixels)
#define glReleaseShaderCompiler (glxw_es3._glReleaseShaderCompiler)
#define glRenderbufferStorage (glxw_es3._glRenderbufferStorage)
#define glSampleCoverage (glxw_es3._glSampleCoverage)
#define glScissor (glxw_es3._glScissor)
#define glShaderBinary (glxw_es3._glShaderBinary)
#define glShaderSource (glxw_es3._glShaderSource)
#define glStencilFunc (glxw_es3._glStencilFunc)
#define glStencilFuncSeparate (glxw_es3._glStencilFuncSeparate)
#define glStencilMask (glxw_es3._glStencilMask)
#define glStencilMaskSeparate (glxw_es3._glStencilMaskSeparate)
#define glStencilOp (glxw_es3._glStencilOp)
#define glStencilOpSeparate (glxw_es3._glStencilOpSeparate)
#define glTexImage2D (glxw_es3._glTexImage2D)
#define glTexParameterf (glxw_es3._glTexParameterf)
#define glTexParameterfv (glxw_es3._glTexParameterfv)
#define glTexParameteri (glxw_es3._glTexParameteri)
#define glTexParameteriv (glxw_es3._glTexParameteriv)
#define glTexSubImage2D (glxw_es3._glTexSubImage2D)
#define glUniform1f (glxw_es3._glUniform1f)
#define glUniform1fv (glxw_es3._glUniform1fv)
#define glUniform1i (glxw_es3._glUniform1i)
#define glUniform1iv (glxw_es3._glUniform1iv)
#define glUniform2f (glxw_es3._glUniform2f)
#define glUniform2fv (glxw_es3._glUniform2fv)
#define glUniform2i (glxw_es3._glUniform2i)
#define glUniform2iv (glxw_es3._glUniform2iv)
#define glUniform3f (glxw_es3._glUniform3f)
#define glUniform3fv (glxw_es3._glUniform3fv)
#define glUniform3i (glxw_es3._glUniform3i)
#define glUniform3iv (glxw_es3._glUniform3iv)
#define glUniform4f (glxw_es3._glUniform4f)
#define glUniform4fv (glxw_es3._glUniform4fv)
#define glUniform4i (glxw_es3._glUniform4i)
#define glUniform4iv (glxw_es3._glUniform4iv)
#define glUniformMatrix2fv (glxw_es3._glUniformMatrix2fv)
#define glUniformMatrix3fv (glxw_es3._glUniformMatrix3fv)
#define glUniformMatrix4fv (glxw_es3._glUniformMatrix4fv)
#define glUseProgram (glxw_es3._glUseProgram)
#define glValidateProgram (glxw_es3._glValidateProgram)
#define glVertexAttrib1f (glxw_es3._glVertexAttrib1f)
#define glVertexAttrib1fv (glxw_es3._glVertexAttrib1fv)
#define glVertexAttrib2f (glxw_es3._glVertexAttrib2f)
#define glVertexAttrib2fv (glxw_es3._glVertexAttrib2fv)
#define glVertexAttrib3f (glxw_es3._glVertexAttrib3f)
#define glVertexAttrib3fv (glxw_es3._glVertexAttrib3fv)
#define glVertexAttrib4f (glxw_es3._glVertexAttrib4f)
#define glVertexAttrib4fv (glxw_es3._glVertexAttrib4fv)
#define glVertexAttribPointer (glxw_es3._glVertexAttribPointer)
#define glViewport (glxw_es3._glViewport)
#define glReadBuffer (glxw_es3._glReadBuffer)
#define glDrawRangeElements (glxw_es3._glDrawRangeElements)
#define glTexImage3D (glxw_es3._glTexImage3D)
#define glTexSubImage3D (glxw_es3._glTexSubImage3D)
#define glCopyTexSubImage3D (glxw_es3._glCopyTexSubImage3D)
#define glCompressedTexImage3D (glxw_es3._glCompressedTexImage3D)
#define glCompressedTexSubImage3D (glxw_es3._glCompressedTexSubImage3D)
#define glGenQueries (glxw_es3._glGenQueries)
#define glDeleteQueries (glxw_es3._glDeleteQueries)
#define glIsQuery (glxw_es3._glIsQuery)
#define glBeginQuery (glxw_es3._glBeginQuery)
#define glEndQuery (glxw_es3._glEndQuery)
#define glGetQueryiv (glxw_es3._glGetQueryiv)
#define glGetQueryObjectuiv (glxw_es3._glGetQueryObjectuiv)
#define glUnmapBuffer (glxw_es3._glUnmapBuffer)
#define glGetBufferPointerv (glxw_es3._glGetBufferPointerv)
#define glDrawBuffers (glxw_es3._glDrawBuffers)
#define glUniformMatrix2x3fv (glxw_es3._glUniformMatrix2x3fv)
#define glUniformMatrix3x2fv (glxw_es3._glUniformMatrix3x2fv)
#define glUniformMatrix2x4fv (glxw_es3._glUniformMatrix2x4fv)
#define glUniformMatrix4x2fv (glxw_es3._glUniformMatrix4x2fv)
#define glUniformMatrix3x4fv (glxw_es3._glUniformMatrix3x4fv)
#define glUniformMatrix4x3fv (glxw_es3._glUniformMatrix4x3fv)
#define glBlitFramebuffer (glxw_es3._glBlitFramebuffer)
#define glRenderbufferStorageMultisample (glxw_es3._glRenderbufferStorageMultisample)
#define glFramebufferTextureLayer (glxw_es3._glFramebufferTextureLayer)
#define glMapBufferRange (glxw_es3._glMapBufferRange)
#define glFlushMappedBufferRange (glxw_es3._glFlushMappedBufferRange)
#define glBindVertexArray (glxw_es3._glBindVertexArray)
#define glDeleteVertexArrays (glxw_es3._glDeleteVertexArrays)
#define glGenVertexArrays (glxw_es3._glGenVertexArrays)
#define glIsVertexArray (glxw_es3._glIsVertexArray)
#define glGetIntegeri_v (glxw_es3._glGetIntegeri_v)
#define glBeginTransformFeedback (glxw_es3._glBeginTransformFeedback)
#define glEndTransformFeedback (glxw_es3._glEndTransformFeedback)
#define glBindBufferRange (glxw_es3._glBindBufferRange)
#define glBindBufferBase (glxw_es3._glBindBufferBase)
#define glTransformFeedbackVaryings (glxw_es3._glTransformFeedbackVaryings)
#define glGetTransformFeedbackVarying (glxw_es3._glGetTransformFeedbackVarying)
#define glVertexAttribIPointer (glxw_es3._glVertexAttribIPointer)
#define glGetVertexAttribIiv (glxw_es3._glGetVertexAttribIiv)
#define glGetVertexAttribIuiv (glxw_es3._glGetVertexAttribIuiv)
#define glVertexAttribI4i (glxw_es3._glVertexAttribI4i)
#define glVertexAttribI4ui (glxw_es3._glVertexAttribI4ui)
#define glVertexAttribI4iv (glxw_es3._glVertexAttribI4iv)
#define glVertexAttribI4uiv (glxw_es3._glVertexAttribI4uiv)
#define glGetUniformuiv (glxw_es3._glGetUniformuiv)
#define glGetFragDataLocation (glxw_es3._glGetFragDataLocation)
#define glUniform1ui (glxw_es3._glUniform1ui)
#define glUniform2ui (glxw_es3._glUniform2ui)
#define glUniform3ui (glxw_es3._glUniform3ui)
#define glUniform4ui (glxw_es3._glUniform4ui)
#define glUniform1uiv (glxw_es3._glUniform1uiv)
#define glUniform2uiv (glxw_es3._glUniform2uiv)
#define glUniform3uiv (glxw_es3._glUniform3uiv)
#define glUniform4uiv (glxw_es3._glUniform4uiv)
#define glClearBufferiv (glxw_es3._glClearBufferiv)
#define glClearBufferuiv (glxw_es3._glClearBufferuiv)
#define glClearBufferfv (glxw_es3._glClearBufferfv)
#define glClearBufferfi (glxw_es3._glClearBufferfi)
#define glGetStringi (glxw_es3._glGetStringi)
#define glCopyBufferSubData (glxw_es3._glCopyBufferSubData)
#define glGetUniformIndices (glxw_es3._glGetUniformIndices)
#define glGetActiveUniformsiv (glxw_es3._glGetActiveUniformsiv)
#define glGetUniformBlockIndex (glxw_es3._glGetUniformBlockIndex)
#define glGetActiveUniformBlockiv (glxw_es3._glGetActiveUniformBlockiv)
#define glGetActiveUniformBlockName (glxw_es3._glGetActiveUniformBlockName)
#define glUniformBlockBinding (glxw_es3._glUniformBlockBinding)
#define glDrawArraysInstanced (glxw_es3._glDrawArraysInstanced)
#define glDrawElementsInstanced (glxw_es3._glDrawElementsInstanced)
#define glFenceSync (glxw_es3._glFenceSync)
#define glIsSync (glxw_es3._glIsSync)
#define glDeleteSync (glxw_es3._glDeleteSync)
#define glClientWaitSync (glxw_es3._glClientWaitSync)
#define glWaitSync (glxw_es3._glWaitSync)
#define glGetInteger64v (glxw_es3._glGetInteger64v)
#define glGetSynciv (glxw_es3._glGetSynciv)
#define glGetInteger64i_v (glxw_es3._glGetInteger64i_v)
#define glGetBufferParameteri64v (glxw_es3._glGetBufferParameteri64v)
#define glGenSamplers (glxw_es3._glGenSamplers)
#define glDeleteSamplers (glxw_es3._glDeleteSamplers)
#define glIsSampler (glxw_es3._glIsSampler)
#define glBindSampler (glxw_es3._glBindSampler)
#define glSamplerParameteri (glxw_es3._glSamplerParameteri)
#define glSamplerParameteriv (glxw_es3._glSamplerParameteriv)
#define glSamplerParameterf (glxw_es3._glSamplerParameterf)
#define glSamplerParameterfv (glxw_es3._glSamplerParameterfv)
#define glGetSamplerParameteriv (glxw_es3._glGetSamplerParameteriv)
#define glGetSamplerParameterfv (glxw_es3._glGetSamplerParameterfv)
#define glVertexAttribDivisor (glxw_es3._glVertexAttribDivisor)
#define glBindTransformFeedback (glxw_es3._glBindTransformFeedback)
#define glDeleteTransformFeedbacks (glxw_es3._glDeleteTransformFeedbacks)
#define glGenTransformFeedbacks (glxw_es3._glGenTransformFeedbacks)
#define glIsTransformFeedback (glxw_es3._glIsTransformFeedback)
#define glPauseTransformFeedback (glxw_es3._glPauseTransformFeedback)
#define glResumeTransformFeedback (glxw_es3._glResumeTransformFeedback)
#define glGetProgramBinary (glxw_es3._glGetProgramBinary)
#define glProgramBinary (glxw_es3._glProgramBinary)
#define glProgramParameteri (glxw_es3._glProgramParameteri)
#define glInvalidateFramebuffer (glxw_es3._glInvalidateFramebuffer)
#define glInvalidateSubFramebuffer (glxw_es3._glInvalidateSubFramebuffer)
#define glTexStorage2D (glxw_es3._glTexStorage2D)
#define glTexStorage3D (glxw_es3._glTexStorage3D)
#define glGetInternalformativ (glxw_es3._glGetInternalformativ)

#ifdef __cplusplus
}
#endif

#endif
