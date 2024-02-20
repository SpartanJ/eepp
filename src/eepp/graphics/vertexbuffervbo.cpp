#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/renderer/renderergl3.hpp>
#include <eepp/graphics/renderer/renderergl3cp.hpp>
#include <eepp/graphics/renderer/renderergles2.hpp>
#include <eepp/graphics/vertexbuffervbo.hpp>

namespace EE { namespace Graphics {

VertexBufferVBO::VertexBufferVBO( const Uint32& vertexFlags, PrimitiveType drawType,
								  const Int32& reserveVertexSize, const Int32& reserveIndexSize,
								  VertexBufferUsageType usageType ) :
	VertexBuffer( vertexFlags, drawType, reserveVertexSize, reserveIndexSize, usageType ),
	mCompiled( false ),
	mBuffersSet( false ),
	mTextured( false ),
	mVAO( 0 ),
	mElementHandle( 0 ) {
	for ( int i = 0; i < VERTEX_FLAGS_COUNT; i++ )
		mArrayHandle[i] = 0;
}

VertexBufferVBO::~VertexBufferVBO() {
	clear();
}

void VertexBufferVBO::bind() {
	GlobalBatchRenderer::instance()->draw();

	if ( !mCompiled ) {
		compile();

		if ( !mCompiled )
			return;
	}

	setVertexStates();
}

bool VertexBufferVBO::compile() {
	if ( mCompiled )
		return false;

	int curVAO = 0;
#ifndef EE_GLES
	if ( GLv_3CP == GLi->version() ) {
		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		GLi->genVertexArrays( 1, &mVAO );
		GLi->bindVertexArray( mVAO );
	}
#endif

	unsigned int usageType = GL_STATIC_DRAW;
	if ( mUsageType == VertexBufferUsageType::Dynamic )
		usageType = GL_DYNAMIC_DRAW;
	else if ( mUsageType == VertexBufferUsageType::Stream )
		usageType = GL_STREAM_DRAW;

	// Create the VBO vertex arrays
	for ( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			switch ( i ) {
				case VERTEX_FLAG_POSITION: {
					if ( mPosArray.empty() )
						return false;

					glGenBuffersARB( 1, (unsigned int*)&mArrayHandle[i] );
					glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

					if ( !mArrayHandle[i] )
						return false;

					glBufferDataARB( GL_ARRAY_BUFFER, mPosArray.size() * sizeof( Vector2f ),
									 &( mPosArray[0] ), usageType );
					break;
				}
				case VERTEX_FLAG_TEXTURE0:
				case VERTEX_FLAG_TEXTURE1:
				case VERTEX_FLAG_TEXTURE2:
				case VERTEX_FLAG_TEXTURE3:
					if ( mTexCoordArray[i - 1].empty() )
						return false;

					glGenBuffersARB( 1, (unsigned int*)&mArrayHandle[i] );
					glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

					if ( !mArrayHandle[i] )
						return false;

					glBufferDataARB( GL_ARRAY_BUFFER,
									 mTexCoordArray[i - 1].size() * sizeof( Vector2f ),
									 &mTexCoordArray[i - 1][0], usageType );
					break;
				case VERTEX_FLAG_COLOR: {
					if ( mColorArray.empty() )
						return false;

					glGenBuffersARB( 1, (unsigned int*)&mArrayHandle[i] );
					glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

					if ( !mArrayHandle[i] )
						return false;

					glBufferDataARB( GL_ARRAY_BUFFER, mColorArray.size() * sizeof( Color ),
									 &mColorArray[0], usageType );
					break;
				}
				default:
					break;
			}
		}
	}

	if ( GLv_3CP != GLi->version() ) {
		glBindBufferARB( GL_ARRAY_BUFFER, 0 );
	}

	// Create the VBO index array
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) && !mIndexArray.empty() ) {
		glGenBuffersARB( 1, (unsigned int*)&mElementHandle );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER, getIndexCount() * sizeof( Uint32 ),
						 &mIndexArray[0], usageType );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}

	mCompiled = true;
	mBuffersSet = false;

	if ( GLv_3CP == GLi->version() ) {
		GLi->bindVertexArray( curVAO );
	}

	return true;
}

void VertexBufferVBO::draw() {
	if ( !mCompiled )
		return;

	int curVAO = 0;
#ifndef EE_GLES
	if ( GLv_3CP == GLi->version() ) {
		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		GLi->bindVertexArray( mVAO );
	}
#endif

	if ( GLv_3 == GLi->version() || GLv_3CP == GLi->version() || GLv_ES2 == GLi->version() ) {
		if ( !mTextured ) {
			GLi->disable( GL_TEXTURE_2D );
		}
	}

	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		Int32 lSize = mElemDraw;

		if ( mElemDraw < 0 )
			lSize = getIndexCount();

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		GLi->drawElements( mDrawType, lSize, GL_UNSIGNED_INT, (char*)NULL );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	} else {
		GLi->drawArrays( mDrawType, 0, getVertexCount() );
	}

	if ( GLv_3CP == GLi->version() ) {
		GLi->bindVertexArray( curVAO );
	}
}

void VertexBufferVBO::setVertexStates() {
#ifdef EE_GL3_ENABLED
	int index;
#endif

	int curVAO = 0;
#ifndef EE_GLES
	if ( GLv_3CP == GLi->version() ) {
		if ( mBuffersSet ) {
			return;
		}

		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		GLi->bindVertexArray( mVAO );
	}
#endif

	/// TEXTURES
	if ( GLi->isExtension( EEGL_ARB_multitexture ) ) {
		mTextured = false;

		for ( Int32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
				GLi->clientActiveTexture( GL_TEXTURE0 + i );
				GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );

				glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[VERTEX_FLAG_TEXTURE0 + i] );

#ifdef EE_GL3_ENABLED
				if ( GLv_3 == GLi->version() || GLv_3CP == GLi->version() ||
					 GLv_ES2 == GLi->version() ) {
					index =
						GLv_3 == GLi->version()
							? GLi->getRendererGL3()->getStateIndex( EEGL_TEXTURE_COORD_ARRAY )
							: ( GLv_3CP == GLi->version() ? GLi->getRendererGL3CP()->getStateIndex(
																EEGL_TEXTURE_COORD_ARRAY )
														  : GLi->getRendererGLES2()->getStateIndex(
																EEGL_TEXTURE_COORD_ARRAY ) );

					if ( -1 != index )
						glVertexAttribPointerARB( index,
												  VertexElementCount[VERTEX_FLAG_TEXTURE0 + i],
												  GL_FP, GL_FALSE, 0, 0 );
				} else
#endif
				{
					GLi->texCoordPointer( VertexElementCount[VERTEX_FLAG_TEXTURE0 + i], GL_FP, 0,
										  (char*)NULL, 0 );
				}

				mTextured = true;
			} else {
				if ( 0 == i ) {
					GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );
					GLi->disable( GL_TEXTURE_2D );
					break;
				}
			}
		}
	} else {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
			GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[VERTEX_FLAG_TEXTURE0] );

#ifdef EE_GL3_ENABLED
			if ( GLv_3 == GLi->version() || GLv_3CP == GLi->version() ||
				 GLv_ES2 == GLi->version() ) {
				index =
					GLv_3 == GLi->version()
						? GLi->getRendererGL3()->getStateIndex( EEGL_TEXTURE_COORD_ARRAY )
						: ( GLv_3CP == GLi->version()
								? GLi->getRendererGL3CP()->getStateIndex( EEGL_TEXTURE_COORD_ARRAY )
								: GLi->getRendererGLES2()->getStateIndex(
									  EEGL_TEXTURE_COORD_ARRAY ) );

				if ( -1 != index )
					glVertexAttribPointerARB( index, VertexElementCount[VERTEX_FLAG_TEXTURE0],
											  GL_FP, GL_FALSE, 0, 0 );
			} else
#endif
			{
				GLi->texCoordPointer( VertexElementCount[VERTEX_FLAG_TEXTURE0], GL_FP, 0,
									  (char*)NULL, 0 );
			}

			mTextured = true;
		} else {
			GLi->disable( GL_TEXTURE_2D );
			GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );

			mTextured = false;
		}
	}

	/// POSITION
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->enableClientState( GL_VERTEX_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[VERTEX_FLAG_POSITION] );

#ifdef EE_GL3_ENABLED
		if ( GLv_3 == GLi->version() || GLv_3CP == GLi->version() || GLv_ES2 == GLi->version() ) {
			index = GLv_3 == GLi->version()
						? GLi->getRendererGL3()->getStateIndex( EEGL_VERTEX_ARRAY )
						: ( GLv_3CP == GLi->version()
								? GLi->getRendererGL3CP()->getStateIndex( EEGL_VERTEX_ARRAY )
								: GLi->getRendererGLES2()->getStateIndex( EEGL_VERTEX_ARRAY ) );

			if ( -1 != index )
				glVertexAttribPointerARB( index, VertexElementCount[VERTEX_FLAG_POSITION], GL_FP,
										  GL_FALSE, 0, 0 );
		} else
#endif
		{
			GLi->vertexPointer( VertexElementCount[VERTEX_FLAG_POSITION], GL_FP, 0, (char*)NULL,
								0 );
		}
	} else {
		GLi->disableClientState( GL_VERTEX_ARRAY );
	}

	/// COLOR
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->enableClientState( GL_COLOR_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[VERTEX_FLAG_COLOR] );

#ifdef EE_GL3_ENABLED
		if ( GLv_3 == GLi->version() || GLv_3CP == GLi->version() || GLv_ES2 == GLi->version() ) {
			index = GLv_3 == GLi->version()
						? GLi->getRendererGL3()->getStateIndex( EEGL_COLOR_ARRAY )
						: ( GLv_3CP == GLi->version()
								? GLi->getRendererGL3CP()->getStateIndex( EEGL_COLOR_ARRAY )
								: GLi->getRendererGLES2()->getStateIndex( EEGL_COLOR_ARRAY ) );

			if ( -1 != index )
				glVertexAttribPointerARB( index, VertexElementCount[VERTEX_FLAG_COLOR],
										  GL_UNSIGNED_BYTE, GL_TRUE, 0, 0 );
		} else
#endif
		{
			GLi->colorPointer( VertexElementCount[VERTEX_FLAG_COLOR], GL_UNSIGNED_BYTE, 0,
							   (char*)NULL, 0 );
		}
	} else {
		GLi->disableClientState( GL_COLOR_ARRAY );
	}

	mBuffersSet = true;

	if ( GLv_3CP == GLi->version() ) {
		GLi->bindVertexArray( curVAO );
	}
}

void VertexBufferVBO::update( const Uint32& types, bool indices ) {
	unsigned int usageType = GL_STATIC_DRAW;
	if ( mUsageType == VertexBufferUsageType::Dynamic )
		usageType = GL_DYNAMIC_DRAW;
	else if ( mUsageType == VertexBufferUsageType::Stream )
		usageType = GL_STREAM_DRAW;

	for ( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) && VERTEX_FLAG_QUERY( types, i ) ) {
			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				switch ( i ) {
					case VERTEX_FLAG_POSITION: {
						glBufferDataARB( GL_ARRAY_BUFFER, mPosArray.size() * sizeof( Vector2f ),
										 &( mPosArray[0] ), usageType );
						break;
					}
					case VERTEX_FLAG_TEXTURE0:
					case VERTEX_FLAG_TEXTURE1:
					case VERTEX_FLAG_TEXTURE2:
					case VERTEX_FLAG_TEXTURE3:
						glBufferDataARB( GL_ARRAY_BUFFER,
										 mTexCoordArray[i - 1].size() * sizeof( Vector2f ),
										 &mTexCoordArray[i - 1][0], usageType );
						break;
					case VERTEX_FLAG_COLOR: {
						glBufferDataARB( GL_ARRAY_BUFFER, mColorArray.size() * sizeof( Color ),
										 &mColorArray[0], usageType );
						break;
					}
					default:
						break;
				}
			}
		}
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) && indices ) {
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER, getIndexCount() * sizeof( Uint32 ),
						 &mIndexArray[0], usageType );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}

	mBuffersSet = false;
}

void VertexBufferVBO::reload() {
	mCompiled = false;
	mBuffersSet = false;

	compile();
}

void VertexBufferVBO::unbind() {
	if ( GLv_3CP != GLi->version() ) {
		glBindBufferARB( GL_ARRAY_BUFFER, 0 );
	}

	if ( !mTextured ) {
		GLi->enable( GL_TEXTURE_2D );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	GLi->clientActiveTexture( GL_TEXTURE0 );

	if ( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->enableClientState( GL_VERTEX_ARRAY );
	}

	if ( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->enableClientState( GL_COLOR_ARRAY );
	}
}

void VertexBufferVBO::clear() {
	mCompiled = false;
	mBuffersSet = false;

	for ( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) && mArrayHandle[i] ) {
			glDeleteBuffersARB( 1, (unsigned int*)&mArrayHandle[i] );
		}
	}

	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) && mElementHandle ) {
		glDeleteBuffersARB( 1, (unsigned int*)&mElementHandle );
	}

	if ( GLv_3CP == GLi->version() && mVAO ) {
		GLi->deleteVertexArrays( 1, &mVAO );
	}

	VertexBuffer::clear();
}

}} // namespace EE::Graphics
