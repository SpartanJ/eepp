#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/renderer/crenderergl.hpp>
#include <eepp/graphics/renderer/crenderergl3.hpp>
#include <eepp/graphics/renderer/crenderergles2.hpp>

namespace EE { namespace Graphics {

cGL * GLi = NULL;

cGL * cGL::ms_singleton = NULL;

cGL * cGL::CreateSingleton( EEGL_version ver ) {
	#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )
	if ( GLv_default == ver )
		ver = GLv_2;
	#else
	if ( GLv_default == ver )
		#ifdef EE_GLES1
		ver = GLv_ES1;
		#else
		ver = GLv_ES2;
		#endif
	#endif

	switch ( ver ) {
		case GLv_ES2:
		{
			#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			ms_singleton = eeNew( cRendererGLES2, () );
			break;
			#endif
		}
		case GLv_3:
		{
			#if defined( EE_GL3_ENABLED ) || defined( EE_GLES2 )
			ms_singleton = eeNew( cRendererGL3, () );
			break;
			#endif
		}
		case GLv_2:
		case GLv_ES1:
		case GLv_default:
		default:
		{
			#ifndef EE_GLES2
			ms_singleton = eeNew( cRendererGL, () );
			#endif
		}
	}

	return ms_singleton;
}

cGL * cGL::CreateSingleton() {
	if ( ms_singleton == 0 ) {
		#if defined( EE_GLES_BOTH )
			ms_singleton = eeNew( cRendererGL, () );
		#elif defined( EE_GLES2 )
			ms_singleton = eeNew( cRendererGLES2, () );
		#elif defined( EE_GLES1 )
			ms_singleton = eeNew( cRendererGL, () );
		#else
			ms_singleton = eeNew( cRendererGL, () );
		#endif
	}

	return ms_singleton;
}

cGL * cGL::ExistsSingleton() {
	return ms_singleton;
}

cGL * cGL::instance() {
	return CreateSingleton();
}

void cGL::DestroySingleton() {
	if( ms_singleton != 0 ) {
		eeDelete( ms_singleton );
		ms_singleton = 0;
	}
}

cGL::cGL() :
	mExtensions(0),
	mStateFlags( 1 << GLSF_LINE_SMOOTH ),
	mPushClip( true )
{
	GLi = this;
}

cGL::~cGL() {
	GLi = NULL;
}

cRendererGL * cGL::GetRendererGL() {
	return reinterpret_cast<cRendererGL*>( this );
}

cRendererGL3 * cGL::GetRendererGL3() {
	return reinterpret_cast<cRendererGL3*>( this );
}

cRendererGLES2 * cGL::GetRendererGLES2() {
	return reinterpret_cast<cRendererGLES2*>( this );
}

void cGL::WriteExtension( Uint8 Pos, Uint32 BitWrite ) {
	BitOp::WriteBitKey( &mExtensions, Pos, BitWrite );
}

void cGL::Init() {
	#ifdef EE_GLEW_AVAILABLE
	glewExperimental = 1;

	bool glewOn = ( GLEW_OK == glewInit() );

	if ( glewOn ) {
		WriteExtension( EEGL_ARB_texture_non_power_of_two	, GLEW_ARB_texture_non_power_of_two 				);
		WriteExtension( EEGL_ARB_point_parameters			, GLEW_ARB_point_parameters 						);
		WriteExtension( EEGL_ARB_point_sprite				, GLEW_ARB_point_sprite 							);
		WriteExtension( EEGL_ARB_shading_language_100		, GLEW_ARB_shading_language_100 					);
		WriteExtension( EEGL_ARB_shader_objects				, GLEW_ARB_shader_objects							);
		WriteExtension( EEGL_ARB_vertex_shader				, GLEW_ARB_vertex_shader 							);
		WriteExtension( EEGL_ARB_fragment_shader			, GLEW_ARB_fragment_shader 							);
		WriteExtension( EEGL_EXT_framebuffer_object			, GLEW_EXT_framebuffer_object 						);
		WriteExtension( EEGL_ARB_multitexture				, GLEW_ARB_multitexture 							);
		WriteExtension( EEGL_EXT_texture_compression_s3tc	, GLEW_EXT_texture_compression_s3tc 				);
		WriteExtension( EEGL_ARB_vertex_buffer_object		, GLEW_ARB_vertex_buffer_object						);
		WriteExtension( EEGL_ARB_pixel_buffer_object		, GLEW_ARB_pixel_buffer_object						);
		WriteExtension( EEGL_ARB_vertex_array_object		, GLEW_ARB_vertex_array_object 						);
		WriteExtension( EEGL_EXT_blend_func_separate		, GLEW_EXT_blend_func_separate						);
	}
	else
	#endif
	{
		WriteExtension( EEGL_ARB_texture_non_power_of_two	, IsExtension( "GL_ARB_texture_non_power_of_two" )	);
		WriteExtension( EEGL_ARB_point_parameters			, IsExtension( "GL_ARB_point_parameters" )			);
		WriteExtension( EEGL_ARB_point_sprite				, IsExtension( "GL_ARB_point_sprite" )				);
		WriteExtension( EEGL_ARB_shading_language_100		, IsExtension( "GL_ARB_shading_language_100" )		);
		WriteExtension( EEGL_ARB_shader_objects				, IsExtension( "GL_ARB_shader_objects" )			);
		WriteExtension( EEGL_ARB_vertex_shader				, IsExtension( "GL_ARB_vertex_shader" ) 			);
		WriteExtension( EEGL_ARB_fragment_shader			, IsExtension( "GL_ARB_fragment_shader" ) 			);
		WriteExtension( EEGL_EXT_framebuffer_object			, IsExtension( "GL_EXT_framebuffer_object" ) 		);
		WriteExtension( EEGL_ARB_multitexture				, IsExtension( "GL_ARB_multitexture" )				);
		WriteExtension( EEGL_EXT_texture_compression_s3tc	, IsExtension( "GL_EXT_texture_compression_s3tc" )	);
		WriteExtension( EEGL_ARB_vertex_buffer_object		, IsExtension( "GL_ARB_vertex_buffer_object" )		);
		WriteExtension( EEGL_ARB_pixel_buffer_object		, IsExtension( "GL_ARB_pixel_buffer_object" )		);
		WriteExtension( EEGL_ARB_vertex_array_object		, IsExtension( "GL_ARB_vertex_array_object" )		);
		WriteExtension( EEGL_EXT_blend_func_separate		, IsExtension( "GL_EXT_blend_func_separate" )		);
	}

	#ifdef EE_GLES

	WriteExtension( EEGL_ARB_point_parameters				, 1													);
	WriteExtension( EEGL_ARB_point_sprite					, 1													);
	WriteExtension( EEGL_ARB_multitexture					, 1													);

	WriteExtension( EEGL_IMG_texture_compression_pvrtc		, IsExtension( "GL_IMG_texture_compression_pvrtc" )	);
	WriteExtension( EEGL_OES_compressed_ETC1_RGB8_texture	, IsExtension( "GL_OES_compressed_ETC1_RGB8_texture" )	);

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
}

bool cGL::IsExtension( const std::string& name ) {
#ifdef EE_GLEW_AVAILABLE
	return 0 != glewIsSupported( name.c_str() );
#else
	char *Exts = (char *)glGetString( GL_EXTENSIONS );

	if ( NULL != Exts && strstr( Exts, name.c_str() ) ) {
		return true;
	}

	return false;
#endif
}

bool cGL::IsExtension( EEGL_extensions name ) {
	return 0 != ( mExtensions & ( 1 << name ) );
}

bool cGL::PointSpriteSupported() {
#ifdef EE_GLES
	return true;
#else
	return IsExtension( EEGL_ARB_point_parameters ) && IsExtension( EEGL_ARB_point_sprite );
#endif
}

bool cGL::ShadersSupported() {
#ifdef EE_GLES
	return ( GLv_ES2 == Version() || GLv_3 == Version() );
#else
	return IsExtension( EEGL_ARB_shading_language_100 ) && IsExtension( EEGL_ARB_shader_objects ) && IsExtension( EEGL_ARB_vertex_shader ) && IsExtension( EEGL_ARB_fragment_shader );
#endif
}

Uint32 cGL::GetTextureParamEnum( const EE_TEXTURE_PARAM& Type ) {
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

Uint32 cGL::GetTextureFuncEnum( const EE_TEXTURE_FUNC& Type ) {
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

Uint32 cGL::GetTextureSourceEnum( const EE_TEXTURE_SOURCE& Type ) {
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

Uint32 cGL::GetTextureOpEnum( const EE_TEXTURE_OP& Type ) {
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

std::string cGL::GetExtensions() {
	const char * extsc = (const char*)glGetString( GL_EXTENSIONS );
	std::string exts;

	if ( NULL != extsc ) {
		exts = std::string( extsc );
	}

	return exts;
}

void cGL::Viewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
	glViewport( x, y, width, height );
}

void cGL::Disable ( GLenum cap ) {
	glDisable( cap );
}

void cGL::Enable( GLenum cap ) {
	glEnable( cap );
}

const char * cGL::GetString( GLenum name ) {
	return (const char*)glGetString( name );
}

void cGL::Clear ( GLbitfield mask ) {
	glClear( mask );
}

void cGL::ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
	glClearColor( red, green, blue, alpha );
}

void cGL::Scissor ( GLint x, GLint y, GLsizei width, GLsizei height ) {
	glScissor( x, y, width, height );
}

void cGL::PolygonMode( GLenum face, GLenum mode ) {
	#ifndef EE_GLES
	glPolygonMode( face, mode );
	#endif
}

void cGL::DrawArrays (GLenum mode, GLint first, GLsizei count) {
	glDrawArrays( mode, first, count );
}

void cGL::DrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
	glDrawElements( mode, count, type, indices );
}

void cGL::BindTexture ( GLenum target, GLuint texture ) {
	glBindTexture( target, texture );
}

void cGL::ActiveTexture( GLenum texture ) {
	glActiveTexture( texture );
}

void cGL::BlendFunc ( GLenum sfactor, GLenum dfactor ) {
	glBlendFunc( sfactor, dfactor );
}

void cGL::SetShader( cShaderProgram * Shader ) {
	#ifdef EE_SHADERS_SUPPORTED
	if ( NULL != Shader ) {
		glUseProgram( Shader->Handler() );
	} else {
		glUseProgram( 0 );
	}
	#endif
}

bool cGL::IsLineSmooth() {
	return BitOp::ReadBitKey( &mStateFlags, GLSF_LINE_SMOOTH );
}

void cGL::LineSmooth() {
	LineSmooth( IsLineSmooth() );
}

void cGL::LineSmooth( const bool& Enable ) {
	if ( Enable ) {
		GLi->Enable( GL_LINE_SMOOTH );
	} else {
		GLi->Disable( GL_LINE_SMOOTH );
	}

	BitOp::WriteBitKey( &mStateFlags, GLSF_LINE_SMOOTH, Enable ? 1 : 0 );
}

void cGL::PolygonMode() {
	EE_FILL_MODE Mode = DRAW_FILL;

	if ( BitOp::ReadBitKey( &mStateFlags, GLSF_POLYGON_MODE ) )
		Mode = DRAW_LINE;

	PolygonMode( Mode );
}

void cGL::PolygonMode( const EE_FILL_MODE& Mode ) {
	if ( Mode == DRAW_FILL )
		PolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	else
		PolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	BitOp::WriteBitKey( &mStateFlags, GLSF_POLYGON_MODE, Mode == DRAW_LINE ? 1 : 0 );
}

std::string cGL::GetVendor() {
	const char * str = GetString( GL_VENDOR );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string cGL::GetRenderer() {
	const char * str = GetString( GL_RENDERER );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string cGL::GetVersion() {
	const char * str = GetString( GL_VERSION );

	if ( NULL != str )
		return std::string( str );

	return std::string();
}

std::string cGL::GetShadingLanguageVersion() {
	if ( ShadersSupported() ) {
		#ifdef GL_SHADING_LANGUAGE_VERSION
			const char * str = GetString( GL_SHADING_LANGUAGE_VERSION );

			if ( NULL != str )
				return std::string( str );
		#endif
	}

	return std::string( "Shaders not supported" );
}

void cGL::GetViewport( GLint * viewport ) {
	glGetIntegerv( GL_VIEWPORT, viewport );
}

eeVector3f cGL::ProjectCurrent( const eeVector3f& point ) {
	GLfloat projMat[16];
	GetCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	GLfloat modelMat[16];
	GetCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	GLint viewPort[4];
	GetViewport( viewPort );

	Vector3<GLfloat> tv3;

	Project( (GLfloat)point.x, (GLfloat)point.y, (GLfloat)point.z, projMat, modelMat, viewPort, &tv3.x, &tv3.y, &tv3.z );

	return eeVector3f( tv3.x, tv3.y, tv3.z );
}

eeVector3f cGL::UnProjectCurrent( const eeVector3f& point ) {
	GLfloat projMat[16];
	GetCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	GLfloat modelMat[16];
	GetCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	GLint viewPort[4];
	GetViewport( viewPort );

	Vector3<GLfloat> tv3;

	UnProject( (GLfloat)point.x, (GLfloat)point.y, (GLfloat)point.z, projMat, modelMat, viewPort, &tv3.x, &tv3.y, &tv3.z );

	return eeVector3f( tv3.x, tv3.y, tv3.z );
}

void cGL::StencilFunc( GLenum func, GLint ref, GLuint mask ) {
	glStencilFunc( func, ref, mask );
}

void cGL::StencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
	glStencilOp( fail, zfail, zpass );
}

void cGL::StencilMask ( GLuint mask ) {
	glStencilMask( mask );
}

void cGL::ColorMask ( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
	glColorMask( red, green, blue, alpha );
}

}}
