#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>

#ifdef EE_GL3_ENABLED

#include <eepp/graphics/renderer/rendererstackhelper.hpp>

namespace EE { namespace Graphics {

const char* EEGLES2_STATES_NAME[] = { "dgl_Vertex", "dgl_Normal", "dgl_FrontColor" };

const char* EEGLES2_TEXTUREUNIT_NAMES[] = { "dgl_MultiTexCoord0", "dgl_MultiTexCoord1",
											"dgl_MultiTexCoord2", "dgl_MultiTexCoord3" };

const char* EEGLES2_PLANES_ENABLED_NAME[] = { "dgl_ClipEnabled[0]", "dgl_ClipEnabled[1]",
											  "dgl_ClipEnabled[2]", "dgl_ClipEnabled[3]",
											  "dgl_ClipEnabled[4]", "dgl_ClipEnabled[5]" };

const char* EEGLES2_PLANES_NAMENABLED_NAME[] = { "dgl_ClipPlane[0]", "dgl_ClipPlane[1]",
												 "dgl_ClipPlane[2]", "dgl_ClipPlane[3]",
												 "dgl_ClipPlane[4]", "dgl_ClipPlane[5]" };

#ifdef EE_GLES2
const GLchar* GLES2_SHADER_HEAD = "precision mediump float;\nprecision lowp int;\n";
#else
const GLchar* GLES2_SHADER_HEAD = "#version 120\n";
#endif

#include "shaders/base.frag.h"
#include "shaders/base.vert.h"

#include "shaders/clipped.frag.h"
#include "shaders/clipped.vert.h"

#include "shaders/pointsprite.frag.h"
#include "shaders/pointsprite.vert.h"

#include "shaders/primitive.frag.h"
#include "shaders/primitive.vert.h"

RendererGLES2::RendererGLES2() :
	RendererGLShader(),
	mTexActive( 1 ),
	mTexActiveLoc( -1 ),
	mClippingEnabledLoc( -1 ),
	mPointSize( 1.f ),
	mCurActiveTex( 0 ),
	mClippingEnabled( false ),
	mPointSpriteEnabled( false ),
	mLoaded( false ),
	mCurShaderLocal( true ) {
	mQuadsSupported = false;
	mQuadVertexs = 6;
#if !defined( EE_GLES2 )
	Renderer::enable( GL_VERTEX_PROGRAM_POINT_SIZE );
#endif
}

RendererGLES2::~RendererGLES2() {}

GraphicsLibraryVersion RendererGLES2::version() {
	return GLv_ES2;
}

std::string RendererGLES2::versionStr() {
	return "OpenGL ES 2";
}

void RendererGLES2::init() {
	if ( !mLoaded ) {
		Uint32 i;

		Renderer::init();

		std::string hs( GLES2_SHADER_HEAD );
		std::string vs( hs + EEGLES2_SHADER_BASE_VS );
		std::string fs( hs + EEGLES2_SHADER_BASE_FS );

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

		auto reloadShader = [this]( ShaderProgram* shader ) {
			RendererGLES2::reloadShader( shader );
		};

		mShaders[EEGLES2_SHADER_BASE] =
			ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[EEGLES2_SHADER_BASE]->setReloadCb( reloadShader );

		vs = hs + EEGLES2_SHADER_CLIPPED_VS;
		fs = hs + EEGLES2_SHADER_CLIPPED_FS;

		mShaders[EEGLES2_SHADER_CLIPPED] =
			ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[EEGLES2_SHADER_CLIPPED]->setReloadCb( reloadShader );

		vs = hs + EEGLES2_SHADER_POINTSPRITE_VS;
		fs = hs + EEGLES2_SHADER_POINTSPRITE_FS;

		mShaders[EEGLES2_SHADER_POINTSPRITE] =
			ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[EEGLES2_SHADER_POINTSPRITE]->setReloadCb( reloadShader );

		vs = hs + EEGLES2_SHADER_PRIMITIVE_VS;
		fs = hs + EEGLES2_SHADER_PRIMITIVE_FS;

		mShaders[EEGLES2_SHADER_PRIMITIVE] =
			ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		mShaders[EEGLES2_SHADER_PRIMITIVE]->setReloadCb( reloadShader );

		Shader::ensure( true );

		setShader( EEGLES2_SHADER_BASE );
	} else {
		mCurShader = NULL;

		mShaders[EEGLES2_SHADER_BASE]->reload();

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

void RendererGLES2::reloadShader( ShaderProgram* Shader ) {
	mCurShader = NULL;

	setShader( Shader );
}

void RendererGLES2::setShader( const EEGLES2_SHADERS& Shader ) {
	setShader( mShaders[Shader] );
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

void RendererGLES2::setShader( ShaderProgram* Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[EEGLES2_SHADER_BASE];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	if ( -1 == mAttribsLoc[EEGL_VERTEX_ARRAY] )
		disableClientState( GL_VERTEX_ARRAY );

	if ( -1 == mAttribsLoc[EEGL_COLOR_ARRAY] )
		disableClientState( GL_COLOR_ARRAY );

	if ( -1 == mTextureUnits[mCurActiveTex] )
		disableClientState( GL_TEXTURE_COORD_ARRAY );

	mShaderPrev = mCurShader;
	mCurShader = Shader;

	checkLocalShader();

	if ( !mCurShader )
		return;

	mProjectionMatrix_id = mCurShader->getUniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id = mCurShader->getUniformLocation( "dgl_ModelViewMatrix" );
	mTextureMatrix_id = mCurShader->getUniformLocation( "dgl_TextureMatrix" );
	mTexActiveLoc = mCurShader->getUniformLocation( "dgl_TexActive" );
	mClippingEnabledLoc = mCurShader->getUniformLocation( "dgl_ClippingEnabled" );
	mPointSizeLoc = mCurShader->getUniformLocation( "dgl_PointSize" );
	mCurActiveTex = 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mAttribsLoc[i] = mCurShader->getAttributeLocation( EEGLES2_STATES_NAME[i] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mClippingEnabledLoc ) {
			mPlanes[i] = mCurShader->getUniformLocation( EEGLES2_PLANES_NAMENABLED_NAME[i] );
		} else {
			mPlanes[i] = -1;
		}
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[i] = mCurShader->getAttributeLocation( EEGLES2_TEXTUREUNIT_NAMES[i] );
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
		mCurShader->setUniform( mTexActiveLoc, mTexActive );
	}

	if ( -1 != mClippingEnabledLoc ) {
		mCurShader->setUniform( mClippingEnabledLoc, 0 );
	}

	if ( -1 != mPointSizeLoc ) {
		mCurShader->setUniform( mPointSizeLoc, mPointSize );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mPlanes[i] ) {
			mCurShader->setUniform( EEGLES2_PLANES_ENABLED_NAME[i], 0 );
		}
	}
}

void RendererGLES2::enable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D: {
			if ( 0 == mTexActive ) {
				mTexActive = 1;

				if ( !mClippingEnabled ) {
					setShader( EEGLES2_SHADER_BASE );
				} else if ( -1 != mTexActiveLoc ) {
					mCurShader->setUniform( mTexActiveLoc, mTexActive );
				}
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

			setShader( EEGLES2_SHADER_CLIPPED );

			mPlanesStates[plane] = 1;

			planeStateCheck( true );

			mCurShader->setUniform( EEGLES2_PLANES_ENABLED_NAME[plane], 1 );

			return;
		}
		case GL_POINT_SPRITE: {
			mPointSpriteEnabled = 1;

			setShader( EEGLES2_SHADER_POINTSPRITE );

			return;
		}
	}

	Renderer::enable( cap );
}

void RendererGLES2::disable( unsigned int cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D: {
			if ( 1 == mTexActive ) {
				mTexActive = 0;

				if ( !mClippingEnabled ) {
					disableClientState( GL_TEXTURE_COORD_ARRAY );
					setShader( EEGLES2_SHADER_PRIMITIVE );
				} else if ( -1 != mTexActiveLoc ) {
					mCurShader->setUniform( mTexActiveLoc, mTexActive );
				}
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

			if ( mTexActive ) {
				setShader( EEGLES2_SHADER_BASE );
			} else {
				setShader( EEGLES2_SHADER_PRIMITIVE );
			}

			mPlanesStates[plane] = 0;

			planeStateCheck( false );

			return;
		}
		case GL_POINT_SPRITE: {
			mPointSpriteEnabled = 0;

			setShader( EEGLES2_SHADER_BASE );

			return;
		}
	}

	Renderer::disable( cap );
}

void RendererGLES2::enableClientState( unsigned int array ) {
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

void RendererGLES2::disableClientState( unsigned int array ) {
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

void RendererGLES2::vertexPointer( int size, unsigned int type, int stride, const void* pointer,
								   unsigned int /*allocate*/ ) {
	const int index = mAttribsLoc[EEGL_VERTEX_ARRAY];

	if ( -1 != index ) {
		if ( 0 == mAttribsLocStates[EEGL_VERTEX_ARRAY] ) {
			mAttribsLocStates[EEGL_VERTEX_ARRAY] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void RendererGLES2::colorPointer( int size, unsigned int type, int stride, const void* pointer,
								  unsigned int /*allocate*/ ) {
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

void RendererGLES2::texCoordPointer( int size, unsigned int type, int stride, const void* pointer,
									 unsigned int /*allocate*/ ) {
	if ( mCurShaderLocal ) {
		if ( 1 == mTexActive ) {
			if ( mCurShader == mShaders[EEGLES2_SHADER_PRIMITIVE] ) {
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

	const int index = mTextureUnits[mCurActiveTex];

	if ( -1 != index ) {
		if ( 0 == mTextureUnitsStates[mCurActiveTex] ) {
			mTextureUnitsStates[mCurActiveTex] = 1;

			glEnableVertexAttribArray( index );
		}

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

int RendererGLES2::getStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT || State == EEGL_TEXTURE_COORD_ARRAY );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[mCurActiveTex];

	return mAttribsLoc[State];
}

void RendererGLES2::planeStateCheck( bool tryEnable ) {
	int i;

	if ( tryEnable ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[i] ) {
				mCurShader->setUniform( mClippingEnabledLoc, 1 );

				mClippingEnabled = 1;

				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[i] ) {
				return;
			}
		}

		mClippingEnabled = 0;
	}
}

void RendererGLES2::clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width,
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

void RendererGLES2::clip2DPlaneDisable() {
	GLi->disable( GL_CLIP_PLANE0 );
	GLi->disable( GL_CLIP_PLANE1 );
	GLi->disable( GL_CLIP_PLANE2 );
	GLi->disable( GL_CLIP_PLANE3 );
}

void RendererGLES2::pointSize( float size ) {
#if !defined( EE_GLES2 ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	glPointSize( size );
#endif

	mCurShader->setUniform( "dgl_PointSize", size );

	mPointSize = size;
}

void RendererGLES2::clipPlane( unsigned int plane, const double* equation ) {
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

float RendererGLES2::pointSize() {
	return mPointSize;
}

void RendererGLES2::clientActiveTexture( unsigned int texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

std::string RendererGLES2::getBaseVertexShader() {
	return mBaseVertexShader;
}

}} // namespace EE::Graphics

#endif
