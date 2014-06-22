#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>

#ifdef EE_GL3_ENABLED

#include <eepp/graphics/renderer/rendererhelper.hpp>

namespace EE { namespace Graphics {

const char * EEGL3_STATES_NAME[] = {
	"dgl_Vertex",
	"dgl_Normal",
	"dgl_FrontColor"
};

const char * EEGL3_TEXTUREUNIT_NAMES[] = {
	"dgl_MultiTexCoord0",
	"dgl_MultiTexCoord1",
	"dgl_MultiTexCoord2",
	"dgl_MultiTexCoord3"
};

const char * EEGL3_PLANES_ENABLED_NAME[] = {
	"dgl_ClipEnabled[0]",
	"dgl_ClipEnabled[1]",
	"dgl_ClipEnabled[2]",
	"dgl_ClipEnabled[3]",
	"dgl_ClipEnabled[4]",
	"dgl_ClipEnabled[5]"
};

const char * EEGL3_PLANES_NAME[] = {
	"dgl_ClipPlane[0]",
	"dgl_ClipPlane[1]",
	"dgl_ClipPlane[2]",
	"dgl_ClipPlane[3]",
	"dgl_ClipPlane[4]",
	"dgl_ClipPlane[5]"
};

const GLchar * EEGL3_SHADER_BASE_VS =
#include "shaders/basegl3.vert"

const GLchar * EEGL3_SHADER_BASE_FS =
#include "shaders/basegl3.frag"

RendererGL3::RendererGL3() :
	mProjectionMatrix_id(0),
	mModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL),
	mShaderPrev(NULL),
	mTexActive(1),
	mTexActiveLoc(-1),
	mPointSpriteLoc(-1),
	mClippingEnabledLoc(-1),
	mPointSize(1.f),
	mCurActiveTex( 0 ),
	mLoaded( false )
{
#if defined( EE_GLES2 ) || defined( EE_GLES_BOTH )
	mQuadsSupported		= false;
	mQuadVertexs		= 6;
#endif

	mStack = eeNew( cMatrixStack, () );
	mStack->mProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	mStack->mModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix

	cGL::Enable( GL_VERTEX_PROGRAM_POINT_SIZE );
}

RendererGL3::~RendererGL3() {
	eeSAFE_DELETE( mStack );
}

EEGL_version RendererGL3::Version() {
	return GLv_3;
}

std::string RendererGL3::VersionStr() {
	return "OpenGL 3";
}

void RendererGL3::Init() {
	if ( !mLoaded ) {
		Uint32 i;

		cGL::Init();

		std::string vs( EEGL3_SHADER_BASE_VS );
		std::string fs( EEGL3_SHADER_BASE_FS );

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

		mShaders[ EEGL3_SHADER_BASE ]			= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGL3_SHADER_BASE ]->SetReloadCb( cb::Make1( this, &RendererGL3::ReloadShader ) );

		Shader::Ensure( true );

		SetShader( EEGL3_SHADER_BASE );
	} else {
		mCurShader = NULL;

		mShaders[ EEGL3_SHADER_BASE ]->Reload();

		SetShader( EEGL3_SHADER_BASE );
	}

	ClientActiveTexture( GL_TEXTURE0 );

	mLoaded = true;
}

unsigned int RendererGL3::BaseShaderId() {
	return mCurShader->Handler();
}

void RendererGL3::ReloadCurrentShader() {
	ReloadShader( mCurShader );
}

void RendererGL3::ReloadShader( ShaderProgram * Shader ) {
	mCurShader = NULL;

	SetShader( Shader );
}

void RendererGL3::SetShader( const EEGL3_SHADERS& Shader ) {
	SetShader( mShaders[ Shader ] );
}

void RendererGL3::SetShader( ShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGL3_SHADER_BASE ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	DisableClientState( GL_VERTEX_ARRAY );
	DisableClientState( GL_TEXTURE_COORD_ARRAY );
	DisableClientState( GL_COLOR_ARRAY );

	mShaderPrev				= mCurShader;
	mCurShader				= Shader;
	mProjectionMatrix_id	= mCurShader->UniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->UniformLocation( "dgl_ModelViewMatrix" );
	mTexActiveLoc			= mCurShader->UniformLocation( "dgl_TexActive" );
	mPointSpriteLoc			= mCurShader->UniformLocation( "dgl_PointSpriteActive" );
	mClippingEnabledLoc		= mCurShader->UniformLocation( "dgl_ClippingEnabled" );
	mCurActiveTex			= 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mAttribsLoc[ i ] = mCurShader->AttributeLocation( EEGL3_STATES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		mPlanes[ i ] = mCurShader->UniformLocation( EEGL3_PLANES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[ i ] = mCurShader->AttributeLocation( EEGL3_TEXTUREUNIT_NAMES[ i ] );
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
		mCurShader->SetUniform( mTexActiveLoc, 1 );
	}

	mCurShader->SetUniform( mClippingEnabledLoc, 0 );

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mPlanes[ i ] ) {
			mCurShader->SetUniform( EEGL3_PLANES_ENABLED_NAME[ i ], 0 );
		}
	}

	if ( -1 != mPointSpriteLoc ) {
		mCurShader->SetUniform( mPointSpriteLoc, 0 );
	}
}

void RendererGL3::Enable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 0 == mTexActive ) {
				mTexActive = 1;

				mCurShader->SetUniform( mTexActiveLoc, mTexActive );
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

			if ( 0 == mPlanesStates[ plane ] ) {
				mPlanesStates[ plane ] = 1;

				PlaneStateCheck( true );

				mCurShader->SetUniform( EEGL3_PLANES_ENABLED_NAME[ plane ], 1 );
			}

			return;
		}
		case GL_POINT_SPRITE:
		{
			mCurShader->SetUniform( mPointSpriteLoc, 1 );

			break;
		}
	}

	cGL::Enable( cap );
}

void RendererGL3::Disable ( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 1 == mTexActive ) {
				mTexActive = 0;

				mCurShader->SetUniform( mTexActiveLoc, mTexActive );
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

			if ( 1 == mPlanesStates[ plane ] ) {
				mPlanesStates[ plane ] = 0;

				PlaneStateCheck( false );

				mCurShader->SetUniform( EEGL3_PLANES_ENABLED_NAME[ plane ], 0 );
			}

			return;
		}
		case GL_POINT_SPRITE:
		{
			mCurShader->SetUniform( mPointSpriteLoc, 0 );

			break;
		}
	}

	cGL::Disable( cap );
}

void RendererGL3::EnableClientState( unsigned int array ) {
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

void RendererGL3::DisableClientState( unsigned int array ) {
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

void RendererGL3::VertexPointer ( int size, unsigned int type, int stride, const void * pointer, unsigned int allocate ) {
	const int index = mAttribsLoc[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[ EEGL_VERTEX_ARRAY ] ) {
			mAttribsLocStates[ EEGL_VERTEX_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void RendererGL3::ColorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
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

void RendererGL3::TexCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
	const int index = mTextureUnits[ mCurActiveTex ];

	if ( -1 != index ) {
		if ( 0 == mTextureUnitsStates[ mCurActiveTex ] ) {
			mTextureUnitsStates[ mCurActiveTex ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

int RendererGL3::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[ mCurActiveTex ];

	return mAttribsLoc[ State ];
}

void RendererGL3::PlaneStateCheck( bool tryEnable ) {
	int i;

	if (  tryEnable  ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[ i ] ) {
				mCurShader->SetUniform( mClippingEnabledLoc, 1 );
				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++) {
			if ( 0 != mPlanesStates[ i ] ) {
				return;
			}
		}

		mCurShader->SetUniform( mClippingEnabledLoc, 0 );
	}
}

void RendererGL3::UpdateMatrix() {
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

void RendererGL3::PushMatrix() {
	mStack->mCurMatrix->push( mStack->mCurMatrix->top() );
	UpdateMatrix();
}

void RendererGL3::PopMatrix() {
	mStack->mCurMatrix->pop();
	UpdateMatrix();
}

void RendererGL3::LoadIdentity() {
	mStack->mCurMatrix->top() = glm::mat4(1.0);
	UpdateMatrix();
}

void RendererGL3::MultMatrixf ( const float * m ) {
	mStack->mCurMatrix->top() *= toGLMmat4( m );
	UpdateMatrix();
}

void RendererGL3::Translatef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void RendererGL3::Rotatef( float angle, float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void RendererGL3::Scalef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void RendererGL3::Ortho( float left, float right, float bottom, float top, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::ortho( left, right, bottom, top , zNear, zFar );
	UpdateMatrix();
}

void RendererGL3::LookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ ) {
	mStack->mCurMatrix->top() *= glm::lookAt( glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ) );
	UpdateMatrix();
}

void RendererGL3::Perspective ( float fovy, float aspect, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	UpdateMatrix();
}

void RendererGL3::LoadMatrixf( const float * m ) {
	mStack->mCurMatrix->top() = toGLMmat4( m );
	UpdateMatrix();
}

void RendererGL3::Frustum( float left, float right, float bottom, float top, float near_val, float far_val ) {
	mStack->mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	UpdateMatrix();
}

void RendererGL3::GetCurrentMatrix( unsigned int mode, float * m ) {
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

unsigned int RendererGL3::GetCurrentMatrixMode() {
	return mCurrentMode;
}

void RendererGL3::MatrixMode(unsigned int mode) {
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

void RendererGL3::Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
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

void RendererGL3::Clip2DPlaneDisable() {
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

void RendererGL3::PointSize( float size ) {
	mCurShader->SetUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void RendererGL3::ClipPlane( unsigned int plane, const double * equation ) {
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

	glUniform4f( location, (float)teq[0], (float)teq[1], (float)teq[2], (float)teq[3] );
}

float RendererGL3::PointSize() {
	return mPointSize;
}

void RendererGL3::ClientActiveTexture( unsigned int texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

void RendererGL3::TexEnvi( unsigned int target, unsigned int pname, int param ) {
	//! @TODO: Implement TexEnvi
}

std::string RendererGL3::GetBaseVertexShader() {
	return mBaseVertexShader;
}

int RendererGL3::Project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz ) {
	glm::vec3 tv3( glm::project( glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

int RendererGL3::UnProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz ) {
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
