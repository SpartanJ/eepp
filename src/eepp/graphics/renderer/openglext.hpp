#ifndef EE_GLEXTENSIONS_HPP
#define EE_GLEXTENSIONS_HPP

#include <eepp/config.hpp>

#ifndef EE_GLES
//! GL2 and GL3 ( PC platform )
#ifdef EE_GLEW_AVAILABLE
#define GLEW_STATIC
#define GLEW_NO_GLU
#include <glew/glew.h>
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#endif
#endif

#include <eepp/graphics/renderer/opengl.hpp>

#ifndef APIENTRY
#define APIENTRY
#endif

namespace EE { namespace Graphics {

typedef const GLubyte*( APIENTRY* pglGetStringiFunc )( unsigned int, unsigned int );
typedef void( APIENTRY* pglGenFramebuffers )( GLsizei n, GLuint* framebuffers );
typedef void( APIENTRY* pglBindFramebuffer )( GLenum target, GLuint framebuffer );
typedef void( APIENTRY* pglFramebufferTexture2D )( GLenum target, GLenum attachment,
												   GLenum textarget, GLuint texture, GLint level );
typedef void( APIENTRY* pglDeleteFramebuffers )( GLsizei n, const GLuint* framebuffers );
typedef void( APIENTRY* pglGenRenderbuffers )( GLsizei n, GLuint* renderbuffers );
typedef void( APIENTRY* pglDeleteRenderbuffers )( GLsizei n, const GLuint* renderbuffers );
typedef void( APIENTRY* pglRenderbufferStorage )( GLenum target, GLenum internalformat,
												  GLsizei width, GLsizei height );
typedef void( APIENTRY* pglFramebufferRenderbuffer )( GLenum target, GLenum attachment,
													  GLenum renderbuffertarget,
													  GLuint renderbuffer );
typedef GLenum( APIENTRY* pglCheckFramebufferStatus )( GLenum target );
typedef void( APIENTRY* pglBindRenderbuffer )( GLenum target, GLuint renderbuffer );
typedef void( APIENTRY* pglBlendFuncSeparate )( GLenum sfactorRGB, GLenum dfactorRGB,
												GLenum sfactorAlpha, GLenum dfactorAlpha );
typedef void( APIENTRY* pglDiscardFramebufferEXT )( GLenum target, GLsizei numAttachments,
													const GLenum* attachments );
typedef void( APIENTRY* pglBlendEquationSeparate )( GLenum modeRGB, GLenum modeAlpha );
typedef void( APIENTRY* pglBlitFramebufferEXT )( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
												 GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
												 GLbitfield mask, GLenum filter );
typedef void( APIENTRY* pglActiveTexture )( GLenum texture );
typedef void( APIENTRY* pglClientActiveTexture )( GLenum texture );
typedef void( APIENTRY* pglGenBuffers )( GLsizei n, GLuint* buffers );
typedef void( APIENTRY* pglDeleteBuffers )( GLsizei n, const GLuint* buffers );
typedef void( APIENTRY* pglBindBuffer )( GLenum target, GLuint buffer );
typedef void( APIENTRY* pglBufferData )( GLenum target, IntPtr size, const void* data,
										 GLenum usage );
typedef void( APIENTRY* pglBufferSubData )( GLenum target, IntPtr offset, IntPtr size,
											const void* data );
typedef void( APIENTRY* pglVertexAttribPointer )( GLuint index, GLint size, GLenum type,
												  GLboolean normalized, GLsizei stride,
												  const void* pointer );
typedef void( APIENTRY* pglEnableVertexAttribArray )( GLuint index );
typedef void( APIENTRY* pglDisableVertexAttribArray )( GLuint index );
typedef GLuint( APIENTRY* pglCreateShader )( GLenum type );
typedef void( APIENTRY* pglDeleteShader )( GLuint shader );
typedef void( APIENTRY* pglShaderSource )( GLuint shader, GLsizei count,
										   const GLchar* const* strings, const GLint* lengths );
typedef void( APIENTRY* pglCompileShader )( GLuint shader );
typedef void( APIENTRY* pglGetShaderiv )( GLuint shader, GLenum pname, GLint* params );
typedef void( APIENTRY* pglGetShaderInfoLog )( GLuint shader, GLsizei maxLength, GLsizei* length,
											   GLchar* infoLog );
typedef GLuint( APIENTRY* pglCreateProgram )();
typedef void( APIENTRY* pglDeleteProgram )( GLuint program );
typedef void( APIENTRY* pglAttachShader )( GLuint program, GLuint shader );
typedef void( APIENTRY* pglLinkProgram )( GLuint program );
typedef void( APIENTRY* pglUseProgram )( GLuint program );
typedef void( APIENTRY* pglGetProgramiv )( GLuint program, GLenum pname, GLint* params );
typedef void( APIENTRY* pglGetProgramInfoLog )( GLuint program, GLsizei maxLength, GLsizei* length,
												GLchar* infoLog );
typedef GLint( APIENTRY* pglGetUniformLocation )( GLuint program, const GLchar* name );
typedef GLint( APIENTRY* pglGetAttribLocation )( GLuint program, const GLchar* name );
typedef void( APIENTRY* pglUniform1i )( GLint location, GLint value );
typedef void( APIENTRY* pglUniform1f )( GLint location, GLfloat value );
typedef void( APIENTRY* pglUniform2fv )( GLint location, GLsizei count, const GLfloat* value );
typedef void( APIENTRY* pglUniform3fv )( GLint location, GLsizei count, const GLfloat* value );
typedef void( APIENTRY* pglUniform4f )( GLint location, GLfloat x, GLfloat y, GLfloat z,
										GLfloat w );
typedef void( APIENTRY* pglUniform4fv )( GLint location, GLsizei count, const GLfloat* value );
typedef void( APIENTRY* pglUniformMatrix4fv )( GLint location, GLsizei count, GLboolean transpose,
											   const GLfloat* value );
typedef void( APIENTRY* pglCompressedTexImage2D )( GLenum target, GLint level,
												   GLenum internalFormat, GLsizei width,
												   GLsizei height, GLint border, GLsizei imageSize,
												   const void* data );
typedef void( APIENTRY* pglGetCompressedTexImage )( GLenum target, GLint level, void* pixels );
typedef void( APIENTRY* pglBindVertexArray )( GLuint array );
typedef void( APIENTRY* pglDeleteVertexArrays )( GLsizei n, const GLuint* arrays );
typedef void( APIENTRY* pglGenVertexArrays )( GLsizei n, GLuint* arrays );

}} // namespace EE::Graphics

#endif
