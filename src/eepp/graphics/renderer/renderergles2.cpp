#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>

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

RendererGLES2::RendererGLES2() :
	mStack( eeNew( MatrixStack, () ) ),
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
	mQuadsSupported		= false;
	mQuadVertexs		= 6;

	mStack->mProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	mStack->mModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
}

RendererGLES2::~RendererGLES2() {
	eeSAFE_DELETE( mStack );
}

EEGL_version RendererGLES2::Version() {
	return GLv_ES2;
}

std::string RendererGLES2::VersionStr() {
	return "OpenGL ES 2";
}

void RendererGLES2::Init() {
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

		Shader::Ensure( false );

		mShaders[ EEGLES2_SHADER_BASE ]			= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_BASE ]->SetReloadCb( cb::Make1( this, &RendererGLES2::ReloadShader ) );

		vs = EEGLES2_SHADER_CLIPPED_VS;
		fs = EEGLES2_SHADER_CLIPPED_FS;

		mShaders[ EEGLES2_SHADER_CLIPPED ]			= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_CLIPPED ]->SetReloadCb( cb::Make1( this, &RendererGLES2::ReloadShader ) );

		vs = EEGLES2_SHADER_POINTSPRITE_VS;
		fs = EEGLES2_SHADER_POINTSPRITE_FS;

		mShaders[ EEGLES2_SHADER_POINTSPRITE ]		= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_POINTSPRITE ]->SetReloadCb( cb::Make1( this, &RendererGLES2::ReloadShader ) );

		vs = EEGLES2_SHADER_PRIMITIVE_VS;
		fs = EEGLES2_SHADER_PRIMITIVE_FS;

		mShaders[ EEGLES2_SHADER_PRIMITIVE ]		= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_PRIMITIVE ]->SetReloadCb( cb::Make1( this, &RendererGLES2::ReloadShader ) );

		Shader::Ensure( true );

		SetShader( EEGLES2_SHADER_BASE );
	} else {
		mCurShader = NULL;

		mShaders[ EEGLES2_SHADER_BASE ]->Reload();

		SetShader( EEGLES2_SHADER_BASE );
	}

	ClientActiveTexture( GL_TEXTURE0 );

	mLoaded = true;
}

unsigned int RendererGLES2::BaseShaderId() {
	return mCurShader->Handler();
}

void RendererGLES2::ReloadCurrentShader() {
	ReloadShader( mCurShader );
}

void RendererGLES2::ReloadShader( ShaderProgram * Shader ) {
	mCurShader = NULL;

	SetShader( Shader );
}

void RendererGLES2::SetShader( const EEGLES2_SHADERS& Shader ) {
	SetShader( mShaders[ Shader ] );
}

void RendererGLES2::CheckLocalShader() {
	for ( Uint32 i = 0; i < EEGLES2_SHADERS_COUNT; i++ ) {
		if ( mShaders[i] == mCurShader ) {
			mCurShaderLocal = true;
			return;
		}
	}

	mCurShaderLocal = false;
}

void RendererGLES2::SetShader( ShaderProgram * Shader ) {
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

	unsigned int CM = mCurrentMode;

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

void RendererGLES2::Enable( unsigned int cap ) {
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
			int plane = cap - GL_CLIP_PLANE0;

			SetShader( EEGLES2_SHADER_CLIPPED );

			mPlanesStates[ plane ] = 1;

			PlaneStateCheck( true );

			mCurShader->SetUniform( EEGLES2_PLANES_ENABLED_NAME[ plane ], 1 );

			return;
		}
		case GL_POINT_SPRITE:
		{
			mPointSpriteEnabled = 1;

			//cGL::Enable( GL_VERTEX_PROGRAM_POINT_SIZE );

			SetShader( EEGLES2_SHADER_POINTSPRITE );

			return;
		}
	}

	cGL::Enable( cap );
}

void RendererGLES2::Disable ( unsigned int cap ) {
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
			int plane = cap - GL_CLIP_PLANE0;

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

			//cGL::Disable( GL_VERTEX_PROGRAM_POINT_SIZE );

			SetShader( EEGLES2_SHADER_BASE );

			return;
		}
	}

	cGL::Disable( cap );
}

void RendererGLES2::EnableClientState( unsigned int array ) {
	int state;

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

void RendererGLES2::DisableClientState( unsigned int array ) {
	int state;

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

void RendererGLES2::VertexPointer ( int size, unsigned int type, int stride, const void * pointer, unsigned int allocate ) {
	const int index = mAttribsLoc[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[ EEGL_VERTEX_ARRAY ] ) {
			mAttribsLocStates[ EEGL_VERTEX_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void RendererGLES2::ColorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
	const int index = mAttribsLoc[ EEGL_COLOR_ARRAY ];

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

void RendererGLES2::TexCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
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

	const int index = mTextureUnits[ mCurActiveTex ];

	if ( -1 != index ) {
		if ( 0 == mTextureUnitsStates[ mCurActiveTex ] ) {
			mTextureUnitsStates[ mCurActiveTex ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

int RendererGLES2::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[ mCurActiveTex ];

	return mAttribsLoc[ State ];
}

void RendererGLES2::PlaneStateCheck( bool tryEnable ) {
	int i;

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

void RendererGLES2::UpdateMatrix() {
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

void RendererGLES2::PushMatrix() {
	mStack->mCurMatrix->push( mStack->mCurMatrix->top() );
	UpdateMatrix();
}

void RendererGLES2::PopMatrix() {
	mStack->mCurMatrix->pop();
	UpdateMatrix();
}

void RendererGLES2::LoadIdentity() {
	mStack->mCurMatrix->top() = glm::mat4(1.0);
	UpdateMatrix();
}

void RendererGLES2::MultMatrixf ( const float * m ) {
	mStack->mCurMatrix->top() *= toGLMmat4( m );
	UpdateMatrix();
}

void RendererGLES2::Translatef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void RendererGLES2::Rotatef( float angle, float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void RendererGLES2::Scalef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void RendererGLES2::Ortho( float left, float right, float bottom, float top, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::ortho( left, right, bottom, top , zNear, zFar );
	UpdateMatrix();
}

void RendererGLES2::LookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ ) {
	mStack->mCurMatrix->top() *= glm::lookAt( glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ) );
	UpdateMatrix();
}

void RendererGLES2::Perspective ( float fovy, float aspect, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	UpdateMatrix();
}

void RendererGLES2::LoadMatrixf( const float * m ) {
	mStack->mCurMatrix->top() = toGLMmat4( m );
	UpdateMatrix();
}

void RendererGLES2::Frustum( float left, float right, float bottom, float top, float near_val, float far_val ) {
	mStack->mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	UpdateMatrix();
}

void RendererGLES2::GetCurrentMatrix( unsigned int mode, float * m ) {
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

unsigned int RendererGLES2::GetCurrentMatrixMode() {
	return mCurrentMode;
}

void RendererGLES2::MatrixMode(unsigned int mode) {
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

void RendererGLES2::Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	float tX = (float)x;
	float tY = (float)y;
	float tW = (float)Width;
	float tH = (float)Height;

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

	glUniform4fv( mPlanes[0], 1, static_cast<const float*>( &vclip_left[0]	)	);
	glUniform4fv( mPlanes[1], 1, static_cast<const float*>( &vclip_right[0]	)	);
	glUniform4fv( mPlanes[2], 1, static_cast<const float*>( &vclip_top[0]		)	);
	glUniform4fv( mPlanes[3], 1, static_cast<const float*>( &vclip_bottom[0]	)	);

	if ( mPushClip ) {
		mPlanesClipped.push_back( Rectf( x, y, Width, Height ) );
	}
}

void RendererGLES2::Clip2DPlaneDisable() {
	if ( ! mPlanesClipped.empty() ) { // This should always be true
		mPlanesClipped.pop_back();
	}

	if ( mPlanesClipped.empty() ) {
		GLi->Disable(GL_CLIP_PLANE0);
		GLi->Disable(GL_CLIP_PLANE1);
		GLi->Disable(GL_CLIP_PLANE2);
		GLi->Disable(GL_CLIP_PLANE3);
	} else {
		Rectf R( mPlanesClipped.back() );
		mPushClip = false;
		Clip2DPlaneEnable( R.Left, R.Top, R.Right, R.Bottom );
		mPushClip = true;
	}
}

void RendererGLES2::PointSize( float size ) {
	#if !defined( EE_GLES2 ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glPointSize( size );
	#endif

	mCurShader->SetUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void RendererGLES2::ClipPlane( unsigned int plane, const double * equation ) {
	Int32 nplane	= plane - GL_CLIP_PLANE0;
	Int32 location;

	if ( nplane < EE_MAX_PLANES ) {
		location = mPlanes[ nplane ];
	} else {
		std::string planeNum( "dgl_ClipPlane[" + String::toStr( nplane ) + "]" );

		location = glGetUniformLocation( mCurShader->Handler(), (GLchar*)&planeNum[0] );
	}

	glm::vec4 teq( equation[0], equation[1], equation[2], equation[3] );

	teq = teq * glm::inverse( mStack->mModelViewMatrix.top() );		/// Apply the inverse of the model view matrix to the equation

	glUniform4f( location, (float)teq[0], (float)teq[1], (float)teq[2], (float)teq[3] );
}

float RendererGLES2::PointSize() {
	return mPointSize;
}

void RendererGLES2::ClientActiveTexture( unsigned int texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

void RendererGLES2::TexEnvi( unsigned int target, unsigned int pname, int param ) {
	//! @TODO: Implement TexEnvi
}

std::string RendererGLES2::GetBaseVertexShader() {
	return mBaseVertexShader;
}

int RendererGLES2::Project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz ) {
	glm::vec3 tv3( glm::project( glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

int RendererGLES2::UnProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz ) {
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
