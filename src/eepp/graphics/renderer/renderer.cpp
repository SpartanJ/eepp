#include <SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/renderer/renderergl.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>
#include <eepp/graphics/renderer/renderergl3cp.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace Graphics {

#ifndef APIENTRY
#define APIENTRY
#endif

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

Renderer* GLi = NULL;

Renderer* Renderer::sSingleton = NULL;

GraphicsLibraryVersion Renderer::glVersionFromString( std::string glVersion ) {
	GraphicsLibraryVersion GLVer;
	String::toLowerInPlace( glVersion );
	if ( "3" == glVersion || "opengl 3" == glVersion || "gl3" == glVersion ||
		 "opengl3" == glVersion )
		GLVer = GLv_3;
	else if ( "4" == glVersion || "opengl es 2" == glVersion || "gles2" == glVersion ||
			  "opengles2" == glVersion || "es2" == glVersion )
		GLVer = GLv_ES2;
	else if ( "5" == glVersion || "opengl 3 core profile" == glVersion || "gl3cp" == glVersion ||
			  "opengl3cp" == glVersion || "opengl core profile" == glVersion ||
			  "core profile" == glVersion || "cp" == glVersion )
		GLVer = GLv_3CP;
	else if ( "opengl es 1" == glVersion || "gles1" == glVersion || "gl es 1" == glVersion ||
			  "opengl es1" == glVersion || "opengles1" == glVersion || "es1" == glVersion ||
			  "gles 1" == glVersion )
		GLVer = GLv_ES1;
	else if ( "2" == glVersion || "opengl 2" == glVersion || "gl2" == glVersion ||
			  "gl 2" == glVersion )
		GLVer = GLv_2;
	else
		GLVer = GLv_default;
	return GLVer;
}

std::string Renderer::graphicsLibraryVersionToString( const GraphicsLibraryVersion& glVersion ) {
	switch ( glVersion ) {
		case GLv_2:
			return "OpenGL 2";
		case GLv_3:
			return "OpenGL 3";
		case GLv_3CP:
			return "OpenGL 3 Core Profile";
		case GLv_ES1:
			return "OpenGL ES 1";
		case GLv_ES2:
			return "OpenGL ES 2";
		case GLv_default:
		default:
			return graphicsLibraryVersionToString( getDefaultGraphicsLibraryVersion() );
	}
}

GraphicsLibraryVersion Renderer::getDefaultGraphicsLibraryVersion() {
	GraphicsLibraryVersion ver = GLv_2;
#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )
	ver = GLv_2;
#else
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#ifdef EE_GLES2
	ver = GLv_ES2;
#else
	ver = GLv_ES1;
#endif
#else
#ifndef EE_GLES1_DEFAULT
#ifdef EE_GLES2
	ver = GLv_ES2;
#else
	ver = GLv_ES1;
#endif
#else
	ver = GLv_ES1;
#endif
#endif
#endif
	return ver;
}

std::vector<GraphicsLibraryVersion> Renderer::getAvailableGraphicsLibraryVersions() {
	std::vector<GraphicsLibraryVersion> vers;
#if ( defined( EE_GLES1 ) && !defined( EE_GLES2 ) ) || defined( EE_GLES_BOTH )
	vers.emplace_back( GLv_ES1 );
#else
	vers.emplace_back( GLv_2 );
#endif
#ifdef EE_GL3_ENABLED
	vers.emplace_back( GLv_3 );
	vers.emplace_back( GLv_3CP );
	vers.emplace_back( GLv_ES2 );
#endif
	return vers;
}

Renderer* Renderer::createSingleton( GraphicsLibraryVersion ver ) {
	if ( ver == GLv_default )
		ver = getDefaultGraphicsLibraryVersion();

	switch ( ver ) {
		case GLv_ES2: {
#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			sSingleton = eeNew( RendererGLES2, () );
			break;
#endif
		}
		case GLv_3: {
#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			sSingleton = eeNew( RendererGL3, () );
			break;
#endif
		}
		case GLv_3CP: {
#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			sSingleton = eeNew( RendererGL3CP, () );
			break;
#endif
		}
		case GLv_2:
		case GLv_ES1:
		case GLv_default:
		default: {
#ifndef EE_GLES2
			sSingleton = eeNew( RendererGL, () );
#endif
		}
	}

	return sSingleton;
}

Renderer* Renderer::createSingleton() {
	if ( sSingleton == 0 ) {
#if defined( EE_GLES_BOTH )
		sSingleton = eeNew( RendererGL, () );
#elif defined( EE_GLES2 )
		sSingleton = eeNew( RendererGLES2, () );
#elif defined( EE_GLES1 )
		sSingleton = eeNew( RendererGL, () );
#else
		sSingleton = eeNew( RendererGL, () );
#endif
	}

	return sSingleton;
}

Renderer* Renderer::existsSingleton() {
	return sSingleton;
}

Renderer* Renderer::instance() {
	return createSingleton();
}

void Renderer::destroySingleton() {
	if ( sSingleton != 0 ) {
		eeDelete( sSingleton );
		sSingleton = 0;
	}
}

Renderer::Renderer() :
	mExtensions( 0 ),
	mStateFlags( 1 << RSF_LINE_SMOOTH ),
	mQuadsSupported( true ),
	mQuadVertexs( 4 ),
	mLineWidth( 1 ),
	mCurVAO( 0 ),
	mClippingMask( eeNew( ClippingMask, () ) ) {
	GLi = this;
}

Renderer::~Renderer() {
	eeSAFE_DELETE( mClippingMask );
	GLi = NULL;
}

RendererGL* Renderer::getRendererGL() {
	return reinterpret_cast<RendererGL*>( this );
}

RendererGL3* Renderer::getRendererGL3() {
	return reinterpret_cast<RendererGL3*>( this );
}

RendererGL3CP* Renderer::getRendererGL3CP() {
	return reinterpret_cast<RendererGL3CP*>( this );
}

RendererGLES2* Renderer::getRendererGLES2() {
	return reinterpret_cast<RendererGLES2*>( this );
}

void Renderer::writeExtension( Uint8 Pos, Uint32 BitWrite ) {
	BitOp::writeBitKey( &mExtensions, Pos, BitWrite );
}

void Renderer::init() {
#ifdef EE_GLEW_AVAILABLE
#if EE_PLATFORM != EE_PLATFORM_MACOS
	glewExperimental = 1;
#endif
	bool glewOn = ( GLEW_OK == glewInit() );

	if ( glewOn ) {
		writeExtension( EEGL_ARB_texture_non_power_of_two, GLEW_ARB_texture_non_power_of_two );
		writeExtension( EEGL_ARB_point_parameters, GLEW_ARB_point_parameters );
		writeExtension( EEGL_ARB_point_sprite, GLEW_ARB_point_sprite );
		writeExtension( EEGL_ARB_shading_language_100, GLEW_ARB_shading_language_100 );
		writeExtension( EEGL_ARB_shader_objects, GLEW_ARB_shader_objects );
		writeExtension( EEGL_ARB_vertex_shader, GLEW_ARB_vertex_shader );
		writeExtension( EEGL_ARB_fragment_shader, GLEW_ARB_fragment_shader );
		writeExtension( EEGL_EXT_framebuffer_object, GLEW_EXT_framebuffer_object );
		writeExtension( EEGL_ARB_multitexture, GLEW_ARB_multitexture );
		writeExtension( EEGL_EXT_texture_compression_s3tc, GLEW_EXT_texture_compression_s3tc );
		writeExtension( EEGL_ARB_vertex_buffer_object, GLEW_ARB_vertex_buffer_object );
		writeExtension( EEGL_ARB_pixel_buffer_object, GLEW_ARB_pixel_buffer_object );
		writeExtension( EEGL_ARB_vertex_array_object, GLEW_ARB_vertex_array_object );
		writeExtension( EEGL_EXT_blend_func_separate, GLEW_EXT_blend_func_separate );
		writeExtension( EEGL_EXT_blend_minmax, GLEW_EXT_blend_minmax );
		writeExtension( EEGL_EXT_blend_subtract, GLEW_EXT_blend_subtract );
	} else
#endif
	{
		writeExtension( EEGL_ARB_texture_non_power_of_two,
						isExtension( "GL_ARB_texture_non_power_of_two" ) );
		writeExtension( EEGL_ARB_point_parameters, isExtension( "GL_ARB_point_parameters" ) );
		writeExtension( EEGL_ARB_point_sprite, isExtension( "GL_ARB_point_sprite" ) );
		writeExtension( EEGL_ARB_shading_language_100,
						isExtension( "GL_ARB_shading_language_100" ) );
		writeExtension( EEGL_ARB_shader_objects, isExtension( "GL_ARB_shader_objects" ) );
		writeExtension( EEGL_ARB_vertex_shader, isExtension( "GL_ARB_vertex_shader" ) );
		writeExtension( EEGL_ARB_fragment_shader, isExtension( "GL_ARB_fragment_shader" ) );
		writeExtension( EEGL_EXT_framebuffer_object, isExtension( "GL_EXT_framebuffer_object" ) );
		writeExtension( EEGL_ARB_multitexture, isExtension( "GL_ARB_multitexture" ) );
		writeExtension( EEGL_EXT_texture_compression_s3tc,
						isExtension( "GL_EXT_texture_compression_s3tc" ) );
		writeExtension( EEGL_ARB_vertex_buffer_object,
						isExtension( "GL_ARB_vertex_buffer_object" ) );
		writeExtension( EEGL_ARB_pixel_buffer_object, isExtension( "GL_ARB_pixel_buffer_object" ) );
		writeExtension( EEGL_ARB_vertex_array_object, isExtension( "GL_ARB_vertex_array_object" ) );
		writeExtension( EEGL_EXT_blend_func_separate, isExtension( "GL_EXT_blend_func_separate" ) );
		writeExtension( EEGL_EXT_blend_minmax, isExtension( "GL_EXT_blend_minmax" ) );
		writeExtension( EEGL_EXT_blend_subtract, isExtension( "GL_EXT_blend_subtract" ) );
	}

	// NVIDIA added support for GL_OES_compressed_ETC1_RGB8_texture in desktop GPUs
	// GLEW doesn't return the correct result
	writeExtension( EEGL_OES_compressed_ETC1_RGB8_texture,
					isExtension( "GL_OES_compressed_ETC1_RGB8_texture" ) );

#ifdef EE_GLES

	writeExtension( EEGL_ARB_point_parameters, 1 );
	writeExtension( EEGL_ARB_point_sprite, 1 );
	writeExtension( EEGL_ARB_multitexture, 1 );

	writeExtension( EEGL_EXT_blend_minmax, 1 );
	writeExtension( EEGL_EXT_blend_subtract, 1 );

	writeExtension( EEGL_IMG_texture_compression_pvrtc,
					isExtension( "GL_IMG_texture_compression_pvrtc" ) );

	if ( !isExtension( EEGL_EXT_texture_compression_s3tc ) ) {
		writeExtension( EEGL_EXT_texture_compression_s3tc,
						isExtension( "GL_OES_texture_compression_S3TC" ) );
	}

	if ( !isExtension( EEGL_EXT_framebuffer_object ) ) {
		writeExtension( EEGL_EXT_framebuffer_object, isExtension( "GL_OES_framebuffer_object" ) );
	}

	if ( !isExtension( EEGL_ARB_texture_non_power_of_two ) ) {
		writeExtension( EEGL_ARB_texture_non_power_of_two,
						isExtension( "GL_IMG_texture_npot" ) ||
							isExtension( "GL_OES_texture_npot" ) ||
							isExtension( "GL_APPLE_texture_2D_limited_npot" ) );
	}

	if ( !isExtension( EEGL_ARB_vertex_array_object ) ) {
		writeExtension( EEGL_ARB_vertex_array_object, isExtension( "GL_OES_vertex_array_object" ) );
	}

	if ( !isExtension( EEGL_EXT_blend_func_separate ) ) {
		writeExtension( EEGL_EXT_blend_func_separate, isExtension( "GL_OES_blend_func_separate" ) );
	}

#endif

#ifdef EE_GLES2
	if ( GLv_ES2 == version() || GLv_3CP == version() ) {
		writeExtension( EEGL_EXT_framebuffer_object, 1 );
		writeExtension( EEGL_ARB_vertex_buffer_object, 1 );
		writeExtension( EEGL_ARB_shader_objects, 1 );
		writeExtension( EEGL_ARB_vertex_shader, 1 );
		writeExtension( EEGL_ARB_fragment_shader, 1 );
		writeExtension( EEGL_EXT_blend_func_separate, 1 );
	}
#endif

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	if ( !isExtension( EEGL_EXT_texture_compression_s3tc ) ) {
		writeExtension( EEGL_EXT_texture_compression_s3tc,
						isExtension( "WEBGL_compressed_texture_s3tc" ) ||
							isExtension( "WEBKIT_WEBGL_compressed_texture_s3tc" ) ||
							isExtension( "MOZ_WEBGL_compressed_texture_s3tc" ) );
	}

	writeExtension( EEGL_ARB_texture_non_power_of_two, 1 );
#endif
}

bool Renderer::isExtension( const std::string& name ) {
#ifdef EE_GLEW_AVAILABLE
	return 0 != glewIsSupported( name.c_str() );
#else
	return 0 != SOIL_GL_ExtensionSupported( name.c_str() );
#endif
}

bool Renderer::isExtension( GraphicsLibraryExtension name ) {
	return 0 != ( mExtensions & ( 1 << name ) );
}

bool Renderer::pointSpriteSupported() {
#ifdef EE_GLES
	return true;
#else
	return isExtension( EEGL_ARB_point_sprite );
#endif
}

bool Renderer::shadersSupported() {
#ifdef EE_GLES
	return ( GLv_ES2 == version() || GLv_3 == version() || GLv_3CP == version() );
#else
	return GLv_3CP == version() ||
		   ( isExtension( EEGL_ARB_shader_objects ) && isExtension( EEGL_ARB_vertex_shader ) &&
			 isExtension( EEGL_ARB_fragment_shader ) );
#endif
}

std::string Renderer::getExtensions() {
	std::string exts;

#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS
	if ( GLv_3 == version() || GLv_3CP == version() ) {
		static pglGetStringiFunc eeglGetStringiFunc = NULL;

		int num_exts = 0;
		int i;

		if ( NULL == eeglGetStringiFunc ) {
			eeglGetStringiFunc = (pglGetStringiFunc)SOIL_GL_GetProcAddress( "glGetStringi" );

			if ( NULL == eeglGetStringiFunc ) {
				return "";
			}
		}

#ifndef GL_NUM_EXTENSIONS
#define GL_NUM_EXTENSIONS 0x821D
#endif
		glGetIntegerv( GL_NUM_EXTENSIONS, &num_exts );
		for ( i = 0; i < num_exts; i++ ) {
			const char* thisext = (const char*)eeglGetStringiFunc( GL_EXTENSIONS, i );

			exts += std::string( thisext ) + " ";
		}

		return exts;
	}
#endif

	const char* extsc = (const char*)glGetString( GL_EXTENSIONS );

	if ( NULL != extsc ) {
		exts = std::string( extsc );
	}

	return exts;
}

void Renderer::viewport( int x, int y, int width, int height ) {
	glViewport( x, y, width, height );
}

void Renderer::disable( unsigned int cap ) {
	glDisable( cap );
}

void Renderer::enable( unsigned int cap ) {
	glEnable( cap );
}

const char* Renderer::getString( unsigned int name ) {
	return (const char*)glGetString( name );
}

void Renderer::clear( unsigned int mask ) {
	glClear( mask );
}

void Renderer::clearColor( float red, float green, float blue, float alpha ) {
	glClearColor( red, green, blue, alpha );
}

void Renderer::scissor( int x, int y, int width, int height ) {
	glScissor( x, y, width, height );
}

void Renderer::polygonMode( unsigned int face, unsigned int mode ) {
#ifndef EE_GLES
	glPolygonMode( face, mode );
#endif
}

void Renderer::drawArrays( unsigned int mode, int first, int count ) {
	glDrawArrays( mode, first, count );
}

void Renderer::drawElements( unsigned int mode, int count, unsigned int type,
							 const void* indices ) {
	glDrawElements( mode, count, type, indices );
}

void Renderer::bindTexture( unsigned int target, unsigned int texture ) {
	if ( GLv_3CP == version() && 0 == texture )
		return;
	glBindTexture( target, texture );
}

void Renderer::activeTexture( unsigned int texture ) {
	glActiveTexture( texture );
}

void Renderer::blendFunc( unsigned int sfactor, unsigned int dfactor ) {
	glBlendFunc( sfactor, dfactor );
}

void Renderer::blendFuncSeparate( unsigned int sfactorRGB, unsigned int dfactorRGB,
								  unsigned int sfactorAlpha, unsigned int dfactorAlpha ) {
	static pglBlendFuncSeparate eeglBlendFuncSeparate = NULL;

	if ( NULL == eeglBlendFuncSeparate )
		eeglBlendFuncSeparate = (pglBlendFuncSeparate)getProcAddress( "glBlendFuncSeparate" );

	if ( NULL != eeglBlendFuncSeparate )
		eeglBlendFuncSeparate( sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha );
}

void Renderer::blendEquationSeparate( unsigned int modeRGB, unsigned int modeAlpha ) {
	static pglBlendEquationSeparate eeglBlendEquationSeparate = NULL;

	if ( NULL == eeglBlendEquationSeparate )
		eeglBlendEquationSeparate =
			(pglBlendEquationSeparate)getProcAddress( "glBlendEquationSeparate" );

	if ( NULL != eeglBlendEquationSeparate )
		eeglBlendEquationSeparate( modeRGB, modeAlpha );
}

void Renderer::blitFrameBuffer( int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
								int dstX1, int dstY1, unsigned int mask, unsigned int filter ) {
	static pglBlitFramebufferEXT eeglBlitFramebufferEXT = NULL;

	if ( NULL == eeglBlitFramebufferEXT )
		eeglBlitFramebufferEXT = (pglBlitFramebufferEXT)getProcAddress( "glBlitFramebufferEXT" );

	if ( NULL != eeglBlitFramebufferEXT )
		eeglBlitFramebufferEXT( srcX0, srcY1, srcX1, srcY0, dstX0, dstY0, dstX1, dstY1, mask,
								filter );
}

void Renderer::setShader( ShaderProgram* Shader ) {
#ifdef EE_SHADERS_SUPPORTED
	if ( NULL != Shader ) {
		glUseProgram( Shader->getHandler() );
	} else {
		glUseProgram( 0 );
	}
#endif
}

bool Renderer::isLineSmooth() {
	return BitOp::readBitKey( &mStateFlags, RSF_LINE_SMOOTH );
}

void Renderer::polygonSmooth( const bool& Enable ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && defined( GL_POLYGON_SMOOTH ) && \
	!defined( EE_GLES1 ) && !defined( EE_GLES2 ) && !defined( EE_GLES_BOTH )
	if ( Enable ) {
		enable( GL_POLYGON_SMOOTH );
	} else {
		disable( GL_POLYGON_SMOOTH );
	}

	BitOp::writeBitKey( &mStateFlags, RSF_POLYGON_SMOOTH, Enable ? 1 : 0 );
#endif
}

void Renderer::polygonSmooth() {
	polygonSmooth( isPolygonSmooth() );
}

bool Renderer::isPolygonSmooth() {
	return BitOp::readBitKey( &mStateFlags, RSF_POLYGON_SMOOTH );
}

void Renderer::lineSmooth() {
	lineSmooth( isLineSmooth() );
}

void Renderer::lineSmooth( const bool& Enable ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( Enable ) {
		enable( GL_LINE_SMOOTH );
	} else {
		disable( GL_LINE_SMOOTH );
	}

	BitOp::writeBitKey( &mStateFlags, RSF_LINE_SMOOTH, Enable ? 1 : 0 );
#endif
}

void Renderer::lineWidth( float width ) {
	if ( width != mLineWidth ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		if ( GLv_3CP != version() )
#endif
		{
			glLineWidth( width );
		}
		mLineWidth = width;
	}
}

void Renderer::polygonMode() {
	PrimitiveFillMode Mode = DRAW_FILL;

	if ( BitOp::readBitKey( &mStateFlags, RSF_POLYGON_MODE ) )
		Mode = DRAW_LINE;

	polygonMode( Mode );
}

void Renderer::pixelStorei( unsigned int pname, int param ) {
	glPixelStorei( pname, param );
}

void Renderer::multisample( bool enabled ) {
	if ( enabled )
		enable( GL_MULTISAMPLE );
	else
		disable( GL_MULTISAMPLE );

	BitOp::writeBitKey( &mStateFlags, RSF_MULTISAMPLE, enabled ? 1 : 0 );
}

bool Renderer::isMultisample() {
	return BitOp::readBitKey( &mStateFlags, RSF_MULTISAMPLE );
}

void Renderer::polygonMode( const PrimitiveFillMode& Mode ) {
	if ( Mode == DRAW_FILL )
		polygonMode( GL_FRONT_AND_BACK, GL_FILL );
	else
		polygonMode( GL_FRONT_AND_BACK, GL_LINE );

	BitOp::writeBitKey( &mStateFlags, RSF_POLYGON_MODE, Mode == DRAW_LINE ? 1 : 0 );
}

std::string Renderer::getVendor() {
	const char* str = getString( GL_VENDOR );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string Renderer::getRenderer() {
	const char* str = getString( GL_RENDERER );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string Renderer::getVersion() {
	const char* str = getString( GL_VERSION );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string Renderer::getShadingLanguageVersion() {
	if ( shadersSupported() ) {
#ifdef GL_SHADING_LANGUAGE_VERSION
		const char* str = getString( GL_SHADING_LANGUAGE_VERSION );

		if ( NULL != str )
			return std::string( str );
#endif
	}

	return std::string( "Shaders not supported" );
}

void Renderer::getViewport( int* viewport ) {
	glGetIntegerv( GL_VIEWPORT, viewport );
}

Vector3f Renderer::projectCurrent( const Vector3f& point ) {
	float projMat[16];
	getCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	float modelMat[16];
	getCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	int viewPort[4];
	getViewport( viewPort );

	Vector3f fPoint( point );
	fPoint.y = viewPort[3] - point.y;

	Vector3<float> tv3;

	project( (float)fPoint.x, (float)fPoint.y, (float)fPoint.z, projMat, modelMat, viewPort, &tv3.x,
			 &tv3.y, &tv3.z );

	return Vector3f( tv3.x, tv3.y, tv3.z );
}

Vector3f Renderer::unProjectCurrent( const Vector3f& point ) {
	float projMat[16];
	getCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	float modelMat[16];
	getCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	int viewPort[4];
	getViewport( viewPort );

	Vector3f fPoint( point );
	fPoint.y = viewPort[3] - point.y;

	Vector3<float> tv3;

	unProject( (float)fPoint.x, (float)fPoint.y, (float)fPoint.z, projMat, modelMat, viewPort,
			   &tv3.x, &tv3.y, &tv3.z );

	return Vector3f( tv3.x, tv3.y, tv3.z );
}

void Renderer::stencilFunc( unsigned int func, int ref, unsigned int mask ) {
	glStencilFunc( func, ref, mask );
}

void Renderer::stencilOp( unsigned int fail, unsigned int zfail, unsigned int zpass ) {
	glStencilOp( fail, zfail, zpass );
}

void Renderer::stencilMask( unsigned int mask ) {
	glStencilMask( mask );
}

void Renderer::colorMask( Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha ) {
	glColorMask( red, green, blue, alpha );
}

const int& Renderer::quadVertexs() const {
	return mQuadVertexs;
}

ClippingMask* Renderer::getClippingMask() const {
	return mClippingMask;
}

void* Renderer::getProcAddress( std::string proc ) {
	void* addr = NULL;

#ifdef EE_GLES
	if ( version() == GLv_ES1 )
		addr = SOIL_GL_GetProcAddress( ( proc + "OES" ).c_str() );
#endif

	if ( NULL == addr )
		addr = SOIL_GL_GetProcAddress( proc.c_str() );

	if ( NULL == addr )
		addr = SOIL_GL_GetProcAddress( ( proc + "EXT" ).c_str() );

	return addr;
}

void Renderer::readPixels( int x, int y, unsigned int width, unsigned int height, void* pixels ) {
	glReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
}

Color Renderer::readPixel( int x, int y ) {
	Uint8 pixel[4];
	readPixels( x, y, 1, 1, pixel );
	return Color( pixel[0], pixel[1], pixel[2], pixel[3] );
}

void Renderer::genFramebuffers( int n, unsigned int* framebuffers ) {
	static pglGenFramebuffers eeglGenFramebuffers = NULL;

	if ( NULL == eeglGenFramebuffers )
		eeglGenFramebuffers = (pglGenFramebuffers)getProcAddress( "glGenFramebuffers" );

	if ( NULL != eeglGenFramebuffers )
		eeglGenFramebuffers( n, framebuffers );
}

void Renderer::bindFramebuffer( unsigned int target, unsigned int framebuffer ) {
	static pglBindFramebuffer eeglBindFramebuffer = NULL;

	if ( NULL == eeglBindFramebuffer )
		eeglBindFramebuffer = (pglBindFramebuffer)getProcAddress( "glBindFramebuffer" );

	if ( NULL != eeglBindFramebuffer )
		eeglBindFramebuffer( target, framebuffer );
}

void Renderer::framebufferTexture2D( unsigned int target, unsigned int attachment,
									 unsigned int textarget, unsigned int texture, int level ) {
	static pglFramebufferTexture2D eeglFramebufferTexture2D = NULL;

	if ( NULL == eeglFramebufferTexture2D )
		eeglFramebufferTexture2D =
			(pglFramebufferTexture2D)getProcAddress( "glFramebufferTexture2D" );

	if ( NULL != eeglFramebufferTexture2D )
		eeglFramebufferTexture2D( target, attachment, textarget, texture, level );
}

void Renderer::genRenderbuffers( int n, unsigned int* renderbuffers ) {
	static pglGenRenderbuffers eeglGenRenderbuffers = NULL;

	if ( NULL == eeglGenRenderbuffers )
		eeglGenRenderbuffers = (pglGenRenderbuffers)getProcAddress( "glGenRenderbuffers" );

	if ( NULL != eeglGenRenderbuffers )
		eeglGenRenderbuffers( n, renderbuffers );
}

void Renderer::deleteRenderbuffers( int n, const unsigned int* renderbuffers ) {
	static pglDeleteRenderbuffers eeglDeleteRenderbuffers = NULL;

	if ( NULL == eeglDeleteRenderbuffers )
		eeglDeleteRenderbuffers = (pglDeleteRenderbuffers)getProcAddress( "glDeleteRenderbuffers" );

	if ( NULL != eeglDeleteRenderbuffers )
		eeglDeleteRenderbuffers( n, renderbuffers );
}

void Renderer::bindRenderbuffer( unsigned int target, unsigned int renderbuffer ) {
	static pglBindRenderbuffer eeglBindRenderbuffer = NULL;

	if ( NULL == eeglBindRenderbuffer )
		eeglBindRenderbuffer = (pglBindRenderbuffer)getProcAddress( "glBindRenderbuffer" );

	if ( NULL != eeglBindRenderbuffer )
		eeglBindRenderbuffer( target, renderbuffer );
}

void Renderer::renderbufferStorage( unsigned int target, unsigned int internalformat, int width,
									int height ) {
	static pglRenderbufferStorage eeglRenderbufferStorage = NULL;

	if ( NULL == eeglRenderbufferStorage )
		eeglRenderbufferStorage = (pglRenderbufferStorage)getProcAddress( "glRenderbufferStorage" );

	if ( NULL != eeglRenderbufferStorage )
		eeglRenderbufferStorage( target, internalformat, width, height );
}

void Renderer::framebufferRenderbuffer( unsigned int target, unsigned int attachment,
										unsigned int renderbuffertarget,
										unsigned int renderbuffer ) {
	static pglFramebufferRenderbuffer eeglFramebufferRenderbuffer = NULL;

	if ( NULL == eeglFramebufferRenderbuffer )
		eeglFramebufferRenderbuffer =
			(pglFramebufferRenderbuffer)getProcAddress( "glFramebufferRenderbuffer" );

	if ( NULL != eeglFramebufferRenderbuffer )
		eeglFramebufferRenderbuffer( target, attachment, renderbuffertarget, renderbuffer );
}

unsigned int Renderer::checkFramebufferStatus( unsigned int target ) {
	static pglCheckFramebufferStatus eeglCheckFramebufferStatus = NULL;

	if ( NULL == eeglCheckFramebufferStatus )
		eeglCheckFramebufferStatus =
			(pglCheckFramebufferStatus)getProcAddress( "glCheckFramebufferStatus" );

	if ( NULL != eeglCheckFramebufferStatus )
		return (unsigned int)eeglCheckFramebufferStatus( target );

	return 0;
}

void Renderer::discardFramebuffer( unsigned int target, int numAttachments,
								   const unsigned int* attachments ) {
	static pglDiscardFramebufferEXT eeglDiscardFramebuffer = NULL;

	if ( NULL == eeglDiscardFramebuffer )
		eeglDiscardFramebuffer = (pglDiscardFramebufferEXT)getProcAddress( "glDiscardFramebuffer" );

	if ( NULL != eeglDiscardFramebuffer )
		return eeglDiscardFramebuffer( target, numAttachments, attachments );
}

void Renderer::deleteFramebuffers( int n, const unsigned int* framebuffers ) {
	static pglDeleteFramebuffers eeglDeleteFramebuffers = NULL;

	if ( NULL == eeglDeleteFramebuffers )
		eeglDeleteFramebuffers = (pglDeleteFramebuffers)getProcAddress( "glDeleteFramebuffers" );

	if ( NULL != eeglDeleteFramebuffers )
		eeglDeleteFramebuffers( n, framebuffers );
}

void Renderer::bindVertexArray( unsigned int array ) {
#if !defined( EE_GLES )
	if ( mCurVAO != array ) {
		glBindVertexArray( array );

		mCurVAO = array;
	}
#endif
}

void Renderer::deleteVertexArrays( int n, const unsigned int* arrays ) {
#if !defined( EE_GLES )
	glDeleteVertexArrays( n, arrays );
#endif
}

void Renderer::genVertexArrays( int n, unsigned int* arrays ) {
#if !defined( EE_GLES )
	glGenVertexArrays( n, arrays );
#endif
}

const bool& Renderer::quadsSupported() const {
	return mQuadsSupported;
}

}} // namespace EE::Graphics
