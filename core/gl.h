#pragma once
// En Android se usa OpenGL ES 3 del sistema. En Windows se cargan las funciones
// necesarias en tiempo de ejecucion (sin depender de cabeceras externas).
#ifdef FORGE_GLES
#include <GLES3/gl3.h>
#else
#include <cstddef>

typedef unsigned int GLenum, GLuint, GLbitfield;
typedef int GLint, GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_DEPTH_BUFFER_BIT 0x100
#define GL_DEPTH_TEST 0xB71
#define GL_CULL_FACE 0xB44
#define GL_SCISSOR_TEST 0xC11
#define GL_BLEND 0xBE2
#define GL_SRC_ALPHA 0x302
#define GL_ONE_MINUS_SRC_ALPHA 0x303
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_TRIANGLES 4
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_BYTE 0x1401
#define GL_FLOAT 0x1406
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_FRAMEBUFFER 0x8D40
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_NONE 0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE_2D 0xDE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901
#define GL_RGBA 0x1908
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_LEQUAL 0x203
#define GL_FALSE 0
#define GL_TRUE 1

#ifdef _WIN32
#define FAPI __stdcall
#else
#define FAPI
#endif

#define GL_FUNCS(X) \
    X(void, glClear, (GLbitfield)) \
    X(void, glClearColor, (GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void, glEnable, (GLenum)) \
    X(void, glDisable, (GLenum)) \
    X(void, glViewport, (GLint, GLint, GLsizei, GLsizei)) \
    X(void, glScissor, (GLint, GLint, GLsizei, GLsizei)) \
    X(void, glBlendFunc, (GLenum, GLenum)) \
    X(void, glPolygonOffset, (GLfloat, GLfloat)) \
    X(void, glDrawElements, (GLenum, GLsizei, GLenum, const void*)) \
    X(void, glDrawArrays, (GLenum, GLint, GLsizei)) \
    X(void, glGenTextures, (GLsizei, GLuint*)) \
    X(void, glBindTexture, (GLenum, GLuint)) \
    X(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
    X(void, glTexParameteri, (GLenum, GLenum, GLint)) \
    X(void, glDeleteTextures, (GLsizei, const GLuint*)) \
    X(void, glActiveTexture, (GLenum)) \
    X(void, glGenerateMipmap, (GLenum)) \
    X(void, glGenBuffers, (GLsizei, GLuint*)) \
    X(void, glBindBuffer, (GLenum, GLuint)) \
    X(void, glBufferData, (GLenum, GLsizeiptr, const void*, GLenum)) \
    X(void, glGenVertexArrays, (GLsizei, GLuint*)) \
    X(void, glBindVertexArray, (GLuint)) \
    X(void, glEnableVertexAttribArray, (GLuint)) \
    X(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
    X(GLuint, glCreateShader, (GLenum)) \
    X(void, glShaderSource, (GLuint, GLsizei, const GLchar* const*, const GLint*)) \
    X(void, glCompileShader, (GLuint)) \
    X(void, glGetShaderiv, (GLuint, GLenum, GLint*)) \
    X(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*)) \
    X(void, glDeleteShader, (GLuint)) \
    X(GLuint, glCreateProgram, ()) \
    X(void, glAttachShader, (GLuint, GLuint)) \
    X(void, glLinkProgram, (GLuint)) \
    X(void, glGetProgramiv, (GLuint, GLenum, GLint*)) \
    X(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*)) \
    X(void, glUseProgram, (GLuint)) \
    X(GLint, glGetUniformLocation, (GLuint, const GLchar*)) \
    X(void, glUniform1i, (GLint, GLint)) \
    X(void, glUniform1f, (GLint, GLfloat)) \
    X(void, glUniform2f, (GLint, GLfloat, GLfloat)) \
    X(void, glUniform3f, (GLint, GLfloat, GLfloat, GLfloat)) \
    X(void, glUniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
    X(void, glUniform3fv, (GLint, GLsizei, const GLfloat*)) \
    X(void, glUniform4fv, (GLint, GLsizei, const GLfloat*)) \
    X(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat*)) \
    X(void, glGenFramebuffers, (GLsizei, GLuint*)) \
    X(void, glBindFramebuffer, (GLenum, GLuint)) \
    X(void, glFramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint)) \
    X(GLenum, glCheckFramebufferStatus, (GLenum)) \
    X(void, glDrawBuffers, (GLsizei, const GLenum*)) \
    X(void, glReadBuffer, (GLenum))

#define X(r, n, a) typedef r(FAPI* PFN_##n) a; extern PFN_##n n;
GL_FUNCS(X)
#undef X

// Carga todas las funciones con el cargador de la plataforma (p. ej. glfwGetProcAddress).
bool forgeLoadGL(void* (*getProc)(const char*));
#endif
