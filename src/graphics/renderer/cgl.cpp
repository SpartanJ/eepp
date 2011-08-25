#include "cgl.hpp"
#include "crenderergl.hpp"
#include "crenderergl3.hpp"

namespace EE { namespace Graphics {

cGL * GLi = NULL;

cGL * cGL::ms_singleton = NULL;

cGL * cGL::CreateSingleton( EEGL_version ver ) {
	#ifndef EE_GLES2
	if ( GLv_default == ver )
		ver = GLv_2;
	#else
	if ( GLv_default == ver )
		ver = GLv_3;
	#endif

	switch ( ver ) {
		case GLv_ES2:
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
		#ifdef EE_GLES2
			ms_singleton = eeNew( cRendererGL3, () );
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
	mStateFlags( 1 << GLSF_LINE_SMOOTH )
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

void cGL::WriteExtension( Uint8 Pos, Uint32 BitWrite ) {
	Write32BitKey( &mExtensions, Pos, BitWrite );
}

void cGL::Init() {
	bool glewOn = false;

	#ifdef EE_GLEW_AVAILABLE
	glewOn = ( GLEW_OK == glewInit() );

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
		WriteExtension( EEGL_ARB_vertex_array_object		, GLEW_ARB_vertex_array_object 						);
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
		WriteExtension( EEGL_ARB_vertex_array_object		, IsExtension( "GL_ARB_vertex_array_object" )		);

		glewOn = false; /// avoid compiler warning
	}
}

bool cGL::IsExtension( const std::string& name ) {
#ifdef EE_GLEW_AVAILABLE
	return 0 != glewIsSupported( name.c_str() );
#else
	char *Exts = (char *)glGetString( GL_EXTENSIONS );

	if ( strstr( Exts, name.c_str() ) ) {
		return true;
	}

	return false;
#endif
}

bool cGL::IsExtension( EEGL_extensions name ) {
	return 0 != ( mExtensions & ( 1 << name ) );
}

bool cGL::PointSpriteSupported() {
	return IsExtension( EEGL_ARB_point_parameters ) && IsExtension( EEGL_ARB_point_sprite );
}

bool cGL::ShadersSupported() {
	return IsExtension( EEGL_ARB_shading_language_100 ) && IsExtension( EEGL_ARB_shader_objects ) && IsExtension( EEGL_ARB_vertex_shader ) && IsExtension( EEGL_ARB_fragment_shader );
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

char * cGL::GetExtensions() {
	return (char *)glGetString( GL_EXTENSIONS );
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

char * cGL::GetString( GLenum name ) {
	return (char*)glGetString( name );
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
	#ifndef EE_GLES1
	if ( NULL != Shader ) {
		glUseProgram( Shader->Handler() );
	} else {
		glUseProgram( 0 );
	}
	#endif
}

bool cGL::IsLineSmooth() {
	return Read32BitKey( &mStateFlags, GLSF_LINE_SMOOTH );
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

	Write32BitKey( &mStateFlags, GLSF_LINE_SMOOTH, Enable ? 1 : 0 );
}

void cGL::PolygonMode() {
	EE_FILL_MODE Mode = EE_DRAW_FILL;

	if ( Read32BitKey( &mStateFlags, GLSF_POLYGON_MODE ) )
		Mode = EE_DRAW_LINE;

	PolygonMode( Mode );
}

void cGL::PolygonMode( const EE_FILL_MODE& Mode ) {
	if ( Mode == EE_DRAW_FILL )
		PolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	else
		PolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	Write32BitKey( &mStateFlags, GLSF_POLYGON_MODE, Mode == EE_DRAW_LINE ? 1 : 0 );
}

std::string cGL::GetVendor() {
	return std::string( reinterpret_cast<const char*> ( cGL::instance()->GetString( GL_VENDOR ) ) );
}

std::string cGL::GetRenderer() {
	return std::string( reinterpret_cast<const char*> ( cGL::instance()->GetString( GL_RENDERER ) ) );
}

std::string cGL::GetVersion() {
	return std::string( reinterpret_cast<const char*> ( cGL::instance()->GetString( GL_VERSION ) ) );
}

std::string cGL::GetShadingLanguageVersion() {
	if ( ShadersSupported() ) {
		#ifdef GL_SHADING_LANGUAGE_VERSION
			return std::string( reinterpret_cast<const char*> ( cGL::instance()->GetString( GL_SHADING_LANGUAGE_VERSION ) ) );
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

	eeVector3f tv3;

	Project( point.x, point.y, point.z, projMat, modelMat, viewPort, &tv3.x, &tv3.y, &tv3.z );

	return tv3;
}

eeVector3f cGL::UnProjectCurrent( const eeVector3f& point ) {
	GLfloat projMat[16];
	GetCurrentMatrix( GL_PROJECTION_MATRIX, projMat );

	GLfloat modelMat[16];
	GetCurrentMatrix( GL_MODELVIEW_MATRIX, modelMat );

	GLint viewPort[4];
	GetViewport( viewPort );

	eeVector3f tv3;

	UnProject( point.x, point.y, point.z, projMat, modelMat, viewPort, &tv3.x, &tv3.y, &tv3.z );

	return tv3;
}

}}
