/*
    CR_GRAPHICS_EXTENSIONS.h - Single-header multi-platform OpenGL function loader
    ----------------------------------------------------------------------------
    USAGE
    ----------------------------------------------------------------------------
    1) Add the following lines in exactly one of your cpp files to compile the
       implementation:
           #define CR_GRAPHICS_EXTENSIONS_IMPLEMENTATION
           #include "CR_GRAPHICS_EXTENSIONS.h"
    2) In all other files in which you want to use OpenGL functions, simply 
       include this header file as follows:
           #include "CR_GRAPHICS_EXTENSIONS.h"
    3) Call CR_GRAPHICS_EXTENSIONS_init() before using any OpenGL function and after you have a
       valid OpenGL context.
    ----------------------------------------------------------------------------
    LICENSE
    ----------------------------------------------------------------------------
    This software is in the public domain. Where that dedication is not
    recognized, you are granted a perpetual, irrevocable license to copy,
    distribute, and modify this file as you see fit.
*/
#ifndef CR_GRAPHICS_EXTENSIONS_H
#define CR_GRAPHICS_EXTENSIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CR_PLATFORM_INLINE
#ifdef _WIN32
#define CR_PLATFORM_INLINE __inline
#else
#define CR_PLATFORM_INLINE
#endif
#endif


#if defined(__linux__)
#include <dlfcn.h>
#define GLDECL // Empty define
#define CR_GL_FUNC_LIST_WIN32 // Empty define
#endif // __linux__

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define GLDECL WINAPI

#pragma comment (lib, "opengl32.lib")


/*
#define GL_ARRAY_BUFFER                   0x8892 // Acquired from:
#define GL_ARRAY_BUFFER_BINDING           0x8894 // https://www.opengl.org/registry/api/GL/glext.h
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COMPILE_STATUS                 0x8B81
#define GL_CURRENT_PROGRAM                0x8B8D
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ELEMENT_ARRAY_BUFFER_BINDING   0x8895
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_FRAMEBUFFER                    0x8D40
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_FUNC_ADD                       0x8006
#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_STATIC_DRAW                    0x88E4
#define GL_STREAM_DRAW                    0x88E0
#define GL_TEXTURE0                       0x84C0
#define GL_VERTEX_SHADER                  0x8B31
*/

typedef char GLchar;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define CR_GL_FUNC_LIST_WIN32 \
    /* ret, name, params */ \
	GLE(const GLubyte *,	wglGetExtensionsStringARB, void) \
	GLE(const GLubyte *,	wglGetExtensionsStringEXT, void) \
	GLE(BOOL,				wglChoosePixelFormatARB,	  HDC hdc, const int *piAttribIList, const FLOAT * pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats) \
	GLE(HGLRC,				wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const int *attribList) \
	GLE(void,				wglSwapIntervalEXT, int interval) \
	/* end */

#endif // _WIN32

#include "GL/gl.h"
#include "GL/glext.h"
#include "GL/glcorearb.h"
#include "GL/wglext.h"


#define CR_GL_FUNC_LIST \
    /* ret, name, params */ \
    GLE(void,					glActiveTexture,            GLenum texture) \
    GLE(void,					glAttachShader,				GLuint program, GLuint shader) \
	GLE(void,					glBindBuffer,				GLenum target, GLuint buffer) \
    GLE(void,					glBindFramebuffer,			GLenum target, GLuint framebuffer) \
	GLE(void,					glBindRenderbuffer,			GLenum target, GLuint renderbuffer) \
	GLE(void,					glBlendEquation,            GLenum mode) \
	GLE(void,					glBlitFramebuffer,			GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) \
    GLE(void,					glBufferData,				GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
	GLE(void,					glBufferDataARB,			GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) \
	GLE(void,					glBufferStorage,			GLenum target, GLsizeiptr size, const GLvoid * data, GLbitfield flags) \
	GLE(GLenum,					glClientWaitSync,			GLsync sync, GLbitfield flags, GLuint64 timeout) \
	GLE(void,					glCompressedTexImage2D,		GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data); \
	GLE(void,					glCompressedTexImage3D,		GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data); \
	GLE(void,					glCompressedTexSubImage3D,	GLenum target, GLint level, GLint xoffset, 	GLint yoffset, 	GLint zoffset, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const GLvoid * data) \
	GLE(void,					glDeleteQueries,			GLsizei n, const GLuint * ids) \
	GLE(void,					glDeleteSync,				GLsync sync) \
	GLE(void,					glDetachShader,				GLuint prog, GLuint shader); \
	GLE(void,					glDeleteProgram,			GLuint prog); \
	GLE(void,					glDeleteShader,				GLuint shader); \
	GLE(void,					glFramebufferRenderbuffer,	GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) \
    GLE(GLsync,					glFenceSync,				GLenum condition, GLbitfield flags) \
	GLE(void,					glGenQueries,				GLsizei n, GLuint * ids) \
	GLE(void,					glGetInteger64v,			GLenum pname, GLint64 * params) \
	GLE(void,					glBufferSubData,			GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) \
    GLE(GLenum,					glCheckFramebufferStatus,	GLenum target) \
    GLE(void,					glClearBufferfv,			GLenum buffer, GLint drawbuffer, const GLfloat * value) \
    GLE(void,					glCompileShader,			GLuint shader) \
    GLE(GLuint,					glCreateProgram,			void) \
    GLE(GLuint,					glCreateShader,				GLenum type) \
    GLE(void,					glDeleteBuffers,			GLsizei n, const GLuint *buffers) \
    GLE(void,					glDeleteFramebuffers,		GLsizei n, const GLuint *framebuffers) \
	GLE(void,					glDeleteRenderbuffers,		GLsizei n, const GLuint *renderbuffers) \
    GLE(void,					glEnableVertexAttribArray,	GLuint index) \
    GLE(void,					glDrawBuffers,				GLsizei n, const GLenum *bufs) \
    GLE(void,					glFramebufferTexture2D,		GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) \
    GLE(void,					glGenBuffers,				GLsizei n, GLuint *buffers) \
	GLE(void,					glGenRenderbuffers,			GLsizei n, GLuint * renderbuffers); \
    GLE(void,					glGenFramebuffers,			GLsizei n, GLuint * framebuffers) \
	GLE(void,					glGenerateMipmap,			GLenum target); \
	GLE(void,					glGenerateTextureMipmap,	GLuint texture); \
    GLE(GLint,					glGetAttribLocation,		GLuint program, const GLchar *name) \
	GLE(void,					glGetProgramInfoLog,		GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog); \
	GLE(void,					glGetProgramiv,				GLuint program, GLenum pname, GLint *params); \
    GLE(void,					glGetShaderInfoLog,			GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) \
    GLE(void,					glGetShaderiv,				GLuint shader, GLenum pname, GLint *params) \
    GLE(GLint,					glGetUniformLocation,		GLuint program, const GLchar *name) \
	GLE(void,					glQueryCounter,				GLuint id, GLenum target) \
	GLE(void,					glGetQueryObjectui64v,		GLuint id, GLenum pname, GLuint64 * params) \
	GLE(void,					glGetQueryObjecti64v,		GLuint id, GLenum pname, GLint64 * params) \
    GLE(void,					glLinkProgram,				GLuint program) \
	GLE(void*,					glMapBuffer,				GLenum target, GLenum access) \
	GLE(void*,					glMapBufferARB,				GLenum target, GLenum access) \
	GLE(void*,					glMapBufferRange,			GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) \
	GLE(GLboolean,				glUnmapBuffer,				GLenum target) \
	GLE(void,					glMemoryBarrier,			GLbitfield barriers) \
    GLE(void,					glShaderSource,				GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) \
    GLE(void,					glTexImage2DMultisample, 	GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) \
	GLE(void,					glRenderbufferStorage,		GLenum target, GLenum internalFormat, GLsizei width, GLsizei height) \
	GLE(void,					glRenderbufferStorageMultisample, GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height) \
	GLE(void,					glTexStorage2D,				GLenum target, GLint levels,  GLint internalFormat, GLsizei widht, GLsizei height) \
	GLE(void,					glUniform1i,				GLint location, GLint v0) \
    GLE(void,					glUniform1f,				GLint location, GLfloat v0) \
    GLE(void,					glUniform2f,				GLint location, GLfloat v0, GLfloat v1) \
    GLE(void,					glUniform4f,				GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
    GLE(void,					glUniformMatrix4fv,			GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
    GLE(void,					glUseProgram,				GLuint program) \
	GLE(void,					glWaitSync,					GLsync sync, GLbitfield flags, GLuint64 timeout) \
    GLE(void,					glVertexAttribPointer,		GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer) \
	GLE(const GLubyte *,		glGetStringi,				GLenum name, GLuint index) \
    /* end */

#define GLE(ret, name, ...) typedef ret GLDECL name##proc(__VA_ARGS__); extern name##proc * ##name;
CR_GL_FUNC_LIST
CR_GL_FUNC_LIST_WIN32
#undef GLE

bool crgc_gl_ext_init();
bool crgc_wgl_ext_init();


// =============================================================================

//#ifdef CR_GRAPHICS_EXTENSIONS_IMPLEMENTATION

#define GLE(ret, name, ...) name##proc * name;
CR_GL_FUNC_LIST
CR_GL_FUNC_LIST_WIN32
#undef GLE

bool CR_PLATFORM_INLINE crgc_wgl_ext_init()
{
#if defined(__linux__)

    void* libGL = dlopen("libGL.so", RTLD_LAZY);
    if (!libGL) {
        printf("ERROR: libGL.so couldn't be loaded\n");
        return false;
    }

    #define GLE(ret, name, ...)														\
	name = (name##proc *) dlsym(libGL, #name);										\
            if (!name) {															\
                printf("Function gl" #name " couldn't be loaded from libGL.so\n");	\
                //return false;														
            }
        CR_GL_FUNC_LIST
    #undef GLE

#elif defined(_WIN32)

    HINSTANCE dll;
	typedef PROC WINAPI wglGetProcAddressproc(LPCSTR lpszProc);
	wglGetProcAddressproc* wglGetProcAddress;

	printf("\n crgc_wgl_ext_init() \n");
	//static bool gl_ext_load = false;
	//static bool gl_ext_load_success = false; 
	
	//prevent function from being loaded more than once
	//if( gl_ext_load )
	//	return gl_ext_load_success;

	dll = LoadLibraryA("opengl32.dll");
	if (!dll) {
        printf("\nopengl32.dll not found.\n");
        return false;
    }
    /*wglGetProcAddressproc* */wglGetProcAddress =
        (wglGetProcAddressproc*)GetProcAddress(dll, "wglGetProcAddress");

	//use xmacros to iterate over each function definition and expand it
    #define GLE(ret, name, ...)																		\
            name = (name##proc *)wglGetProcAddress(#name);											\
            if (!name) {																			\
                printf("\nFunction " #name " couldn't be loaded from opengl32.dll\n");	\
            }
        CR_GL_FUNC_LIST_WIN32
    #undef GLE

#else
    #error "GL loading for this platform is not implemented yet."
#endif

	//gl_ext_load_success = true;
    return true;
}


bool CR_PLATFORM_INLINE crgc_gl_ext_init()
{
#if defined(__linux__)

    void* libGL = dlopen("libGL.so", RTLD_LAZY);
    if (!libGL) {
        printf("ERROR: libGL.so couldn't be loaded\n");
        return false;
    }

    #define GLE(ret, name, ...)														\
	name = (name##proc *) dlsym(libGL, #name);										\
            if (!name) {															\
                printf("Function gl" #name " couldn't be loaded from libGL.so\n");	\
                //return false;														
            }
        CR_GL_FUNC_LIST
    #undef GLE

#elif defined(_WIN32)

    HINSTANCE dll;
	typedef PROC WINAPI wglGetProcAddressproc(LPCSTR lpszProc);
	wglGetProcAddressproc* wglGetProcAddress;

	printf("\n crgc_gl_ext_init() \n");
	//static bool gl_ext_load = false;
	//static bool gl_ext_load_success = false; 
	
	//prevent function from being loaded more than once
	//if( gl_ext_load )
	//	return gl_ext_load_success;

	dll = LoadLibraryA("opengl32.dll");
	if (!dll) {
        printf("\nopengl32.dll not found.\n");
        return false;
    }
    /*wglGetProcAddressproc* */wglGetProcAddress =
        (wglGetProcAddressproc*)GetProcAddress(dll, "wglGetProcAddress");

	//use xmacros to iterate over each function definition and expand it
    #define GLE(ret, name, ...)																		\
            name = (name##proc *)wglGetProcAddress(#name);											\
            if (!name) {																			\
                printf("\nFunction " #name " couldn't be loaded from opengl32.dll\n");	\
            }
        CR_GL_FUNC_LIST
    #undef GLE

#else
    #error "GL loading for this platform is not implemented yet."
#endif

	//gl_ext_load_success = true;
    return true;
}


#ifdef __cplusplus
}
#endif
    

#endif //CR_GRAPHICS_EXTENSIONS_IMPLEMENTATION