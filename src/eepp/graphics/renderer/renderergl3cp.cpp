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

#ifdef EE_GLES2
const GLchar * GL3CP_SHADER_HEAD = "precision mediump float;\nprecision lowp int;\n";
#else
const GLchar * GL3CP_SHADER_HEAD = "#version 330\n";
#endif

#if !defined( EE_GLES2 )

#include "shaders/basegl3cp.vert.h"

#include "shaders/basegl3cp.frag.h"

#else

#include "shaders/basegl3cp.gles2.vert.h"

#include "shaders/basegl3cp.gles2.frag.h"

#endif

RendererGL3CP::RendererGL3CP() :
	RendererGLShader(),
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
}

RendererGL3CP::~RendererGL3CP() {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( mVBO ); i++ ) {
		if ( 0 != mVBO[i] ) {
			glDeleteBuffersARB( 1, &mVBO[i] );
		}
	}

	deleteVertexArrays( 1, &mVAO );

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

		std::string vs( GL3CP_SHADER_HEAD );
		std::string fs( GL3CP_SHADER_HEAD );

		vs += EEGL3CP_SHADER_BASE_VS;
		fs += EEGL3CP_SHADER_BASE_FS;

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
	mTextureMatrix_id		= mCurShader->getUniformLocation( "dgl_TextureMatrix" );
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

	matrixMode( GL_TEXTURE );
	updateMatrix();
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
			}

			mCurShader->setUniform( EEGL3CP_PLANES_ENABLED_NAME[ plane ], 1 );

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
			}

			mCurShader->setUniform( EEGL3CP_PLANES_ENABLED_NAME[ plane ], 0 );

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
