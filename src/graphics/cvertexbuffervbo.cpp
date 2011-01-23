#include "cvertexbuffervbo.hpp"
#include "glhelper.hpp"
#include "renderer/crenderergl3.hpp"

namespace EE { namespace Graphics {

cVertexBufferVBO::cVertexBufferVBO( const Uint32& VertexFlags, EE_DRAW_MODE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) :
	cVertexBuffer( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ),
	mCompiled( false ),
	mVAO( 0 ),
	mElementHandle( 0 )
{
}

cVertexBufferVBO::~cVertexBufferVBO() {
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			glDeleteBuffers( 1,(GLuint *)&mArrayHandle[ i ] );
		}
	}

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		glDeleteBuffers( 1, (GLuint *)&mElementHandle );
	}

	if ( GLv_3 == GLi->Version() ) {
		glDeleteVertexArrays( 1, &mVAO );
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

	if ( GLv_3 == GLi->Version() ) {
		glGenVertexArrays( 1, &mVAO );
		glBindVertexArray( mVAO );
	}

	GLenum usageType = GL_STATIC_DRAW;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW;

	//Create the VBO vertex arrays
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			glGenBuffers( 1,(GLuint *)&mArrayHandle[ i ] );

			glBindBuffer( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferData( GL_ARRAY_BUFFER, mVertexArray[i].size() * sizeof(eeFloat), &( mVertexArray[i][0] ), usageType );
				else
					glBufferData( GL_ARRAY_BUFFER, mColorArray.size(), &mColorArray[0], usageType );
			} else {
				return false;
			}
		}
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	//Create the VBO index array
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		glGenBuffers( 1, (GLuint *)&mElementHandle );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferData( GL_ELEMENT_ARRAY_BUFFER, GetIndexCount() * sizeof(Uint32), &mIndexArray[0], usageType );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}

	mCompiled = true;

	return true;
}

void cVertexBufferVBO::Draw() {
	if ( !mCompiled )
		return;

	if ( GLv_3 == GLi->Version() ) {
		glBindVertexArray( mVAO );
	}

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		Int32 lSize = mElemDraw;

		if( mElemDraw < 0 )
			lSize = GetIndexCount();

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glDrawElements( mDrawType, lSize, GL_UNSIGNED_INT, (char*)NULL );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	} else {
		glDrawArrays( mDrawType, 0, GetVertexCount() );
	}
}

void cVertexBufferVBO::SetVertexStates() {
	GLint index;

	if ( GLv_3 == GLi->Version() ) {
		glBindVertexArray( mVAO );
	}

	/// POSITION
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->EnableClientState( GL_VERTEX_ARRAY );
		glBindBuffer( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_POSITION ] );

		if ( GLv_3 == GLi->Version() ) {
			index = GLi->GetRendererGL3()->GetStateIndex( EEGL_VERTEX_ARRAY );

			if ( -1 != index )
				glVertexAttribPointer( index, eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FLOAT, GL_FALSE, 0, 0 );
		} else {
			glVertexPointer( eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FLOAT, 0, (char*)NULL );
		}
	} else {
		if ( GLv_3 == GLi->Version() ) {
			GLi->DisableClientState( GL_VERTEX_ARRAY );
		}
	}

	/// COLOR
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->EnableClientState( GL_COLOR_ARRAY );
		glBindBuffer( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_COLOR ] );

		if ( GLv_3 == GLi->Version() ) {
			index = GLi->GetRendererGL3()->GetStateIndex( EEGL_COLOR_ARRAY );

			if ( -1 != index )
				glVertexAttribPointer( index, eeVertexElements[ VERTEX_FLAG_COLOR ], GL_UNSIGNED_BYTE, GL_TRUE, 0, 0 );
		} else {
			glColorPointer( eeVertexElements[ VERTEX_FLAG_COLOR ], GL_UNSIGNED_BYTE, 0, (char*)NULL );
		}
	} else {
		if ( GLv_3 == GLi->Version() ) {
			GLi->DisableClientState( GL_COLOR_ARRAY );
		}
	}

	/// TEXTURES
	if ( GLi->IsExtension( EEGL_ARB_multitexture ) ) {
		for ( Int32 i = 0; i < 5; i++ ) {
			if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
				if ( GLv_3 != GLi->Version() ) {
					glClientActiveTexture( GL_TEXTURE0 + i );
				}

				GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
				glBindBuffer( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_TEXTURE0 + i ] );

				if ( GLv_3 == GLi->Version() ) { /** FIXME: Give Support for multitexturing */
					index = GLi->GetRendererGL3()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY );

					if ( -1 != index && 0 == i )
						glVertexAttribPointer( index, eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FLOAT, GL_FALSE, 0, 0 );
				} else {
					glTexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FLOAT, 0, (char*)NULL );
				}
			} else {
				if ( GLv_3 == GLi->Version() ) {
					GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
				}

				GLi->Disable( GL_TEXTURE_2D );
			}
		}
	} else {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glBindBuffer( GL_ARRAY_BUFFER, mArrayHandle[ VERTEX_FLAG_TEXTURE0 ] );

				if ( GLv_3 == GLi->Version() ) {
					index = GLi->GetRendererGL3()->GetStateIndex( EEGL_TEXTURE_COORD_ARRAY );

					if ( -1 != index ) // Give Support for multitexturing
						glVertexAttribPointer( index, eeVertexElements[ VERTEX_FLAG_TEXTURE0 ], GL_FLOAT, GL_FALSE, 0, 0 );
				} else {
					glTexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 ], GL_FLOAT, 0, (char*)NULL );
				}
		} else {
			if ( GLv_3 == GLi->Version() ) {
				GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
			}

			GLi->Disable( GL_TEXTURE_2D );
		}
	}

	GLi->ActiveTexture( GL_TEXTURE0 );

	if ( GLv_3 != GLi->Version() ) {
		glClientActiveTexture( GL_TEXTURE0 );
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void cVertexBufferVBO::Update( const Uint32& Types, bool Indices ) {
	GLenum usageType = GL_STATIC_DRAW;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW;

	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) && VERTEX_FLAG_QUERY( Types, i ) ) {
			glBindBuffer( GL_ARRAY_BUFFER, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferData( GL_ARRAY_BUFFER, mVertexArray[i].size() * sizeof(eeFloat), &( mVertexArray[i][0] ), usageType );
				else
					glBufferData( GL_ARRAY_BUFFER, mColorArray.size(), &mColorArray[0], usageType );
			}
		}
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) && Indices ) {
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mElementHandle );

		glBufferData( GL_ELEMENT_ARRAY_BUFFER, GetIndexCount() * sizeof(Uint32), &mIndexArray[0], usageType );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}
}

void cVertexBufferVBO::Reload() {
	mCompiled = false;
	Compile();
}

}}
