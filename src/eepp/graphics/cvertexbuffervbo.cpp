#include <eepp/graphics/cvertexbuffervbo.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/renderer/crenderergl3.hpp>
#include <eepp/graphics/renderer/crenderergl3cp.hpp>
#include <eepp/graphics/renderer/crenderergles2.hpp>

namespace EE { namespace Graphics {

cVertexBufferVBO::cVertexBufferVBO( const Uint32& VertexFlags, EE_DRAW_MODE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) :
	cVertexBuffer( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ),
	mCompiled( false ),
	mBuffersSet( false ),
	mTextured( false ),
	mVAO( 0 ),
	mElementHandle( 0 )
{
	memset( mArrayHandle, VERTEX_FLAGS_COUNT, 0 );
}

cVertexBufferVBO::~cVertexBufferVBO() {
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) && mArrayHandle[ i ] ) {
			glDeleteBuffersARB( 1,(unsigned int *)&mArrayHandle[ i ] );
		}
	}

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) && mElementHandle ) {
		glDeleteBuffersARB( 1, (unsigned int *)&mElementHandle );
	}

	if ( GLv_3CP == GLi->Version() && mVAO ) {
		GLi->DeleteVertexArrays( 1, &mVAO );
	}
}

void cVertexBufferVBO::Bind() {
	if ( !mCompiled ) {
		Compile();

		if ( !mCompiled )
			return;
	}

	SetVertexStates();
}

bool cVertexBufferVBO::Compile() {
	if( mCompiled )
		return false;

	int curVAO = 0;
	#ifndef EE_GLES
	if ( GLv_3CP == GLi->Version() ) {
		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		GLi->GenVertexArrays( 1, &mVAO );
		GLi->BindVertexArray( mVAO );
	}
	#endif

	unsigned int usageType = GL_STATIC_DRAW;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW;

	//Create the VBO vertex arrays
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			glGenBuffersARB( 1,(unsigned int *)&mArrayHandle[ i ] );

			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferDataARB( GL_ARRAY_BUFFER, mVertexArray[i].size() * sizeof(Float), &( mVertexArray[i][0] ), usageType );
				else
					glBufferDataARB( GL_ARRAY_BUFFER, mColorArray.size(), &mColorArray[0], usageType );
			} else {
				return false;
			}
		}
	}

	if ( GLv_3CP != GLi->Version() ) {
		glBindBufferARB( GL_ARRAY_BUFFER, 0 );
	}

	//Create the VBO index array
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		glGenBuffersARB( 1, (unsigned int *)&mElementHandle );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER, GetIndexCount() * sizeof(Uint32), &mIndexArray[0], usageType );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}

	mCompiled	= true;
	mBuffersSet	= false;

	if ( GLv_3CP == GLi->Version() ) {
		GLi->BindVertexArray( curVAO );
	}

	return true;
}

void cVertexBufferVBO::Draw() {
	if ( !mCompiled )
		return;

	int curVAO = 0;
	#ifndef EE_GLES
	if ( GLv_3CP == GLi->Version() ) {
		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		GLi->BindVertexArray( mVAO );
	}
	#endif

	if ( GLv_3 == GLi->Version() || GLv_3CP == GLi->Version() || GLv_ES2 == GLi->Version() ) {
		if ( !mTextured ) {
			GLi->Disable( GL_TEXTURE_2D );
		}
	}

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		Int32 lSize = mElemDraw;

		if( mElemDraw < 0 )
			lSize = GetIndexCount();

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glDrawElements( mDrawType, lSize, GL_UNSIGNED_INT, (char*)NULL );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	} else {
		glDrawArrays( mDrawType, 0, GetVertexCount() );
	}

	if ( GLv_3CP == GLi->Version() ) {
		GLi->BindVertexArray( curVAO );
	}
}

void cVertexBufferVBO::SetVertexStates() {
	#ifdef EE_GL3_ENABLED
	int index;
	#endif

	int curVAO = 0;
	#ifndef EE_GLES
	if ( GLv_3CP == GLi->Version() ) {
		if ( mBuffersSet ) {
			return;
		}

		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		GLi->BindVertexArray( mVAO );
	}
	#endif

	/// TEXTURES
	if ( GLi->IsExtension( EEGL_ARB_multitexture ) ) {
		mTextured = false;

		for ( Int32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
				GLi->ClientActiveTexture( GL_TEXTURE0 + i );
				GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );

				glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_TEXTURE0 + i ] );

				#ifdef EE_GL3_ENABLED
				if ( GLv_3 == GLi->Version() || GLv_3CP == GLi->Version() || GLv_ES2 == GLi->Version() ) {
					index = GLv_3 == GLi->Version() ?
							GLi->GetRendererGL3()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY ) :
							( GLv_3CP == GLi->Version() ? GLi->GetRendererGL3CP()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY ) :
														  GLi->GetRendererGLES2()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY ) );

					if ( -1 != index )
						glVertexAttribPointerARB( index, eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FP, GL_FALSE, 0, 0 );
				}
				else
				#endif
				{
					GLi->TexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FP, 0, (char*)NULL, 0 );
				}

				mTextured = true;
			} else {
				if ( 0 == i ) {
					GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
					GLi->Disable( GL_TEXTURE_2D );
					break;
				}
			}
		}
	} else {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
			GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_TEXTURE0 ] );

			#ifdef EE_GL3_ENABLED
			if ( GLv_3 == GLi->Version() || GLv_3CP == GLi->Version() || GLv_ES2 == GLi->Version() ) {
				index = GLv_3 == GLi->Version() ?
						GLi->GetRendererGL3()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY ) :
						( GLv_3CP == GLi->Version() ? GLi->GetRendererGL3CP()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY ) :
													  GLi->GetRendererGLES2()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY ) );

				if ( -1 != index )
					glVertexAttribPointerARB( index, eeVertexElements[ VERTEX_FLAG_TEXTURE0 ], GL_FP, GL_FALSE, 0, 0 );
			}
			else
			#endif
			{
				GLi->TexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 ], GL_FP, 0, (char*)NULL, 0 );
			}

			mTextured = true;
		} else {
			GLi->Disable( GL_TEXTURE_2D );
			GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );

			mTextured = false;
		}
	}

	/// POSITION
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->EnableClientState( GL_VERTEX_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_POSITION ] );

		#ifdef EE_GL3_ENABLED
		if ( GLv_3 == GLi->Version() || GLv_3CP == GLi->Version() || GLv_ES2 == GLi->Version() ) {
			index = GLv_3 == GLi->Version() ?
					GLi->GetRendererGL3()->GetStateIndex( EEGL_VERTEX_ARRAY ) :
					( GLv_3CP == GLi->Version() ? GLi->GetRendererGL3CP()->GetStateIndex( EEGL_VERTEX_ARRAY ) :
												  GLi->GetRendererGLES2()->GetStateIndex( EEGL_VERTEX_ARRAY ) );

			if ( -1 != index )
				glVertexAttribPointerARB( index, eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FP, GL_FALSE, 0, 0 );
		}
		else
		#endif
		{
			GLi->VertexPointer( eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FP, 0, (char*)NULL, 0 );
		}
	} else {
		GLi->DisableClientState( GL_VERTEX_ARRAY );
	}

	/// COLOR
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->EnableClientState( GL_COLOR_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_COLOR ] );

		#ifdef EE_GL3_ENABLED
		if ( GLv_3 == GLi->Version() || GLv_3CP == GLi->Version() || GLv_ES2 == GLi->Version() ) {
			index = GLv_3 == GLi->Version() ?
					GLi->GetRendererGL3()->GetStateIndex( EEGL_COLOR_ARRAY ) :
					( GLv_3CP == GLi->Version() ? GLi->GetRendererGL3CP()->GetStateIndex( EEGL_COLOR_ARRAY ) :
												  GLi->GetRendererGLES2()->GetStateIndex( EEGL_COLOR_ARRAY ) );

			if ( -1 != index )
				glVertexAttribPointerARB( index, eeVertexElements[ VERTEX_FLAG_COLOR ], GL_UNSIGNED_BYTE, GL_TRUE, 0, 0 );
		}
		else
		#endif
		{
			GLi->ColorPointer( eeVertexElements[ VERTEX_FLAG_COLOR ], GL_UNSIGNED_BYTE, 0, (char*)NULL, 0 );
		}
	} else {
		GLi->DisableClientState( GL_COLOR_ARRAY );
	}

	mBuffersSet = true;

	if ( GLv_3CP == GLi->Version() ) {
		GLi->BindVertexArray( curVAO );
	}
}

void cVertexBufferVBO::Update( const Uint32& Types, bool Indices ) {
	unsigned int usageType = GL_STATIC_DRAW;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW;

	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) && VERTEX_FLAG_QUERY( Types, i ) ) {
			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferDataARB( GL_ARRAY_BUFFER, mVertexArray[i].size() * sizeof(Float), &( mVertexArray[i][0] ), usageType );
				else
					glBufferDataARB( GL_ARRAY_BUFFER, mColorArray.size(), &mColorArray[0], usageType );
			}
		}
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) && Indices ) {
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER, GetIndexCount() * sizeof(Uint32), &mIndexArray[0], usageType );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}

	mBuffersSet = false;
}

void cVertexBufferVBO::Reload() {
	mCompiled	= false;
	mBuffersSet	= false;

	Compile();
}


void cVertexBufferVBO::Unbind() {
	if ( GLv_3CP != GLi->Version() ) {
		glBindBufferARB( GL_ARRAY_BUFFER, 0 );
	}

	if ( !mTextured ) {
		GLi->Enable( GL_TEXTURE_2D );
		GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	GLi->ClientActiveTexture( GL_TEXTURE0 );

	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->EnableClientState( GL_VERTEX_ARRAY );
	}

	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->EnableClientState( GL_COLOR_ARRAY );
	}
}

void cVertexBufferVBO::Clear() {
	mCompiled	= false;
	mBuffersSet	= false;
	cVertexBuffer::Clear();
}


}}
