#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderergl3cp.hpp>

#ifdef EE_GL3_ENABLED

#include <eepp/system/log.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/graphics/renderer/rendererstackhelper.hpp>

namespace EE { namespace Graphics {

const char * EEGL3CP_STATES_NAME[] = {
	"dgl_Vertex",
	"dgl_Normal",
	"dgl_FrontColor"
};

const char * EEGL3CP_TEXTUREUNIT_NAMES[] = {
	"dgl_MultiTexCoord0",
	"dgl_MultiTexCoord1",
	"dgl_MultiTexCoord2",
	"dgl_MultiTexCoord3"
};

const char * EEGL3CP_PLANES_ENABLED_NAME[] = {
	"dgl_ClipEnabled[0]",
	"dgl_ClipEnabled[1]",
	"dgl_ClipEnabled[2]",
	"dgl_ClipEnabled[3]",
	"dgl_ClipEnabled[4]",
	"dgl_ClipEnabled[5]"
};

const char * EEGL3CP_PLANES_NAME[] = {
	"dgl_ClipPlane[0]",
	"dgl_ClipPlane[1]",
	"dgl_ClipPlane[2]",
	"dgl_ClipPlane[3]",
	"dgl_ClipPlane[4]",
	"dgl_ClipPlane[5]"
};

#if !defined( EE_GLES2 )

const GLchar * EEGL3CP_SHADER_BASE_VS =
#include "shaders/basegl3cp.vert"

const GLchar * EEGL3CP_SHADER_BASE_FS =
#include "shaders/basegl3cp.frag"

#else

const GLchar * EEGL3CP_SHADER_BASE_VS =
#include "shaders/basegl3.vert"

const GLchar * EEGL3CP_SHADER_BASE_FS =
#include "shaders/basegl3.frag"

#endif

RendererGL3CP::RendererGL3CP() :
	mProjectionMatrix_id(0),
	mModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL),
	mShaderPrev(NULL),
	mTexActive(1),
	mTexActiveLoc(-1),
	mPointSpriteLoc(-1),
	mPointSize(1.f),
	mCurActiveTex( 0 ),
	mCurTexCoordArray( 0 ),
	mVBOSizeAlloc( 1024 * 1024 ),
	mBiggestAlloc( 0 ),
	mLoaded( false )
{
	mQuadsSupported		= false;
	mQuadVertexs		= 6;

	mStack = eeNew( MatrixStack, () );
	mStack->mProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	mStack->mModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
}

RendererGL3CP::~RendererGL3CP() {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( mVBO ); i++ ) {
		if ( 0 != mVBO[i] ) {
			glDeleteBuffersARB( 1, &mVBO[i] );
		}
	}

	deleteVertexArrays( 1, &mVAO );

	eeSAFE_DELETE( mStack );

	#ifdef EE_DEBUG
	Log::instance()->write( "Biggest VBO allocation on GL3 Renderer: " + FileSystem::sizeToString( mBiggestAlloc ) );
	#endif
}

EEGL_version RendererGL3CP::version() {
	return GLv_3CP;
}

std::string RendererGL3CP::versionStr() {
	return "OpenGL 3 Core Profile";
}

void RendererGL3CP::init() {
	if ( !mLoaded ) {
		Uint32 i;

		Renderer::init();

		std::string vs( EEGL3CP_SHADER_BASE_VS );
		std::string fs( EEGL3CP_SHADER_BASE_FS );

		mBaseVertexShader = vs;

		for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
			mAttribsLoc[ i ]		= -1;
			mAttribsLocStates[ i ]	= 0;
		}

		for ( i = 0; i < eeARRAY_SIZE(mVBO); i++ )
			mVBO[i] = 0;

		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			mPlanes[i]			= -1;
			mPlanesStates[i]	= 0;
		}

		for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			mTextureUnits[i] = -1;
			mTextureUnitsStates[i]	= 0;
		}

		Shader::ensure( false );

		mShaders[ EEGL3CP_SHADER_BASE ]			= ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[ EEGL3CP_SHADER_BASE ]->setReloadCb( cb::Make1( this, &RendererGL3CP::reloadShader ) );

		Shader::ensure( true );
	} else {
		mCurShader = NULL;

		mShaders[ EEGL3CP_SHADER_BASE ]->reload();
	}

	genVertexArrays( 1, &mVAO );
	bindVertexArray( mVAO );

	glGenBuffersARB( EEGL_ARRAY_STATES_COUNT+5, &mVBO[0] );

	allocateBuffers( mVBOSizeAlloc );

	clientActiveTexture( GL_TEXTURE0 );

	setShader( mShaders[ EEGL3CP_SHADER_BASE ] );

	mLoaded = true;
}

unsigned int RendererGL3CP::baseShaderId() {
	return mCurShader->getHandler();
}

void RendererGL3CP::reloadCurrentShader() {
	reloadShader( mCurShader );
}

void RendererGL3CP::reloadShader( ShaderProgram * Shader ) {
	mCurShader = NULL;

	setShader( Shader );
}

void RendererGL3CP::setShader( const EEGL3CP_SHADERS& Shader ) {
	setShader( mShaders[ Shader ] );
}

void RendererGL3CP::setShader( ShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGL3CP_SHADER_BASE ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	disableClientState( GL_VERTEX_ARRAY );
	disableClientState( GL_TEXTURE_COORD_ARRAY );
	disableClientState( GL_COLOR_ARRAY );

	mShaderPrev				= mCurShader;
	mCurShader				= Shader;
	mProjectionMatrix_id	= mCurShader->getUniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->getUniformLocation( "dgl_ModelViewMatrix" );
	mTexActiveLoc			= mCurShader->getUniformLocation( "dgl_TexActive" );
	mPointSpriteLoc			= mCurShader->getUniformLocation( "dgl_PointSpriteActive" );
	mClippingEnabledLoc		= mCurShader->getUniformLocation( "dgl_ClippingEnabled" );
	mCurActiveTex			= 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mAttribsLoc[ i ] = mCurShader->getAttributeLocation( EEGL3CP_STATES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		mPlanes[ i ] = mCurShader->getUniformLocation( EEGL3CP_PLANES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[ i ] = mCurShader->getAttributeLocation( EEGL3CP_TEXTUREUNIT_NAMES[ i ] );
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
		mCurShader->setUniform( mTexActiveLoc, 1 );
	}

	mCurShader->setUniform( mClippingEnabledLoc, 0 );

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mPlanes[ i ] ) {
			mCurShader->setUniform( EEGL3CP_PLANES_ENABLED_NAME[ i ], 0 );
		}
	}

	if ( -1 != mPointSpriteLoc ) {
		mCurShader->setUniform( mPointSpriteLoc, 0 );
	}
}

void RendererGL3CP::enable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 0 == mTexActive ) {
				mTexActive = 1;

				mCurShader->setUniform( mTexActiveLoc, mTexActive );
				mCurShader->setUniform( mTextureUnits[ mCurActiveTex ], mCurActiveTex );
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

				planeStateCheck( true );

				mCurShader->setUniform( EEGL3CP_PLANES_ENABLED_NAME[ plane ], 1 );
			}

			return;
		}
		case GL_POINT_SPRITE:
		{
			mCurShader->setUniform( mPointSpriteLoc, 1 );

			return;
		}
	}

	Renderer::enable( cap );
}

void RendererGL3CP::disable ( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 1 == mTexActive ) {
				mTexActive = 0;

				mCurShader->setUniform( mTexActiveLoc, mTexActive );
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

				planeStateCheck( false );

				mCurShader->setUniform( EEGL3CP_PLANES_ENABLED_NAME[ plane ], 0 );
			}

			return;
		}
		case GL_POINT_SPRITE:
		{
			mCurShader->setUniform( mPointSpriteLoc, 0 );

			return;
		}
	}

	Renderer::disable( cap );
}

void RendererGL3CP::enableClientState( unsigned int array ) {
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

void RendererGL3CP::disableClientState( unsigned int array ) {
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

void RendererGL3CP::vertexPointer ( int size, unsigned int type, int stride, const void * pointer, unsigned int allocate ) {
	const int index = mAttribsLoc[ EEGL_VERTEX_ARRAY ];

	#ifdef EE_DEBUG
	mBiggestAlloc = eemax( mBiggestAlloc, allocate );
	#endif

	if ( -1 != index ) {
		bindVertexArray( mVAO );

		if ( allocate > mVBOSizeAlloc ) {
			allocateBuffers( allocate );
		}

		glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ]			);
		glBufferSubDataARB( GL_ARRAY_BUFFER, 0, allocate, pointer );

		if ( 0 == mAttribsLocStates[ EEGL_VERTEX_ARRAY ] ) {
			mAttribsLocStates[ EEGL_VERTEX_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointerARB( index, size, type, GL_TRUE, stride, 0 );
		} else {
			glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, 0 );
		}
	}
}

void RendererGL3CP::colorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
	const int index = mAttribsLoc[ EEGL_COLOR_ARRAY ];

	#ifdef EE_DEBUG
	mBiggestAlloc = eemax( mBiggestAlloc, allocate );
	#endif

	if ( -1 != index ) {
		bindVertexArray( mVAO );

		if ( allocate > mVBOSizeAlloc ) {
			allocateBuffers( allocate );
		}

		glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ]				);
		glBufferSubDataARB( GL_ARRAY_BUFFER, 0, allocate, pointer );

		if ( 0 == mAttribsLocStates[ EEGL_COLOR_ARRAY ] ) {
			mAttribsLocStates[ EEGL_COLOR_ARRAY ] = 1;

			glEnableVertexAttribArray( index );
		}

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointerARB( index, size, type, GL_TRUE, stride, 0 );
		} else {
			glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, 0 );
		}
	}
}

void RendererGL3CP::texCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate ) {
	const int index = mTextureUnits[ mCurActiveTex ];

	#ifdef EE_DEBUG
	mBiggestAlloc = eemax( mBiggestAlloc, allocate );
	#endif

	if ( -1 != index ) {
		bindVertexArray( mVAO );

		if ( allocate > mVBOSizeAlloc ) {
			allocateBuffers( allocate );
		}

		glBindBufferARB( GL_ARRAY_BUFFER, mCurTexCoordArray );
		glBufferSubDataARB( GL_ARRAY_BUFFER, 0, allocate, pointer );

		if ( 0 == mTextureUnitsStates[ mCurActiveTex ] ) {
			mTextureUnitsStates[ mCurActiveTex ] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, 0 );
	}
}

int RendererGL3CP::getStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[ mCurActiveTex ];

	return mAttribsLoc[ State ];
}

void RendererGL3CP::planeStateCheck( bool tryEnable ) {
	int i;

	if (  tryEnable  ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[ i ] ) {
				mCurShader->setUniform( mClippingEnabledLoc, 1 );
				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++) {
			if ( 0 != mPlanesStates[ i ] ) {
				return;
			}
		}

		mCurShader->setUniform( mClippingEnabledLoc, 0 );
	}
}

void RendererGL3CP::updateMatrix() {
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

void RendererGL3CP::pushMatrix() {
	mStack->mCurMatrix->push( mStack->mCurMatrix->top() );
	updateMatrix();
}

void RendererGL3CP::popMatrix() {
	mStack->mCurMatrix->pop();
	updateMatrix();
}

void RendererGL3CP::loadIdentity() {
	mStack->mCurMatrix->top() = glm::mat4(1.0);
	updateMatrix();
}

void RendererGL3CP::multMatrixf ( const float * m ) {
	mStack->mCurMatrix->top() *= toGLMmat4( m );
	updateMatrix();
}

void RendererGL3CP::translatef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGL3CP::rotatef( float angle, float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGL3CP::scalef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGL3CP::ortho( float left, float right, float bottom, float top, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::ortho( left, right, bottom, top , zNear, zFar );
	updateMatrix();
}

void RendererGL3CP::lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ ) {
	mStack->mCurMatrix->top() *= glm::lookAt( glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ) );
	updateMatrix();
}

void RendererGL3CP::perspective ( float fovy, float aspect, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	updateMatrix();
}

void RendererGL3CP::loadMatrixf( const float * m ) {
	mStack->mCurMatrix->top() = toGLMmat4( m );
	updateMatrix();
}

void RendererGL3CP::frustum( float left, float right, float bottom, float top, float near_val, float far_val ) {
	mStack->mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	updateMatrix();
}

void RendererGL3CP::getCurrentMatrix( unsigned int mode, float * m ) {
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

unsigned int RendererGL3CP::getCurrentMatrixMode() {
	return mCurrentMode;
}

void RendererGL3CP::matrixMode(unsigned int mode) {
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

void RendererGL3CP::clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	Rectf r( x, y, x + Width, y + Height );

	glm::vec4 vclip_left	( 1.0	, 0.0	, 0.0	, -r.Left	);
	glm::vec4 vclip_right	( -1.0	, 0.0	, 0.0	, r.Right	);
	glm::vec4 vclip_top		( 0.0	, 1.0	, 0.0	, -r.Top	);
	glm::vec4 vclip_bottom	( 0.0	, -1.0	, 0.0	, r.Bottom	);

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
}

void RendererGL3CP::clip2DPlaneDisable() {
	GLi->disable(GL_CLIP_PLANE0);
	GLi->disable(GL_CLIP_PLANE1);
	GLi->disable(GL_CLIP_PLANE2);
	GLi->disable(GL_CLIP_PLANE3);
}

void RendererGL3CP::pointSize( float size ) {
	mCurShader->setUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void RendererGL3CP::clipPlane( unsigned int plane, const double * equation ) {
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

float RendererGL3CP::pointSize() {
	return mPointSize;
}

void RendererGL3CP::clientActiveTexture( unsigned int texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;

	switch ( mCurActiveTex )
	{
		case 0: mCurTexCoordArray = mVBO[ EEGL_TEXTURE_COORD_ARRAY ]; break;
		case 1: mCurTexCoordArray = mVBO[ EEGL_TEXTURE_COORD_ARRAY+1 ]; break;
		case 2: mCurTexCoordArray = mVBO[ EEGL_TEXTURE_COORD_ARRAY+2 ]; break;
		case 3: mCurTexCoordArray = mVBO[ EEGL_TEXTURE_COORD_ARRAY+3 ]; break;
	}
}

std::string RendererGL3CP::getBaseVertexShader() {
	return mBaseVertexShader;
}

int RendererGL3CP::project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz ) {
	glm::vec3 tv3( glm::project( glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

int RendererGL3CP::unProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz ) {
	glm::vec3 tv3( glm::unProject( glm::vec3( winx, winy, winz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != objx )
		*objx = tv3.x;

	if ( NULL != objy )
		*objy = tv3.y;

	if ( NULL != objz )
		*objz = tv3.z;

	return GL_TRUE;
}

void RendererGL3CP::bindGlobalVAO() {
	bindVertexArray( mVAO );
}

void RendererGL3CP::allocateBuffers( const Uint32& size ) {
	if ( mVBOSizeAlloc != size )
		Log::instance()->write( "Allocating new VBO buffers size: " + String::toStr( size ) );

	mVBOSizeAlloc = size;

	glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ] );
	glBufferDataARB( GL_ARRAY_BUFFER, mVBOSizeAlloc, NULL, GL_STREAM_DRAW );

	glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ] );
	glBufferDataARB( GL_ARRAY_BUFFER, mVBOSizeAlloc, NULL, GL_STREAM_DRAW );

	glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ] );
	glBufferDataARB( GL_ARRAY_BUFFER, mVBOSizeAlloc, NULL, GL_STREAM_DRAW );

	glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY+1 ] );
	glBufferDataARB( GL_ARRAY_BUFFER, mVBOSizeAlloc, NULL, GL_STREAM_DRAW );

	glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY+2 ] );
	glBufferDataARB( GL_ARRAY_BUFFER, mVBOSizeAlloc, NULL, GL_STREAM_DRAW );

	glBindBufferARB( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY+3 ] );
	glBufferDataARB( GL_ARRAY_BUFFER, mVBOSizeAlloc, NULL, GL_STREAM_DRAW );
}

}}

#endif
