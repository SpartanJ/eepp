#include <eepp/graphics/cvertexbuffervbo.hpp>
#include <eepp/graphics/glhelper.hpp>
#include <eepp/graphics/renderer/crenderergl3.hpp>

namespace EE { namespace Graphics {

cVertexBufferVBO::cVertexBufferVBO( const Uint32& VertexFlags, EE_DRAW_MODE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) :
	cVertexBuffer( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ),
	mCompiled( false ),
	mBuffersSet( false ),
	mTextured( false ),
	mVAO( 0 ),
	mElementHandle( 0 )
{
}

cVertexBufferVBO::~cVertexBufferVBO() {
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			glDeleteBuffersARB( 1,(GLuint *)&mArrayHandle[ i ] );
		}
	}

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		glDeleteBuffersARB( 1, (GLuint *)&mElementHandle );
	}

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	if ( GLv_3 == GLi->Version() ) {
		glDeleteVertexArrays( 1, &mVAO );
	}
	#endif
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

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	GLint curVAO = 0;
	if ( GLv_3 == GLi->Version() ) {
		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		glGenVertexArrays( 1, &mVAO );
		glBindVertexArray( mVAO );
	}
	#endif

	GLenum usageType = GL_STATIC_DRAW;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW;

	//Create the VBO vertex arrays
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			glGenBuffersARB( 1,(GLuint *)&mArrayHandle[ i ] );

			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferDataARB( GL_ARRAY_BUFFER, mVertexArray[i].size() * sizeof(eeFloat), &( mVertexArray[i][0] ), usageType );
				else
					glBufferDataARB( GL_ARRAY_BUFFER, mColorArray.size(), &mColorArray[0], usageType );
			} else {
				return false;
			}
		}
	}

	glBindBufferARB( GL_ARRAY_BUFFER, 0 );

	//Create the VBO index array
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		glGenBuffersARB( 1, (GLuint *)&mElementHandle );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER, GetIndexCount() * sizeof(Uint32), &mIndexArray[0], usageType );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	if ( GLv_3 == GLi->Version() ) {
		glBindVertexArray( curVAO );
	}
	#endif

	mCompiled	= true;
	mBuffersSet	= false;

	return true;
}

void cVertexBufferVBO::Draw() {
	if ( !mCompiled )
		return;

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	GLint curVAO = 0;
	#endif
	if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() ) {
		#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		glBindVertexArray( mVAO );
		#endif

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

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	if ( GLv_3 == GLi->Version() ) {
		glBindVertexArray( curVAO );
	}
	#endif

	if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() ) {
		if ( !mTextured ) {
			GLi->Enable( GL_TEXTURE_2D );
		}
	}
}

void cVertexBufferVBO::SetVertexStates() {
	#ifdef EE_GL3_ENABLED
	GLint index;
	#endif

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	GLint curVAO = 0;

	if ( GLv_3 == GLi->Version() ) {
		if ( mBuffersSet ) {
			return;
		}

		glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &curVAO );
		glBindVertexArray( mVAO );
	}
	#endif

	/// POSITION
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->EnableClientState( GL_VERTEX_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_POSITION ] );
		#ifdef EE_GL3_ENABLED
		if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() ) {
			index = GLi->GetRendererGL3()->GetStateIndex( EEGL_VERTEX_ARRAY );

			if ( -1 != index ) {
				glVertexAttribPointerARB( index, eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FP, GL_FALSE, 0, 0 );
			}
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
		if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() ) {
			index = GLi->GetRendererGL3()->GetStateIndex( EEGL_COLOR_ARRAY );

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

	/// TEXTURES
	if ( GLi->IsExtension( EEGL_ARB_multitexture ) ) {
		for ( Int32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
				GLi->ClientActiveTexture( GL_TEXTURE0 + i );
				GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );

				glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_TEXTURE0 + i ] );

				#ifdef EE_GL3_ENABLED
				if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() ) {
					index = GLi->GetRendererGL3()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY );

					if ( -1 != index && 0 == i )
						glVertexAttribPointerARB( index, eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FP, GL_FALSE, 0, 0 );
				}
				else
				#endif
				{
					GLi->TexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FP, 0, (char*)NULL, 0 );
				}

				mTextured = true;
			} else {
				GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
				GLi->Disable( GL_TEXTURE_2D );

				mTextured = false;

				break;
			}
		}
	} else {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
			GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_TEXTURE0 ] );

			#ifdef EE_GL3_ENABLED
			if ( GLv_3 == GLi->Version() || GLv_ES2 == GLi->Version() ) {
				index = GLi->GetRendererGL3()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY );

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

	GLi->ActiveTexture( GL_TEXTURE0 );
	GLi->ClientActiveTexture( GL_TEXTURE0 );

	glBindBufferARB( GL_ARRAY_BUFFER, 0 );

	#if !defined( EE_GLES ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	if ( GLv_3 == GLi->Version() ) {
		glBindVertexArray( curVAO );
	}
	#endif

	mBuffersSet = true;
}

void cVertexBufferVBO::Update( const Uint32& Types, bool Indices ) {
	GLenum usageType = GL_STATIC_DRAW;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW;

	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) && VERTEX_FLAG_QUERY( Types, i ) ) {
			glBindBufferARB( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferDataARB( GL_ARRAY_BUFFER, mVertexArray[i].size() * sizeof(eeFloat), &( mVertexArray[i][0] ), usageType );
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
	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->EnableClientState( GL_VERTEX_ARRAY );
	}

	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->EnableClientState( GL_COLOR_ARRAY );
	}

	if( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
		GLi->Enable( GL_TEXTURE_2D );
		GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
	}
}


}}
