#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/renderer/renderergl.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>
#include <eepp/graphics/renderer/renderergl3cp.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>

namespace EE { namespace Graphics {

typedef const GLubyte *( * pglGetStringiFunc) (unsigned int, unsigned int);

cGL * GLi = NULL;

cGL * cGL::ms_singleton = NULL;

cGL * cGL::createSingleton( EEGL_version ver ) {
	#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )
	if ( GLv_default == ver )
		ver = GLv_2;
	#else
		#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		if ( GLv_default == ver )
			#ifdef EE_GLES1
			ver = GLv_ES1;
			#else
			ver = GLv_ES2;
			#endif
		#else
			if ( GLv_default == ver )
				#ifndef EE_GLES1_DEFAULT
					#ifdef EE_GLES2
					ver = GLv_3CP;
					#else
					ver = GLv_ES1;
					#endif
				#else
					ver = GLv_ES1;
				#endif
		#endif
	#endif

	switch ( ver ) {
		case GLv_ES2:
		{
			#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			ms_singleton = eeNew( RendererGLES2, () );
			break;
			#endif
		}
		case GLv_3:
		{
			#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			ms_singleton = eeNew( RendererGL3, () );
			break;
			#endif
		}
		case GLv_3CP:
		{
			#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			ms_singleton = eeNew( RendererGL3CP, () );
			break;
			#endif
		}
		case GLv_2:
		case GLv_ES1:
		case GLv_default:
		default:
		{
			#ifndef EE_GLES2
			ms_singleton = eeNew( RendererGL, () );
			#endif
		}
	}

	return ms_singleton;
}

cGL * cGL::createSingleton() {
	if ( ms_singleton == 0 ) {
		#if defined( EE_GLES_BOTH )
			ms_singleton = eeNew( RendererGL, () );
		#elif defined( EE_GLES2 )
			ms_singleton = eeNew( RendererGLES2, () );
		#elif defined( EE_GLES1 )
			ms_singleton = eeNew( RendererGL, () );
		#else
			ms_singleton = eeNew( RendererGL, () );
		#endif
	}

	return ms_singleton;
}

cGL * cGL::existsSingleton() {
	return ms_singleton;
}

cGL * cGL::instance() {
	return createSingleton();
}

void cGL::destroySingleton() {
	if( ms_singleton != 0 ) {
		eeDelete( ms_singleton );
		ms_singleton = 0;
	}
}

cGL::cGL() :
	mExtensions(0),
	mStateFlags( 1 << GLSF_LINE_SMOOTH ),
	mPushClip( true ),
	mQuadsSupported( true ),
	mBlendEnabled( false ),
	mQuadVertexs( 4 ),
	mLineWidth( 1 ),
	mCurVAO( 0 )
{
	GLi = this;
}

cGL::~cGL() {
	GLi = NULL;
}

RendererGL * cGL::getRendererGL() {
	return reinterpret_cast<RendererGL*>( this );
}

RendererGL3 * cGL::getRendererGL3() {
	return reinterpret_cast<RendererGL3*>( this );
}

RendererGL3CP * cGL::getRendererGL3CP() {
	return reinterpret_cast<RendererGL3CP*>( this );
}

RendererGLES2 * cGL::getRendererGLES2() {
	return reinterpret_cast<RendererGLES2*>( this );
}

void cGL::writeExtension( Uint8 Pos, Uint32 BitWrite ) {
	BitOp::writeBitKey( &mExtensions, Pos, BitWrite );
}

void cGL::init() {
	#ifdef EE_GLEW_AVAILABLE
	glewExperimental = 1;

	bool glewOn = ( GLEW_OK == glewInit() );

	if ( glewOn ) {
		writeExtension( EEGL_ARB_texture_non_power_of_two	, GLEW_ARB_texture_non_power_of_two 				);
		writeExtension( EEGL_ARB_point_parameters			, GLEW_ARB_point_parameters 						);
		writeExtension( EEGL_ARB_point_sprite				, GLEW_ARB_point_sprite 							);
		writeExtension( EEGL_ARB_shading_language_100		, GLEW_ARB_shading_language_100 					);
		writeExtension( EEGL_ARB_shader_objects				, GLEW_ARB_shader_objects							);
		writeExtension( EEGL_ARB_vertex_shader				, GLEW_ARB_vertex_shader 							);
		writeExtension( EEGL_ARB_fragment_shader			, GLEW_ARB_fragment_shader 							);
		writeExtension( EEGL_EXT_framebuffer_object			, GLEW_EXT_framebuffer_object 						);
		writeExtension( EEGL_ARB_multitexture				, GLEW_ARB_multitexture 							);
		writeExtension( EEGL_EXT_texture_compression_s3tc	, GLEW_EXT_texture_compression_s3tc 				);
		writeExtension( EEGL_ARB_vertex_buffer_object		, GLEW_ARB_vertex_buffer_object						);
		writeExtension( EEGL_ARB_pixel_buffer_object		, GLEW_ARB_pixel_buffer_object						);
		writeExtension( EEGL_ARB_vertex_array_object		, GLEW_ARB_vertex_array_object 						);
		writeExtension( EEGL_EXT_blend_func_separate		, GLEW_EXT_blend_func_separate						);
	}
	else
	#endif
	{
		writeExtension( EEGL_ARB_texture_non_power_of_two	, isExtension( "GL_ARB_texture_non_power_of_two" )	);
		writeExtension( EEGL_ARB_point_parameters			, isExtension( "GL_ARB_point_parameters" )			);
		writeExtension( EEGL_ARB_point_sprite				, isExtension( "GL_ARB_point_sprite" )				);
		writeExtension( EEGL_ARB_shading_language_100		, isExtension( "GL_ARB_shading_language_100" )		);
		writeExtension( EEGL_ARB_shader_objects				, isExtension( "GL_ARB_shader_objects" )			);
		writeExtension( EEGL_ARB_vertex_shader				, isExtension( "GL_ARB_vertex_shader" ) 			);
		writeExtension( EEGL_ARB_fragment_shader			, isExtension( "GL_ARB_fragment_shader" ) 			);
		writeExtension( EEGL_EXT_framebuffer_object			, isExtension( "GL_EXT_framebuffer_object" ) 		);
		writeExtension( EEGL_ARB_multitexture				, isExtension( "GL_ARB_multitexture" )				);
		writeExtension( EEGL_EXT_texture_compression_s3tc	, isExtension( "GL_EXT_texture_compression_s3tc" )	);
		writeExtension( EEGL_ARB_vertex_buffer_object		, isExtension( "GL_ARB_vertex_buffer_object" )		);
		writeExtension( EEGL_ARB_pixel_buffer_object		, isExtension( "GL_ARB_pixel_buffer_object" )		);
		writeExtension( EEGL_ARB_vertex_array_object		, isExtension( "GL_ARB_vertex_array_object" )		);
		writeExtension( EEGL_EXT_blend_func_separate		, isExtension( "GL_EXT_blend_func_separate" )		);
	}

	// NVIDIA added support for GL_OES_compressed_ETC1_RGB8_texture in desktop GPUs
	// GLEW doesn't return the correct result
	writeExtension( EEGL_OES_compressed_ETC1_RGB8_texture	, SOIL_GL_ExtensionSupported( "GL_OES_compressed_ETC1_RGB8_texture" )	);

	#ifdef EE_GLES

	WriteExtension( EEGL_ARB_point_parameters				, 1													);
	WriteExtension( EEGL_ARB_point_sprite					, 1													);
	WriteExtension( EEGL_ARB_multitexture					, 1													);

	WriteExtension( EEGL_IMG_texture_compression_pvrtc		, IsExtension( "GL_IMG_texture_compression_pvrtc" )	);

	if ( !IsExtension( EEGL_EXT_texture_compression_s3tc ) ) {
		WriteExtension(	EEGL_EXT_texture_compression_s3tc	, IsExtension( "GL_OES_texture_compression_S3TC" )	);
	}

	if ( !IsExtension( EEGL_EXT_framebuffer_object ) ) {
		WriteExtension(	EEGL_EXT_framebuffer_object			, IsExtension( "GL_OES_framebuffer_object" )		);
	}

	if ( !IsExtension( EEGL_ARB_texture_non_power_of_two ) ) {
		WriteExtension( EEGL_ARB_texture_non_power_of_two	, IsExtension( "GL_IMG_texture_npot" )	||
															  IsExtension( "GL_OES_texture_npot" )	||
															  IsExtension( "GL_APPLE_texture_2D_limited_npot" )	);
	}

	if ( !IsExtension( EEGL_ARB_vertex_array_object ) ) {
		WriteExtension( EEGL_ARB_vertex_array_object		, IsExtension( "GL_OES_vertex_array_object"	)		);
	}

	#endif

	#ifdef EE_GLES2
	WriteExtension( EEGL_EXT_framebuffer_object				, 1													);
	WriteExtension( EEGL_ARB_vertex_buffer_object			, 1													);
	WriteExtension( EEGL_ARB_shader_objects					, 1													);
	WriteExtension( EEGL_ARB_vertex_shader					, 1													);
	WriteExtension( EEGL_ARB_fragment_shader				, 1													);
	#endif

	#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	if ( !IsExtension( EEGL_EXT_texture_compression_s3tc ) ) {
		WriteExtension(	EEGL_EXT_texture_compression_s3tc	,	IsExtension( "WEBGL_compressed_texture_s3tc" ) ||
																IsExtension( "WEBKIT_WEBGL_compressed_texture_s3tc" ) ||
																IsExtension( "MOZ_WEBGL_compressed_texture_s3tc" ) );
	}
	#endif
}

bool cGL::isExtension( const std::string& name ) {
#ifdef EE_GLEW_AVAILABLE
	return 0 != glewIsSupported( name.c_str() );
#else
	return 0 != SOIL_GL_ExtensionSupported( name.c_str() );
#endif
}

bool cGL::isExtension( EEGL_extensions name ) {
	return 0 != ( mExtensions & ( 1 << name ) );
}

bool cGL::pointSpriteSupported() {
#ifdef EE_GLES
	return true;
#else
	return isExtension( EEGL_ARB_point_sprite );
#endif
}

bool cGL::shadersSupported() {
#ifdef EE_GLES
	return ( GLv_ES2 == Version() || GLv_3 == Version() || GLv_3CP == Version() );
#else
	return GLv_3CP == version() || ( isExtension( EEGL_ARB_shader_objects ) && isExtension( EEGL_ARB_vertex_shader ) && isExtension( EEGL_ARB_fragment_shader ) );
#endif
}

Uint32 cGL::getTextureParamEnum( const EE_TEXTURE_PARAM& Type ) {
	#ifndef EE_GLES
	switch( Type ) {
		case TEX_PARAM_COLOR_FUNC:			return GL_COMBINE_RGB_ARB;
		case TEX_PARAM_ALPHA_FUNC:			return GL_COMBINE_ALPHA_ARB;
		case TEX_PARAM_COLOR_SOURCE_0:		return GL_SOURCE0_RGB_ARB;
		case TEX_PARAM_COLOR_SOURCE_1:		return GL_SOURCE1_RGB_ARB;
		case TEX_PARAM_COLOR_SOURCE_2:		return GL_SOURCE2_RGB_ARB;
		case TEX_PARAM_ALPHA_SOURCE_0:		return GL_SOURCE0_ALPHA_ARB;
		case TEX_PARAM_ALPHA_SOURCE_1:		return GL_SOURCE1_ALPHA_ARB;
		case TEX_PARAM_ALPHA_SOURCE_2:		return GL_SOURCE2_ALPHA_ARB;
		case TEX_PARAM_COLOR_OP_0:			return GL_OPERAND0_RGB_ARB;
		case TEX_PARAM_COLOR_OP_1:			return GL_OPERAND1_RGB_ARB;
		case TEX_PARAM_COLOR_OP_2:			return GL_OPERAND2_RGB_ARB;
		case TEX_PARAM_ALPHA_OP_0:			return GL_OPERAND0_ALPHA_ARB;
		case TEX_PARAM_ALPHA_OP_1:			return GL_OPERAND1_ALPHA_ARB;
		case TEX_PARAM_ALPHA_OP_2:			return GL_OPERAND2_ALPHA_ARB;
		case TEX_PARAM_COLOR_SCALE:			return GL_RGB_SCALE_ARB;
		case TEX_PARAM_ALPHA_SCALE:			return GL_ALPHA_SCALE;
	}
	#endif

	return 0;
}

Uint32 cGL::getTextureFuncEnum( const EE_TEXTURE_FUNC& Type ) {
	#ifndef EE_GLES2
	switch( Type ) {
		case TEX_FUNC_MODULATE:			return GL_MODULATE;
		case TEX_FUNC_REPLACE:			return GL_REPLACE;
		case TEX_FUNC_ADD:				return GL_ADD;
		case TEX_FUNC_SUBSTRACT:		return GL_SUBTRACT_ARB;
		case TEX_FUNC_ADD_SIGNED:		return GL_ADD_SIGNED_ARB;
		case TEX_FUNC_INTERPOLATE:		return GL_INTERPOLATE_ARB;
		case TEX_FUNC_DOT3_RGB:			return GL_DOT3_RGB_ARB;
		case TEX_FUNC_DOT3_RGBA:		return GL_DOT3_RGBA_ARB;
	}
	#endif
	return 0;
}

Uint32 cGL::getTextureSourceEnum( const EE_TEXTURE_SOURCE& Type ) {
	#ifndef EE_GLES2
	switch ( Type ) {
		case TEX_SRC_TEXTURE:	return GL_TEXTURE;
		case TEX_SRC_CONSTANT:	return GL_CONSTANT_ARB;
		case TEX_SRC_PRIMARY:	return GL_PRIMARY_COLOR_ARB;
		case TEX_SRC_PREVIOUS:	return GL_PREVIOUS_ARB;
	}
	#endif

	return 0;
}

Uint32 cGL::getTextureOpEnum( const EE_TEXTURE_OP& Type ) {
	#ifndef EE_GLES2
	switch ( Type ) {
		case TEX_OP_COLOR:				return GL_SRC_COLOR;
		case TEX_OP_ONE_MINUS_COLOR:	return GL_ONE_MINUS_SRC_COLOR;
		case TEX_OP_ALPHA:				return GL_SRC_ALPHA;
		case TEX_OP_ONE_MINUS_ALPHA:	return GL_ONE_MINUS_SRC_ALPHA;
	}
	#endif

	return 0;
}

std::string cGL::getExtensions() {
	std::string exts;

	#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
	if ( GLv_3 == version() || GLv_3CP == version() ) {
		static pglGetStringiFunc eeglGetStringiFunc = NULL;

		int num_exts = 0;
		int i;

		if ( NULL == eeglGetStringiFunc ) {
			eeglGetStringiFunc = (pglGetStringiFunc)SOIL_GL_GetProcAddress("glGetStringi");

			if ( NULL == eeglGetStringiFunc ) {
				return 0;
			}
		}

		#ifndef GL_NUM_EXTENSIONS
		#define GL_NUM_EXTENSIONS 0x821D
		#endif
		glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts);
		for (i = 0; i < num_exts; i++)
		{
			const char *thisext = (const char *) eeglGetStringiFunc(GL_EXTENSIONS, i);

			exts += std::string( thisext ) + " ";
		}

		return exts;
	}
	#endif

	const char * extsc = (const char*)glGetString( GL_EXTENSIONS );

	if ( NULL != extsc ) {
		exts = std::string( extsc );
	}

	return exts;
}

void cGL::viewport( int x, int y, int width, int height ) {
	glViewport( x, y, width, height );
}

void cGL::disable ( unsigned int cap ) {
	switch ( cap )
	{
		case GL_BLEND:
		{
			if ( !mBlendEnabled ) {
				return;
			}

			mBlendEnabled = false;

			break;
		}
	}

	glDisable( cap );
}

void cGL::enable( unsigned int cap ) {
	switch ( cap )
	{
		case GL_BLEND:
		{
			if ( mBlendEnabled ) {
				return;
			}

			mBlendEnabled = true;

			break;
		}
	}

	glEnable( cap );
}

const char * cGL::getString( unsigned int name ) {
	return (const char*)glGetString( name );
}

void cGL::clear ( unsigned int mask ) {
	glClear( mask );
}

void cGL::clearColor( float red, float green, float blue, float alpha ) {
	glClearColor( red, green, blue, alpha );
}

void cGL::scissor ( int x, int y, int width, int height ) {
	glScissor( x, y, width, height );
}

void cGL::polygonMode( unsigned int face, unsigned int mode ) {
	#ifndef EE_GLES
	glPolygonMode( face, mode );
	#endif
}

void cGL::drawArrays (unsigned int mode, int first, int count) {
	glDrawArrays( mode, first, count );
}

void cGL::drawElements( unsigned int mode, int count, unsigned int type, const void *indices ) {
	glDrawElements( mode, count, type, indices );
}

void cGL::bindTexture ( unsigned int target, unsigned int texture ) {
	if ( GLv_3CP == version() && 0 == texture ) return;
	glBindTexture( target, texture );
}

void cGL::activeTexture( unsigned int texture ) {
	glActiveTexture( texture );
}

void cGL::blendFunc ( unsigned int sfactor, unsigned int dfactor ) {
	glBlendFunc( sfactor, dfactor );
}

void cGL::setShader( ShaderProgram * Shader ) {
	#ifdef EE_SHADERS_SUPPORTED
	if ( NULL != Shader ) {
		glUseProgram( Shader->getHandler() );
	} else {
		glUseProgram( 0 );
	}
	#endif
}

bool cGL::isLineSmooth() {
	return BitOp::readBitKey( &mStateFlags, GLSF_LINE_SMOOTH );
}

void cGL::lineSmooth() {
	lineSmooth( isLineSmooth() );
}

void cGL::lineSmooth( const bool& Enable ) {
	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( Enable ) {
		GLi->enable( GL_LINE_SMOOTH );
	} else {
		GLi->disable( GL_LINE_SMOOTH );
	}

	BitOp::writeBitKey( &mStateFlags, GLSF_LINE_SMOOTH, Enable ? 1 : 0 );
	#endif
}

void cGL::lineWidth(float width) {
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

void cGL::polygonMode() {
	EE_FILL_MODE Mode = DRAW_FILL;

	if ( BitOp::readBitKey( &mStateFlags, GLSF_POLYGON_MODE ) )
		Mode = DRAW_LINE;

	polygonMode( Mode );
}

void cGL::pixelStorei(unsigned int pname, int param) {
	glPixelStorei( pname, param );
}

void cGL::polygonMode( const EE_FILL_MODE& Mode ) {
	if ( Mode == DRAW_FILL )
		polygonMode( GL_FRONT_AND_BACK, GL_FILL );
	else
		polygonMode( GL_FRONT_AND_BACK, GL_LINE );

	BitOp::writeBitKey( &mStateFlags, GLSF_POLYGON_MODE, Mode == DRAW_LINE ? 1 : 0 );
}

std::string cGL::getVendor() {
	const char * str = getString( GL_VENDOR );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string cGL::getRenderer() {
	const char * str = getString( GL_RENDERER );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string cGL::getVersion() {
	const char * str = getString( GL_VERSION );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string cGL::getShadingLanguageVersion() {
	if ( shadersSupported() ) {
		#ifdef GL_SHADING_LANGUAGE_VERSION
			const char * str = getString( GL_SHADING_LANGUAGE_VERSION );

			if ( NULL != str )
				return std::string( str );
		#endif
	}

	return std::string( "Shaders not supported" );
}

void cGL::getViewport( int * viewport ) {
	glGetIntegerv( GL_VIEWPORT, viewport );
}

Vector3f cGL::projectCurrent( const Vector3f& point ) {
	float projMat[16];
	getCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	float modelMat[16];
	getCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	int viewPort[4];
	getViewport( viewPort );

	Vector3f fPoint( point );
	fPoint.y = viewPort[3] - point.y;

	Vector3<float> tv3;

	project( (float)fPoint.x, (float)fPoint.y, (float)fPoint.z, projMat, modelMat, viewPort, &tv3.x, &tv3.y, &tv3.z );

	return Vector3f( tv3.x, tv3.y, tv3.z );
}

Vector3f cGL::unProjectCurrent( const Vector3f& point ) {
	float projMat[16];
	getCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	float modelMat[16];
	getCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	int viewPort[4];
	getViewport( viewPort );

	Vector3f fPoint( point );
	fPoint.y = viewPort[3] - point.y;

	Vector3<float> tv3;

	unProject( (float)fPoint.x, (float)fPoint.y, (float)fPoint.z, projMat, modelMat, viewPort, &tv3.x, &tv3.y, &tv3.z );

	return Vector3f( tv3.x, tv3.y, tv3.z );
}

void cGL::stencilFunc( unsigned int func, int ref, unsigned int mask ) {
	glStencilFunc( func, ref, mask );
}

void cGL::stencilOp( unsigned int fail, unsigned int zfail, unsigned int zpass ) {
	glStencilOp( fail, zfail, zpass );
}

void cGL::stencilMask ( unsigned int mask ) {
	glStencilMask( mask );
}

void cGL::colorMask ( Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha ) {
	glColorMask( red, green, blue, alpha );
}

const int& cGL::quadVertexs() const {
	return mQuadVertexs;
}

void cGL::bindVertexArray ( unsigned int array ) {
#if !defined( EE_GLES )
	if ( mCurVAO != array ) {
		glBindVertexArray( array );

		mCurVAO = array;
	}
#endif
}

void cGL::deleteVertexArrays ( int n, const unsigned int *arrays ) {
#if !defined( EE_GLES )
	glDeleteVertexArrays( n, arrays );
#endif
}

void cGL::genVertexArrays ( int n, unsigned int *arrays ) {
#if !defined( EE_GLES )
	glGenVertexArrays( n, arrays );
#endif
}

const bool& cGL::quadsSupported() const {
	return mQuadsSupported;
}

}}
