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

EEGL_version RendererGLES2::version() {
	return GLv_ES2;
}

std::string RendererGLES2::versionStr() {
	return "OpenGL ES 2";
}

void RendererGLES2::init() {
	if ( !mLoaded ) {
		Uint32 i;

		cGL::init();

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

		Shader::ensure( false );

		mShaders[ EEGLES2_SHADER_BASE ]			= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_BASE ]->setReloadCb( cb::Make1( this, &RendererGLES2::reloadShader ) );

		vs = EEGLES2_SHADER_CLIPPED_VS;
		fs = EEGLES2_SHADER_CLIPPED_FS;

		mShaders[ EEGLES2_SHADER_CLIPPED ]			= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_CLIPPED ]->setReloadCb( cb::Make1( this, &RendererGLES2::reloadShader ) );

		vs = EEGLES2_SHADER_POINTSPRITE_VS;
		fs = EEGLES2_SHADER_POINTSPRITE_FS;

		mShaders[ EEGLES2_SHADER_POINTSPRITE ]		= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_POINTSPRITE ]->setReloadCb( cb::Make1( this, &RendererGLES2::reloadShader ) );

		vs = EEGLES2_SHADER_PRIMITIVE_VS;
		fs = EEGLES2_SHADER_PRIMITIVE_FS;

		mShaders[ EEGLES2_SHADER_PRIMITIVE ]		= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGLES2_SHADER_PRIMITIVE ]->setReloadCb( cb::Make1( this, &RendererGLES2::reloadShader ) );

		Shader::ensure( true );

		setShader( EEGLES2_SHADER_BASE );
	} else {
		mCurShader = NULL;

		mShaders[ EEGLES2_SHADER_BASE ]->reload();

		setShader( EEGLES2_SHADER_BASE );
	}

	clientActiveTexture( GL_TEXTURE0 );

	mLoaded = true;
}

unsigned int RendererGLES2::baseShaderId() {
	return mCurShader->getHandler();
}

void RendererGLES2::reloadCurrentShader() {
	reloadShader( mCurShader );
}

void RendererGLES2::reloadShader( ShaderProgram * Shader ) {
	mCurShader = NULL;

	setShader( Shader );
}

void RendererGLES2::setShader( const EEGLES2_SHADERS& Shader ) {
	setShader( mShaders[ Shader ] );
}

void RendererGLES2::checkLocalShader() {
	for ( Uint32 i = 0; i < EEGLES2_SHADERS_COUNT; i++ ) {
		if ( mShaders[i] == mCurShader ) {
			mCurShaderLocal = true;
			return;
		}
	}

	mCurShaderLocal = false;
}

void RendererGLES2::setShader( ShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGLES2_SHADER_BASE ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	disableClientState( GL_VERTEX_ARRAY );
	disableClientState( GL_TEXTURE_COORD_ARRAY );
	disableClientState( GL_COLOR_ARRAY );

	mShaderPrev				= mCurShader;
	mCurShader				= Shader;

	checkLocalShader();

	mProjectionMatrix_id	= mCurShader->uniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->uniformLocation( "dgl_ModelViewMatrix" );
	mTexActiveLoc			= mCurShader->uniformLocation( "dgl_TexActive" );
	mClippingEnabledLoc		= mCurShader->uniformLocation( "dgl_ClippingEnabled" );
	mCurActiveTex			= 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mAttribsLoc[ i ] = mCurShader->attributeLocation( EEGLES2_STATES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mClippingEnabledLoc ) {
			mPlanes[ i ] = mCurShader->uniformLocation( EEGLES2_PLANES_NAMENABLED_NAME[ i ] );
		} else {
			mPlanes[ i ] = -1;
		}
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[ i ] = mCurShader->attributeLocation( EEGLES2_TEXTUREUNIT_NAMES[ i ] );
	}

	glUseProgram( mCurShader->getHandler() );

	if ( -1 != mAttribsLoc[ EEGL_VERTEX_ARRAY ] )
		enableClientState( GL_VERTEX_ARRAY );

	if ( -1 != mAttribsLoc[ EEGL_COLOR_ARRAY ] )
		enableClientState( GL_COLOR_ARRAY );

	if ( -1 != mTextureUnits[ mCurActiveTex ] )
		enableClientState( GL_TEXTURE_COORD_ARRAY );

	unsigned int CM = mCurrentMode;

	matrixMode( GL_PROJECTION );
	updateMatrix();
	matrixMode( GL_MODELVIEW );
	updateMatrix();
	matrixMode( CM );

	if ( -1 != mTexActiveLoc ) {
		mCurShader->setUniform( mTexActiveLoc, mTexActive );
	}

	if ( -1 != mClippingEnabledLoc ) {
		mCurShader->setUniform( mClippingEnabledLoc, 0 );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mPlanes[ i ] ) {
			mCurShader->setUniform( EEGLES2_PLANES_ENABLED_NAME[ i ], 0 );
		}
	}
}

void RendererGLES2::enable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 0 == mTexActive ) {
				mTexActive = 1;

				setShader( EEGLES2_SHADER_BASE );
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

			setShader( EEGLES2_SHADER_CLIPPED );

			mPlanesStates[ plane ] = 1;

			planeStateCheck( true );

			mCurShader->setUniform( EEGLES2_PLANES_ENABLED_NAME[ plane ], 1 );

			return;
		}
		case GL_POINT_SPRITE:
		{
			mPointSpriteEnabled = 1;

			//cGL::Enable( GL_VERTEX_PROGRAM_POINT_SIZE );

			setShader( EEGLES2_SHADER_POINTSPRITE );

			return;
		}
	}

	cGL::enable( cap );
}

void RendererGLES2::disable ( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 1 == mTexActive ) {
				mTexActive = 0;

				setShader( EEGLES2_SHADER_PRIMITIVE );
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
				setShader( EEGLES2_SHADER_BASE );
			} else {
				setShader( EEGLES2_SHADER_PRIMITIVE );
			}

			mPlanesStates[ plane ] = 0;

			planeStateCheck( false );

			return;
		}
		case GL_POINT_SPRITE:
		{
			mPointSpriteEnabled = 0;

			//cGL::Disable( GL_VERTEX_PROGRAM_POINT_SIZE );

			setShader( EEGLES2_SHADER_BASE );

			return;
		}
	}

	cGL::disable( cap );
}

void RendererGLES2::enableClientState( unsigned int array ) {
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

void RendererGLES2::disableClientState( unsigned int array ) {
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

void RendererGLES2::vertexPointer( int size, unsigned int type, int stride, const void * pointer, unsigned int allocate ) {
	const int index = mAttribsLoc[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[ EEGL_VERTEX_ARRAY ] ) {
			mAttribsLocStates[ EEGL_VERTEX_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void RendererGLES2::colorPointer( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
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

void RendererGLES2::texCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
	if ( mCurShaderLocal ) {
		if ( 1 == mTexActive ) {
			if ( mCurShader == mShaders[ EEGLES2_SHADER_PRIMITIVE ] ) {
				if ( mClippingEnabled ) {
					setShader( EEGLES2_SHADER_CLIPPED );
				} else if ( mPointSpriteEnabled ) {
					setShader( EEGLES2_SHADER_POINTSPRITE );
				} else {
					setShader( EEGLES2_SHADER_BASE );
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

int RendererGLES2::getStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[ mCurActiveTex ];

	return mAttribsLoc[ State ];
}

void RendererGLES2::planeStateCheck( bool tryEnable ) {
	int i;

	if (  tryEnable  ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[ i ] ) {
				mCurShader->setUniform( mClippingEnabledLoc, 1 );

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

void RendererGLES2::updateMatrix() {
	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		{
			if ( -1 != mProjectionMatrix_id ) {
				mCurShader->setUniformMatrix( mProjectionMatrix_id, &mStack->mProjectionMatrix.top()[0][0] );
			}

			break;
		}
		case GL_MODELVIEW:
		{
			if ( -1 != mModelViewMatrix_id ) {
				mCurShader->setUniformMatrix( mModelViewMatrix_id, &mStack->mModelViewMatrix.top()[0][0] );
			}

			break;
		}
	}
}

void RendererGLES2::pushMatrix() {
	mStack->mCurMatrix->push( mStack->mCurMatrix->top() );
	updateMatrix();
}

void RendererGLES2::popMatrix() {
	mStack->mCurMatrix->pop();
	updateMatrix();
}

void RendererGLES2::loadIdentity() {
	mStack->mCurMatrix->top() = glm::mat4(1.0);
	updateMatrix();
}

void RendererGLES2::multMatrixf ( const float * m ) {
	mStack->mCurMatrix->top() *= toGLMmat4( m );
	updateMatrix();
}

void RendererGLES2::translatef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGLES2::rotatef( float angle, float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGLES2::scalef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGLES2::ortho( float left, float right, float bottom, float top, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::ortho( left, right, bottom, top , zNear, zFar );
	updateMatrix();
}

void RendererGLES2::lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ ) {
	mStack->mCurMatrix->top() *= glm::lookAt( glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ) );
	updateMatrix();
}

void RendererGLES2::perspective( float fovy, float aspect, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	updateMatrix();
}

void RendererGLES2::loadMatrixf( const float * m ) {
	mStack->mCurMatrix->top() = toGLMmat4( m );
	updateMatrix();
}

void RendererGLES2::frustum( float left, float right, float bottom, float top, float near_val, float far_val ) {
	mStack->mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	updateMatrix();
}

void RendererGLES2::getCurrentMatrix( unsigned int mode, float * m ) {
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

unsigned int RendererGLES2::getCurrentMatrixMode() {
	return mCurrentMode;
}

void RendererGLES2::matrixMode(unsigned int mode) {
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

void RendererGLES2::clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
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

	GLi->enable(GL_CLIP_PLANE0);
	GLi->enable(GL_CLIP_PLANE1);
	GLi->enable(GL_CLIP_PLANE2);
	GLi->enable(GL_CLIP_PLANE3);

	glUniform4fv( mPlanes[0], 1, static_cast<const float*>( &vclip_left[0]	)	);
	glUniform4fv( mPlanes[1], 1, static_cast<const float*>( &vclip_right[0]	)	);
	glUniform4fv( mPlanes[2], 1, static_cast<const float*>( &vclip_top[0]		)	);
	glUniform4fv( mPlanes[3], 1, static_cast<const float*>( &vclip_bottom[0]	)	);

	if ( mPushClip ) {
		mPlanesClipped.push_back( Rectf( x, y, Width, Height ) );
	}
}

void RendererGLES2::clip2DPlaneDisable() {
	if ( ! mPlanesClipped.empty() ) { // This should always be true
		mPlanesClipped.pop_back();
	}

	if ( mPlanesClipped.empty() ) {
		GLi->disable(GL_CLIP_PLANE0);
		GLi->disable(GL_CLIP_PLANE1);
		GLi->disable(GL_CLIP_PLANE2);
		GLi->disable(GL_CLIP_PLANE3);
	} else {
		Rectf R( mPlanesClipped.back() );
		mPushClip = false;
		clip2DPlaneEnable( R.Left, R.Top, R.Right, R.Bottom );
		mPushClip = true;
	}
}

void RendererGLES2::pointSize( float size ) {
	#if !defined( EE_GLES2 ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glPointSize( size );
	#endif

	mCurShader->setUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void RendererGLES2::clipPlane( unsigned int plane, const double * equation ) {
	Int32 nplane	= plane - GL_CLIP_PLANE0;
	Int32 location;

	if ( nplane < EE_MAX_PLANES ) {
		location = mPlanes[ nplane ];
	} else {
		std::string planeNum( "dgl_ClipPlane[" + String::toStr( nplane ) + "]" );

		location = glGetUniformLocation( mCurShader->getHandler(), (GLchar*)&planeNum[0] );
	}

	glm::vec4 teq( equation[0], equation[1], equation[2], equation[3] );

	teq = teq * glm::inverse( mStack->mModelViewMatrix.top() );		/// Apply the inverse of the model view matrix to the equation

	glUniform4f( location, (float)teq[0], (float)teq[1], (float)teq[2], (float)teq[3] );
}

float RendererGLES2::pointSize() {
	return mPointSize;
}

void RendererGLES2::clientActiveTexture( unsigned int texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

void RendererGLES2::texEnvi( unsigned int target, unsigned int pname, int param ) {
	//! @TODO: Implement TexEnvi
}

std::string RendererGLES2::getBaseVertexShader() {
	return mBaseVertexShader;
}

int RendererGLES2::project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz ) {
	glm::vec3 tv3( glm::project( glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

int RendererGLES2::unProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz ) {
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
