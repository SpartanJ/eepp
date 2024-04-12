#ifndef EE_OPENGL_HPP
#define EE_OPENGL_HPP

#include <eepp/config.hpp>

#ifdef EE_USE_DOUBLES
#define GL_FP GL_DOUBLE
#else
#define GL_FP GL_FLOAT
#endif

#ifndef EE_GLES
//! GL2 and GL3 ( PC platform )

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <OpenGL/gl.h>
#else
#if EE_PLATFORM == EE_PLATFORM_WIN
#define APIENTRY __stdcall
#define WINGDIAPI __declspec( dllimport )
#endif

#include <GL/gl.h>
#endif

#ifndef EE_GLEW_AVAILABLE
#if defined( EE_X11_PLATFORM )
#include <GL/glext.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include <OpenGL/glext.h>
#endif
#endif

#else
//! Mobile platform ( Android / iPhone / Maemo )
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#ifdef EE_GLES_BOTH
#if EE_PLATFORM == EE_PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#define EE_GLES1_LATE_INCLUDE
#else
//! GLES2 ( programmable pipeline )
#ifdef EE_GLES2
#if EE_PLATFORM == EE_PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#endif

//! GLES1 ( fixed pipeline )
#ifdef EE_GLES1
#if EE_PLATFORM == EE_PLATFORM_IOS
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#else
#include <GLES/gl.h>
#include <GLES/glext.h>
#endif
#endif
#endif
#endif

#ifdef EE_GLES
typedef char GLchar;

#define glDeleteBuffersARB glDeleteBuffers
#define glGenBuffersARB glGenBuffers
#define glBindBufferARB glBindBuffer
#define glBufferDataARB glBufferData
#define glBufferSubDataARB glBufferSubData

#define GL_QUADS 0x0007
#ifndef GL_RGB8
#define GL_RGB8 GL_RGB8_OES
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 GL_RGBA8_OES
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER GL_READ_FRAMEBUFFER_APPLE
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER GL_DRAW_FRAMEBUFFER_APPLE
#endif

#ifndef glBlitFramebufferEXT
#define glBlitFramebufferEXT( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 )
#endif
#endif

/// Wrap GLES2 functions
#if defined( EE_GLES2 )

#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#define glGenFramebuffersEXT glGenFramebuffers
#define glGenRenderbuffersEXT glGenRenderbuffers
#define glBindFramebufferEXT glBindFramebuffer
#define glRenderbufferStorageEXT glRenderbufferStorage
#define glBindRenderbufferEXT glBindRenderbuffer
#define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glVertexAttribPointerARB glVertexAttribPointer
#define glDeleteRenderbuffersEXT glDeleteRenderbuffers

#define glGenVertexArrays glGenVertexArraysOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#define GL_VERTEX_ARRAY_BINDING GL_VERTEX_ARRAY_BINDING_OES

#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX GL_STENCIL_INDEX_OES
#endif

#endif

/// Exclusive only when its compiling only for GLES2
#if ( defined( EE_GLES2 ) && !defined( EE_GLES1 ) ) || defined( EE_GLES_BOTH )

#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_TEXTURE_MATRIX 0x0BA8
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_INDEX_ARRAY 0x8077
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_EDGE_FLAG_ARRAY 0x8079
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_LINE_SMOOTH 0x0B20
#define GL_LIGHTING 0x0B50
#define GL_CLIP_PLANE0 0x3000
#define GL_CLIP_PLANE1 0x3001
#define GL_CLIP_PLANE2 0x3002
#define GL_CLIP_PLANE3 0x3003
#define GL_CLIP_PLANE4 0x3004
#define GL_CLIP_PLANE5 0x3005
#define GL_POINT_SPRITE 0x8861
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642

#define glBlendFuncSeparateEXT glBlendFuncSeparate

#endif

#if defined( EE_GLES1 ) && !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )

#define glOrtho glOrthof
#define glClipPlane glClipPlanef
#define glFrustum glFrustumf

#define GL_COORD_REPLACE GL_COORD_REPLACE_OES
#define GL_COMBINE_ARB GL_COMBINE

#endif

/// Exclusive when its compiling only for GLES1
#if defined( EE_GLES1 ) && !defined( EE_GLES2 )

#define GL_POINT_SPRITE GL_POINT_SPRITE_OES

#define glCheckFramebufferStatusEXT glCheckFramebufferStatusOES
#define glDeleteFramebuffersEXT glDeleteFramebuffersOES
#define glGenFramebuffersEXT glGenFramebuffersOES
#define glGenRenderbuffersEXT glGenRenderbuffersOES
#define glBindFramebufferEXT glBindFramebufferOES
#define glRenderbufferStorageEXT glRenderbufferStorageOES
#define glBindRenderbufferEXT glBindRenderbufferOES
#define glFramebufferRenderbufferEXT glFramebufferRenderbufferOES
#define glFramebufferTexture2DEXT glFramebufferTexture2DOES
#define glBlendFuncSeparateEXT glBlendFuncSeparateOES
#define glDeleteRenderbuffersEXT glDeleteRenderbuffersOES

#define GL_FRAMEBUFFER GL_FRAMEBUFFER_OES
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACEGL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_OES
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT \
	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS GL_FRAMEBUFFER_INCOMPLETE_FORMATS_OES
#define GL_FRAMEBUFFER_UNSUPPORTED GL_FRAMEBUFFER_UNSUPPORTED_OES
#define GL_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING_OES

#define GL_RENDERBUFFER GL_RENDERBUFFER_OES
#define GL_RENDERBUFFER_WIDTH GL_RENDERBUFFER_WIDTH_OES
#define GL_RENDERBUFFER_HEIGHT GL_RENDERBUFFER_HEIGHT_OES
#define GL_RENDERBUFFER_INTERNAL_FORMAT GL_RENDERBUFFER_INTERNAL_FORMAT_OES
#define GL_RENDERBUFFER_RED_SIZE GL_RENDERBUFFER_RED_SIZE_OES
#define GL_RENDERBUFFER_GREEN_SIZE GL_RENDERBUFFER_GREEN_SIZE_OES
#define GL_RENDERBUFFER_BLUE_SIZE GL_RENDERBUFFER_BLUE_SIZE_OES
#define GL_RENDERBUFFER_ALPHA_SIZE GL_RENDERBUFFER_ALPHA_SIZE_OES
#define GL_RENDERBUFFER_DEPTH_SIZE GL_RENDERBUFFER_DEPTH_SIZE_OES
#define GL_RENDERBUFFER_STENCIL_SIZE GL_RENDERBUFFER_STENCIL_SIZE_OES
#define GL_RENDERBUFFER_BINDING GL_RENDERBUFFER_BINDING_OES

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31

#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT GL_UNSIGNED_INT_24_8_OES
#endif

#define GL_LINE 0x1B01
#define GL_FILL 0x1B02

#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA

#ifdef GL_DEPTH_COMPONENT32_OES
#define GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT32_OES
#else
#define GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT16_OES
#endif

#ifdef GL_STENCIL_INDEX8_OES
#define GL_STENCIL_INDEX GL_STENCIL_INDEX8_OES
#endif

#define GL_DEPTH_ATTACHMENT GL_DEPTH_ATTACHMENT_OES
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_OES
#define GL_STENCIL_ATTACHMENT GL_STENCIL_ATTACHMENT_OES

#define GL_SUBTRACT_ARB GL_SUBTRACT
#define GL_ADD_SIGNED_ARB GL_ADD_SIGNED
#define GL_INTERPOLATE_ARB GL_INTERPOLATE
#define GL_DOT3_RGB_ARB GL_DOT3_RGB
#define GL_DOT3_RGBA_ARB GL_DOT3_RGBA

#define GL_CONSTANT_ARB GL_CONSTANT
#define GL_PRIMARY_COLOR_ARB GL_PRIMARY_COLOR
#define GL_PREVIOUS_ARB GL_PREVIOUS

#define glGenVertexArrays( a, b )
#define glBindVertexArray( a )
#define glDeleteVertexArrays( a, b )

#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif

#define GL_FUNC_ADD GL_FUNC_ADD_OES
#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES

#define GL_FRAMEBUFFER GL_FRAMEBUFFER_OES
#define GL_RENDERBUFFER GL_RENDERBUFFER_OES

#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#include <eepp/graphics/renderer/rendererhelper.hpp>

#endif
