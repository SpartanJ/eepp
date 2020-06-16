#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>

#ifdef EE_GL3_ENABLED

#include <eepp/graphics/renderer/rendererstackhelper.hpp>

namespace EE { namespace Graphics {

const char* EEGL3_STATES_NAME[] = {"dgl_Vertex", "dgl_Normal", "dgl_FrontColor"};

const char* EEGL3_TEXTUREUNIT_NAMES[] = {"dgl_MultiTexCoord0", "dgl_MultiTexCoord1",
										 "dgl_MultiTexCoord2", "dgl_MultiTexCoord3"};

const char* EEGL3_PLANES_ENABLED_NAME[] = {"dgl_ClipEnabled[0]", "dgl_ClipEnabled[1]",
										   "dgl_ClipEnabled[2]", "dgl_ClipEnabled[3]",
										   "dgl_ClipEnabled[4]", "dgl_ClipEnabled[5]"};

const char* EEGL3_PLANES_NAME[] = {"dgl_ClipPlane[0]", "dgl_ClipPlane[1]", "dgl_ClipPlane[2]",
								   "dgl_ClipPlane[3]", "dgl_ClipPlane[4]", "dgl_ClipPlane[5]"};

#ifdef EE_GLES2
const GLchar* GL3_SHADER_HEAD = "precision mediump float;\nprecision lowp int;\n";
#else
const GLchar* GL3_SHADER_HEAD = "#version 120\n";
#endif

#include "shaders/basegl3.vert.h"

#include "shaders/basegl3.frag.h"

RendererGL3::RendererGL3() :
	RendererGLShader(),
	mTexActive( 1 ),
	mTexActiveLoc( -1 ),
	mPointSpriteLoc( -1 ),
	mClippingEnabledLoc( -1 ),
	mPointSize( 1.f ),
	mCurActiveTex( 0 ),
	mLoaded( false ) {
#if defined( EE_GLES2 ) || defined( EE_GLES_BOTH )
	mQuadsSupported = false;
	mQuadVertexs = 6;
#endif
	Renderer::enable( GL_VERTEX_PROGRAM_POINT_SIZE );
}

RendererGL3::~RendererGL3() {}

GraphicsLibraryVersion RendererGL3::version() {
	return GLv_3;
}

std::string RendererGL3::versionStr() {
	return "OpenGL 3";
}

void RendererGL3::init() {
	if ( !mLoaded ) {
		Uint32 i;

		Renderer::init();

		std::string vs( GL3_SHADER_HEAD );
		std::string fs( GL3_SHADER_HEAD );

		vs += EEGL3_SHADER_BASE_VS;
		fs += EEGL3_SHADER_BASE_FS;

		mBaseVertexShader = vs;

		for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
			mAttribsLoc[i] = -1;
			mAttribsLocStates[i] = 0;
		}

		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			mPlanes[i] = -1;
			mPlanesStates[i] = 0;
		}

		for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			mTextureUnits[i] = -1;
			mTextureUnitsStates[i] = 0;
		}

		Shader::ensure( false );

		mShaders[EEGL3_SHADER_BASE] =
			ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[EEGL3_SHADER_BASE]->setReloadCb( cb::Make1( this, &RendererGL3::reloadShader ) );

		Shader::ensure( true );

		setShader( EEGL3_SHADER_BASE );
	} else {
		mCurShader = NULL;

		mShaders[EEGL3_SHADER_BASE]->reload();

		setShader( EEGL3_SHADER_BASE );
	}

	clientActiveTexture( GL_TEXTURE0 );

	mLoaded = true;
}

unsigned int RendererGL3::baseShaderId() {
	return mCurShader->getHandler();
}

void RendererGL3::reloadCurrentShader() {
	reloadShader( mCurShader );
}

void RendererGL3::reloadShader( ShaderProgram* Shader ) {
	mCurShader = NULL;

	setShader( Shader );
}

void RendererGL3::setShader( const EEGL3_SHADERS& Shader ) {
	setShader( mShaders[Shader] );
}

void RendererGL3::setShader( ShaderProgram* Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[EEGL3_SHADER_BASE];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	if ( -1 == mAttribsLoc[EEGL_VERTEX_ARRAY] )
		disableClientState( GL_VERTEX_ARRAY );

	if ( -1 == mAttribsLoc[EEGL_COLOR_ARRAY] )
		disableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( -1 == mTextureUnits[mCurActiveTex] )
		disableClientState( GL_COLOR_ARRAY );

	mShaderPrev = mCurShader;
	mCurShader = Shader;
	mProjectionMatrix_id = mCurShader->getUniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id = mCurShader->getUniformLocation( "dgl_ModelViewMatrix" );
	mTextureMatrix_id = mCurShader->getUniformLocation( "dgl_TextureMatrix" );
	mTexActiveLoc = mCurShader->getUniformLocation( "dgl_TexActive" );
	mPointSpriteLoc = mCurShader->getUniformLocation( "dgl_PointSpriteActive" );
	mClippingEnabledLoc = mCurShader->getUniformLocation( "dgl_ClippingEnabled" );
	mCurActiveTex = 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mAttribsLoc[i] = mCurShader->getAttributeLocation( EEGL3_STATES_NAME[i] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		mPlanes[i] = mCurShader->getUniformLocation( EEGL3_PLANES_NAME[i] );
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[i] = mCurShader->getAttributeLocation( EEGL3_TEXTUREUNIT_NAMES[i] );
	}

	glUseProgram( mCurShader->getHandler() );

	if ( -1 != mAttribsLoc[EEGL_VERTEX_ARRAY] )
		enableClientState( GL_VERTEX_ARRAY );

	if ( -1 != mAttribsLoc[EEGL_COLOR_ARRAY] )
		enableClientState( GL_COLOR_ARRAY );

	if ( -1 != mTextureUnits[mCurActiveTex] )
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
		if ( -1 != mPlanes[i] ) {
			mCurShader->setUniform( EEGL3_PLANES_ENABLED_NAME[i], 0 );
		}
	}

	if ( -1 != mPointSpriteLoc ) {
		mCurShader->setUniform( mPointSpriteLoc, 0 );
	}
}

void RendererGL3::enable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D: {
			if ( 0 == mTexActive ) {
				mTexActive = 1;

				mCurShader->setUniform( mTexActiveLoc, mTexActive );
			}

			return;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5: {
			int plane = cap - GL_CLIP_PLANE0;

			if ( 0 == mPlanesStates[plane] ) {
				mPlanesStates[plane] = 1;

				planeStateCheck( true );

				mCurShader->setUniform( EEGL3_PLANES_ENABLED_NAME[plane], 1 );
			}

			return;
		}
		case GL_POINT_SPRITE: {
			mCurShader->setUniform( mPointSpriteLoc, 1 );

			break;
		}
	}

	Renderer::enable( cap );
}

void RendererGL3::disable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D: {
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
		case GL_CLIP_PLANE5: {
			int plane = cap - GL_CLIP_PLANE0;

			if ( 1 == mPlanesStates[plane] ) {
				mPlanesStates[plane] = 0;

				planeStateCheck( false );

				mCurShader->setUniform( EEGL3_PLANES_ENABLED_NAME[plane], 0 );
			}

			return;
		}
		case GL_POINT_SPRITE: {
			mCurShader->setUniform( mPointSpriteLoc, 0 );

			break;
		}
	}

	Renderer::disable( cap );
}

void RendererGL3::enableClientState( unsigned int array ) {
	int state;

	if ( GL_TEXTURE_COORD_ARRAY == array ) {
		if ( -1 != ( state = mTextureUnits[mCurActiveTex] ) ) {
			mTextureUnitsStates[mCurActiveTex] = 1;

			glEnableVertexAttribArray( state );
		}
	} else {
		Int32 Pos = array - GL_VERTEX_ARRAY;

		if ( -1 != ( state = mAttribsLoc[Pos] ) ) {
			mAttribsLocStates[Pos] = 1;

			glEnableVertexAttribArray( state );
		}
	}
}

void RendererGL3::disableClientState( unsigned int array ) {
	int state;

	if ( GL_TEXTURE_COORD_ARRAY == array ) {
		if ( -1 != ( state = mTextureUnits[mCurActiveTex] ) ) {
			mTextureUnitsStates[mCurActiveTex] = 0;

			glDisableVertexAttribArray( state );
		}
	} else {
		Int32 Pos = array - GL_VERTEX_ARRAY;

		if ( -1 != ( state = mAttribsLoc[Pos] ) ) {
			mAttribsLocStates[Pos] = 0;

			glDisableVertexAttribArray( state );
		}
	}
}

void RendererGL3::vertexPointer( int size, unsigned int type, int stride, const void* pointer,
								 unsigned int allocate ) {
	const int index = mAttribsLoc[EEGL_VERTEX_ARRAY];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[EEGL_VERTEX_ARRAY] ) {
			mAttribsLocStates[EEGL_VERTEX_ARRAY] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void RendererGL3::colorPointer( int size, unsigned int type, int stride, const void* pointer,
								unsigned int allocate ) {
	const int index = mAttribsLoc[EEGL_COLOR_ARRAY];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[EEGL_COLOR_ARRAY] ) {
			mAttribsLocStates[EEGL_COLOR_ARRAY] = 1;

			glEnableVertexAttribArray( index );
		}

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointerARB( index, size, type, GL_TRUE, stride, pointer );
		} else {
			glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
		}
	}
}

void RendererGL3::texCoordPointer( int size, unsigned int type, int stride, const void* pointer,
								   unsigned int allocate ) {
	const int index = mTextureUnits[mCurActiveTex];

	if ( -1 != index ) {
		if ( 0 == mTextureUnitsStates[mCurActiveTex] ) {
			mTextureUnitsStates[mCurActiveTex] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

int RendererGL3::getStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[mCurActiveTex];

	return mAttribsLoc[State];
}

void RendererGL3::planeStateCheck( bool tryEnable ) {
	int i;

	if ( tryEnable ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[i] ) {
				mCurShader->setUniform( mClippingEnabledLoc, 1 );
				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[i] ) {
				return;
			}
		}

		mCurShader->setUniform( mClippingEnabledLoc, 0 );
	}
}

void RendererGL3::clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width,
									 const Int32& Height ) {
	Rectf r( x, y, x + Width, y + Height );

	glm::vec4 vclip_left( 1.0, 0.0, 0.0, -r.Left );
	glm::vec4 vclip_right( -1.0, 0.0, 0.0, r.Right );
	glm::vec4 vclip_top( 0.0, 1.0, 0.0, -r.Top );
	glm::vec4 vclip_bottom( 0.0, -1.0, 0.0, r.Bottom );

	glm::mat4 invMV = glm::inverse( mStack->mModelViewMatrix.top() );

	vclip_left = vclip_left * invMV;
	vclip_right = vclip_right * invMV;
	vclip_top = vclip_top * invMV;
	vclip_bottom = vclip_bottom * invMV;

	GLi->enable( GL_CLIP_PLANE0 );
	GLi->enable( GL_CLIP_PLANE1 );
	GLi->enable( GL_CLIP_PLANE2 );
	GLi->enable( GL_CLIP_PLANE3 );

	glUniform4fv( mPlanes[0], 1, static_cast<const float*>( &vclip_left[0] ) );
	glUniform4fv( mPlanes[1], 1, static_cast<const float*>( &vclip_right[0] ) );
	glUniform4fv( mPlanes[2], 1, static_cast<const float*>( &vclip_top[0] ) );
	glUniform4fv( mPlanes[3], 1, static_cast<const float*>( &vclip_bottom[0] ) );
}

void RendererGL3::clip2DPlaneDisable() {
	GLi->disable( GL_CLIP_PLANE0 );
	GLi->disable( GL_CLIP_PLANE1 );
	GLi->disable( GL_CLIP_PLANE2 );
	GLi->disable( GL_CLIP_PLANE3 );
}

void RendererGL3::pointSize( float size ) {
	mCurShader->setUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void RendererGL3::clipPlane( unsigned int plane, const double* equation ) {
	Int32 nplane = plane - GL_CLIP_PLANE0;
	Int32 location;

	if ( nplane < EE_MAX_PLANES ) {
		location = mPlanes[nplane];
	} else {
		std::string planeNum( "dgl_ClipPlane[" + String::toString( nplane ) + "]" );

		location = glGetUniformLocation( mCurShader->getHandler(), (GLchar*)&planeNum[0] );
	}

	glm::vec4 teq( equation[0], equation[1], equation[2], equation[3] );

	teq = teq *
		  glm::inverse( mStack->mModelViewMatrix
							.top() ); /// Apply the inverse of the model view matrix to the equation

	glUniform4f( location, (float)teq[0], (float)teq[1], (float)teq[2], (float)teq[3] );
}

float RendererGL3::pointSize() {
	return mPointSize;
}

void RendererGL3::clientActiveTexture( unsigned int texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

std::string RendererGL3::getBaseVertexShader() {
	return mBaseVertexShader;
}

}} // namespace EE::Graphics

#endif
