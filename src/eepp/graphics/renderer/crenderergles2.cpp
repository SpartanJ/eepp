#include <eepp/graphics/renderer/crenderergles2.hpp>

#ifdef EE_GL3_ENABLED

#include <eepp/graphics/renderer/rendererhelper.hpp>

namespace EE { namespace Graphics {

const char * EEGLES2_STATES_NAME[] = {
	"dgl_Vertex",
	"dgl_Normal",
	"dgl_FrontColor"
};

const char * EEGLES2_TEXTUREUNIT_NAMES[] = {
	"dgl_MultiTexCoord0",
	"dgl_MultiTexCoord1",
	"dgl_MultiTexCoord2",
	"dgl_MultiTexCoord3"
};

const char * EEGLES2_PLANES_ENABLED_NAME[] = {
	"dgl_ClipEnabled[0]",
	"dgl_ClipEnabled[1]",
	"dgl_ClipEnabled[2]",
	"dgl_ClipEnabled[3]",
	"dgl_ClipEnabled[4]",
	"dgl_ClipEnabled[5]"
};

const char * EEGLES2_PLANES_NAMENABLED_NAME[] = {
	"dgl_ClipPlane[0]",
	"dgl_ClipPlane[1]",
	"dgl_ClipPlane[2]",
	"dgl_ClipPlane[3]",
	"dgl_ClipPlane[4]",
	"dgl_ClipPlane[5]"
};

const GLchar * EEGLES2_SHADER_BASE_VS =
#include "shaders/base.vert"

const GLchar * EEGLES2_SHADER_BASE_FS =
#include "shaders/base.frag"

const GLchar * EEGLES2_SHADER_CLIPPED_VS =
#include "shaders/clipped.vert"

const GLchar * EEGLES2_SHADER_CLIPPED_FS =
#include "shaders/clipped.frag"

const GLchar * EEGLES2_SHADER_POINTSPRITE_VS =
#include "shaders/pointsprite.vert"

const GLchar * EEGLES2_SHADER_POINTSPRITE_FS =
#include "shaders/pointsprite.frag"

const GLchar * EEGLES2_SHADER_PRIMITIVE_VS =
#include "shaders/primitive.vert"

const GLchar * EEGLES2_SHADER_PRIMITIVE_FS =
#include "shaders/primitive.frag"

cRendererGLES2::cRendererGLES2() :
	mStack( eeNew( cMatrixStack, () ) ),
	mProjectionMatrix_id(0),
	mModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL),
	mShaderPrev(NULL),
	mTexActive(1),
	mTexActiveLoc(-1),
	mClippingEnabledLoc(-1),
	mPointSize(1.f),
	mCurActiveTex( 0 ),
	mClippingEnabled( false ),
	mPointSpriteEnabled( false ),
	mLoaded( false ),
	mCurShaderLocal( true )
{
	mStack->mProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	mStack->mModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
}

cRendererGLES2::~cRendererGLES2() {
	eeSAFE_DELETE( mStack );
}

EEGL_version cRendererGLES2::Version() {
	return GLv_ES2;
}

std::string cRendererGLES2::VersionStr() {
	return "OpenGL ES 2";
}

void cRendererGLES2::Init() {
	if ( !mLoaded ) {
		Uint32 i;

		cGL::Init();

		std::string vs( EEGLES2_SHADER_BASE_VS );
		std::string fs( EEGLES2_SHADER_BASE_FS );

		mBaseVertexShader = vs;

		for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
			mAttribsLoc[ i ]		= -1;
			mAttribsLocStates[ i ]	= 0;
		}

		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			mPlanes[i]			= -1;
			mPlanesStates[i]	= 0;
		}

		for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			mTextureUnits[i]		= -1;
			mTextureUnitsStates[i]	= 0;
		}

		cShader::Ensure( false );

		mShaders[ EEGLES2_SHADER_BASE ]			= cShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_BASE ]->SetReloadCb( cb::Make1( this, &cRendererGLES2::ReloadShader ) );

		vs = EEGLES2_SHADER_CLIPPED_VS;
		fs = EEGLES2_SHADER_CLIPPED_FS;

		mShaders[ EEGLES2_SHADER_CLIPPED ]			= cShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_CLIPPED ]->SetReloadCb( cb::Make1( this, &cRendererGLES2::ReloadShader ) );

		vs = EEGLES2_SHADER_POINTSPRITE_VS;
		fs = EEGLES2_SHADER_POINTSPRITE_FS;

		mShaders[ EEGLES2_SHADER_POINTSPRITE ]		= cShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_POINTSPRITE ]->SetReloadCb( cb::Make1( this, &cRendererGLES2::ReloadShader ) );

		vs = EEGLES2_SHADER_PRIMITIVE_VS;
		fs = EEGLES2_SHADER_PRIMITIVE_FS;

		mShaders[ EEGLES2_SHADER_PRIMITIVE ]		= cShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_PRIMITIVE ]->SetReloadCb( cb::Make1( this, &cRendererGLES2::ReloadShader ) );

		cShader::Ensure( true );

		SetShader( EEGLES2_SHADER_BASE );
	} else {
		mCurShader = NULL;

		mShaders[ EEGLES2_SHADER_BASE ]->Reload();

		SetShader( EEGLES2_SHADER_BASE );
	}

	ClientActiveTexture( GL_TEXTURE0 );

	mLoaded = true;
}

GLuint cRendererGLES2::BaseShaderId() {
	return mCurShader->Handler();
}

void cRendererGLES2::ReloadCurrentShader() {
	ReloadShader( mCurShader );
}

void cRendererGLES2::ReloadShader( cShaderProgram * Shader ) {
	mCurShader = NULL;

	SetShader( Shader );
}

void cRendererGLES2::SetShader( const EEGLES2_SHADERS& Shader ) {
	SetShader( mShaders[ Shader ] );
}

void cRendererGLES2::CheckLocalShader() {
	for ( Uint32 i = 0; i < EEGLES2_SHADERS_COUNT; i++ ) {
		if ( mShaders[i] == mCurShader ) {
			mCurShaderLocal = true;
			return;
		}
	}

	mCurShaderLocal = false;
}

void cRendererGLES2::SetShader( cShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGLES2_SHADER_BASE ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	DisableClientState( GL_VERTEX_ARRAY );
	DisableClientState( GL_TEXTURE_COORD_ARRAY );
	DisableClientState( GL_COLOR_ARRAY );

	mShaderPrev				= mCurShader;
	mCurShader				= Shader;

	CheckLocalShader();

	mProjectionMatrix_id	= mCurShader->UniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->UniformLocation( "dgl_ModelViewMatrix" );
	mTexActiveLoc			= mCurShader->UniformLocation( "dgl_TexActive" );
	mClippingEnabledLoc		= mCurShader->UniformLocation( "dgl_ClippingEnabled" );
	mCurActiveTex			= 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mAttribsLoc[ i ] = mCurShader->AttributeLocation( EEGLES2_STATES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mClippingEnabledLoc ) {
			mPlanes[ i ] = mCurShader->UniformLocation( EEGLES2_PLANES_NAMENABLED_NAME[ i ] );
		} else {
			mPlanes[ i ] = -1;
		}
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[ i ] = mCurShader->AttributeLocation( EEGLES2_TEXTUREUNIT_NAMES[ i ] );
	}

	glUseProgram( mCurShader->Handler() );

	if ( -1 != mAttribsLoc[ EEGL_VERTEX_ARRAY ] )
		EnableClientState( GL_VERTEX_ARRAY );

	if ( -1 != mAttribsLoc[ EEGL_COLOR_ARRAY ] )
		EnableClientState( GL_COLOR_ARRAY );

	if ( -1 != mTextureUnits[ mCurActiveTex ] )
		EnableClientState( GL_TEXTURE_COORD_ARRAY );

	GLenum CM = mCurrentMode;

	MatrixMode( GL_PROJECTION );
	UpdateMatrix();
	MatrixMode( GL_MODELVIEW );
	UpdateMatrix();
	MatrixMode( CM );

	if ( -1 != mTexActiveLoc ) {
		mCurShader->SetUniform( mTexActiveLoc, mTexActive );
	}

	if ( -1 != mClippingEnabledLoc ) {
		mCurShader->SetUniform( mClippingEnabledLoc, 0 );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mPlanes[ i ] ) {
			mCurShader->SetUniform( EEGLES2_PLANES_ENABLED_NAME[ i ], 0 );
		}
	}
}

void cRendererGLES2::Enable( GLenum cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 0 == mTexActive ) {
				mTexActive = 1;

				SetShader( EEGLES2_SHADER_BASE );
			}

			return;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
		{
			GLint plane = cap - GL_CLIP_PLANE0;

			SetShader( EEGLES2_SHADER_CLIPPED );

			mPlanesStates[ plane ] = 1;

			PlaneStateCheck( true );

			mCurShader->SetUniform( EEGLES2_PLANES_ENABLED_NAME[ plane ], 1 );

			return;
		}
		case GL_POINT_SPRITE:
		{
			mPointSpriteEnabled = 1;

			cGL::Enable( GL_VERTEX_PROGRAM_POINT_SIZE );

			SetShader( EEGLES2_SHADER_POINTSPRITE );

			break;
		}
	}

	cGL::Enable( cap );
}

void cRendererGLES2::Disable ( GLenum cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 1 == mTexActive ) {
				mTexActive = 0;

				SetShader( EEGLES2_SHADER_PRIMITIVE );
			}

			return;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
		{
			GLint plane = cap - GL_CLIP_PLANE0;

			if ( mTexActive ) {
				SetShader( EEGLES2_SHADER_BASE );
			} else {
				SetShader( EEGLES2_SHADER_PRIMITIVE );
			}

			mPlanesStates[ plane ] = 0;

			PlaneStateCheck( false );

			return;
		}
		case GL_POINT_SPRITE:
		{
			mPointSpriteEnabled = 0;

			cGL::Disable( GL_VERTEX_PROGRAM_POINT_SIZE );

			SetShader( EEGLES2_SHADER_BASE );

			break;
		}
	}

	cGL::Disable( cap );
}

void cRendererGLES2::EnableClientState( GLenum array ) {
	GLint state;

	if ( GL_TEXTURE_COORD_ARRAY == array ) {
		if ( -1 != ( state = mTextureUnits[ mCurActiveTex ] ) ) {
			mTextureUnitsStates[ mCurActiveTex ] = 1;

			glEnableVertexAttribArray( state );
		}
	} else {
		Int32 Pos = array - GL_VERTEX_ARRAY;

		if ( -1 != ( state = mAttribsLoc[ Pos ] ) ) {
			mAttribsLocStates[ Pos ] = 1;

			glEnableVertexAttribArray( state );
		}
	}
}

void cRendererGLES2::DisableClientState( GLenum array ) {
	GLint state;

	if ( GL_TEXTURE_COORD_ARRAY == array ) {
		if ( -1 != ( state = mTextureUnits[ mCurActiveTex ] ) ) {
			mTextureUnitsStates[ mCurActiveTex ] = 0;

			glDisableVertexAttribArray( state );
		}
	} else {
		Int32 Pos = array - GL_VERTEX_ARRAY;

		if ( -1 != ( state = mAttribsLoc[ Pos ] ) ) {
			mAttribsLocStates[ Pos ] = 0;

			glDisableVertexAttribArray( state );
		}
	}
}

void cRendererGLES2::VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid * pointer ) {
	const GLint index = mAttribsLoc[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[ EEGL_VERTEX_ARRAY ] ) {
			mAttribsLocStates[ EEGL_VERTEX_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void cRendererGLES2::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	const GLint index = mAttribsLoc[ EEGL_COLOR_ARRAY ];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[ EEGL_COLOR_ARRAY ] ) {
			mAttribsLocStates[ EEGL_COLOR_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointerARB( index, size, type, GL_TRUE, stride, pointer );
		} else {
			glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
		}
	}
}

void cRendererGLES2::TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	if ( mCurShaderLocal ) {
		if ( 1 == mTexActive ) {
			if ( mCurShader == mShaders[ EEGLES2_SHADER_PRIMITIVE ] ) {
				if ( mClippingEnabled ) {
					SetShader( EEGLES2_SHADER_CLIPPED );
				} else if ( mPointSpriteEnabled ) {
					SetShader( EEGLES2_SHADER_POINTSPRITE );
				} else {
					SetShader( EEGLES2_SHADER_BASE );
				}
			}
		}
	}

	const GLint index = mTextureUnits[ mCurActiveTex ];

	if ( -1 != index ) {
		if ( 0 == mTextureUnitsStates[ mCurActiveTex ] ) {
			mTextureUnitsStates[ mCurActiveTex ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

GLint cRendererGLES2::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[ mCurActiveTex ];

	return mAttribsLoc[ State ];
}

void cRendererGLES2::PlaneStateCheck( bool tryEnable ) {
	GLint i;

	if (  tryEnable  ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[ i ] ) {
				mCurShader->SetUniform( mClippingEnabledLoc, 1 );

				mClippingEnabled = 1;

				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++) {
			if ( 0 != mPlanesStates[ i ] ) {
				return;
			}
		}

		mClippingEnabled = 0;
	}
}

void cRendererGLES2::UpdateMatrix() {
	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		{
			if ( -1 != mProjectionMatrix_id ) {
				mCurShader->SetUniformMatrix( mProjectionMatrix_id, &mStack->mProjectionMatrix.top()[0][0] );
			}

			break;
		}
		case GL_MODELVIEW:
		{
			if ( -1 != mModelViewMatrix_id ) {
				mCurShader->SetUniformMatrix( mModelViewMatrix_id, &mStack->mModelViewMatrix.top()[0][0] );
			}

			break;
		}
	}
}

void cRendererGLES2::PushMatrix() {
	mStack->mCurMatrix->push( mStack->mCurMatrix->top() );
	UpdateMatrix();
}

void cRendererGLES2::PopMatrix() {
	mStack->mCurMatrix->pop();
	UpdateMatrix();
}

void cRendererGLES2::LoadIdentity() {
	mStack->mCurMatrix->top() = glm::mat4(1.0);
	UpdateMatrix();
}

void cRendererGLES2::MultMatrixf ( const GLfloat * m ) {
	mStack->mCurMatrix->top() *= toGLMmat4( m );
	UpdateMatrix();
}

void cRendererGLES2::Translatef( GLfloat x, GLfloat y, GLfloat z ) {
	mStack->mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void cRendererGLES2::Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
	mStack->mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void cRendererGLES2::Scalef( GLfloat x, GLfloat y, GLfloat z ) {
	mStack->mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void cRendererGLES2::Ortho( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar ) {
	mStack->mCurMatrix->top() *= glm::ortho( left, right, bottom, top , zNear, zFar );
	UpdateMatrix();
}

void cRendererGLES2::LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ ) {
	mStack->mCurMatrix->top() *= glm::lookAt( glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ) );
	UpdateMatrix();
}

void cRendererGLES2::Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar ) {
	mStack->mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	UpdateMatrix();
}

void cRendererGLES2::LoadMatrixf( const GLfloat * m ) {
	mStack->mCurMatrix->top() = toGLMmat4( m );
	UpdateMatrix();
}

void cRendererGLES2::Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val ) {
	mStack->mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	UpdateMatrix();
}

void cRendererGLES2::GetCurrentMatrix( GLenum mode, GLfloat * m ) {
	switch ( mode ) {
		case GL_PROJECTION:
		case GL_PROJECTION_MATRIX:
		{
			fromGLMmat4( mStack->mProjectionMatrix.top(), m );
			break;
		}
		case GL_MODELVIEW:
		case GL_MODELVIEW_MATRIX:
		{
			fromGLMmat4( mStack->mModelViewMatrix.top(), m );
			break;
		}
	}
}

GLenum cRendererGLES2::GetCurrentMatrixMode() {
	return mCurrentMode;
}

void cRendererGLES2::MatrixMode(GLenum mode) {
	mCurrentMode = mode;

	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		case GL_PROJECTION_MATRIX:
		{
			mStack->mCurMatrix = &mStack->mProjectionMatrix;
			break;
		}
		case GL_MODELVIEW:
		case GL_MODELVIEW_MATRIX:
		{
			mStack->mCurMatrix = &mStack->mModelViewMatrix;
			break;
		}
	}
}

void cRendererGLES2::Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	GLfloat tX = (GLfloat)x;
	GLfloat tY = (GLfloat)y;
	GLfloat tW = (GLfloat)Width;
	GLfloat tH = (GLfloat)Height;

	glm::vec4 vclip_left	( 1.0	, 0.0	, 0.0	, -tX		);
	glm::vec4 vclip_right	( -1.0	, 0.0	, 0.0	, tX + tW	);
	glm::vec4 vclip_top		( 0.0	, 1.0	, 0.0	, -tY		);
	glm::vec4 vclip_bottom	( 0.0	, -1.0	, 0.0	, tY + tH	);

	glm::mat4 invMV = glm::inverse( mStack->mModelViewMatrix.top() );

	vclip_left		= vclip_left	* invMV;
	vclip_right		= vclip_right	* invMV;
	vclip_top		= vclip_top		* invMV;
	vclip_bottom	= vclip_bottom	* invMV;

	GLi->Enable(GL_CLIP_PLANE0);
	GLi->Enable(GL_CLIP_PLANE1);
	GLi->Enable(GL_CLIP_PLANE2);
	GLi->Enable(GL_CLIP_PLANE3);

	glUniform4fv( mPlanes[0], 1, static_cast<const GLfloat*>( &vclip_left[0]	)	);
	glUniform4fv( mPlanes[1], 1, static_cast<const GLfloat*>( &vclip_right[0]	)	);
	glUniform4fv( mPlanes[2], 1, static_cast<const GLfloat*>( &vclip_top[0]		)	);
	glUniform4fv( mPlanes[3], 1, static_cast<const GLfloat*>( &vclip_bottom[0]	)	);
}

void cRendererGLES2::Clip2DPlaneDisable() {
	GLi->Disable(GL_CLIP_PLANE0);
	GLi->Disable(GL_CLIP_PLANE1);
	GLi->Disable(GL_CLIP_PLANE2);
	GLi->Disable(GL_CLIP_PLANE3);
}

void cRendererGLES2::PointSize( GLfloat size ) {
	#ifndef EE_GLES2
	glPointSize( size );
	#endif

	mCurShader->SetUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void cRendererGLES2::ClipPlane( GLenum plane, const GLdouble * equation ) {
	Int32 nplane	= plane - GL_CLIP_PLANE0;
	Int32 location;

	if ( nplane < EE_MAX_PLANES ) {
		location = mPlanes[ nplane ];
	} else {
		std::string planeNum( "dgl_ClipPlane[" + String::ToStr( nplane ) + "]" );

		location = glGetUniformLocation( mCurShader->Handler(), (GLchar*)&planeNum[0] );
	}

	glm::vec4 teq( equation[0], equation[1], equation[2], equation[3] );

	teq = teq * glm::inverse( mStack->mModelViewMatrix.top() );		/// Apply the inverse of the model view matrix to the equation

	glUniform4f( location, (GLfloat)teq[0], (GLfloat)teq[1], (GLfloat)teq[2], (GLfloat)teq[3] );
}

GLfloat cRendererGLES2::PointSize() {
	return mPointSize;
}

void cRendererGLES2::ClientActiveTexture( GLenum texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

void cRendererGLES2::TexEnvi( GLenum target, GLenum pname, GLint param ) {
	//! @TODO: Implement TexEnvi
}

std::string cRendererGLES2::GetBaseVertexShader() {
	return mBaseVertexShader;
}

GLint cRendererGLES2::Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz ) {
	glm::vec3 tv3( glm::project( glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

GLint cRendererGLES2::UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz ) {
	glm::vec3 tv3( glm::unProject( glm::vec3( winx, winy, winz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != objx )
		*objx = tv3.x;

	if ( NULL != objy )
		*objy = tv3.y;

	if ( NULL != objz )
		*objz = tv3.z;

	return GL_TRUE;
}

}}

#endif
