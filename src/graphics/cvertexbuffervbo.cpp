#include "cvertexbuffervbo.hpp"

namespace EE { namespace Graphics {

cVertexBufferVBO::cVertexBufferVBO( const Uint32& VertexFlags, EE_DRAW_TYPE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) :
	cVertexBuffer( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ),
	mCompiled( false ),
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

	GLenum usageType = GL_STATIC_DRAW_ARB;
	if( mUsageType== VBO_USAGE_TYPE_DYNAMIC ) usageType = GL_DYNAMIC_DRAW_ARB;
	else if( mUsageType== VBO_USAGE_TYPE_STREAM ) usageType = GL_STREAM_DRAW_ARB;

	//Create the VBO vertex arrays
	for( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
			glGenBuffersARB( 1,(GLuint *)&mArrayHandle[ i ] );

			glBindBufferARB( GL_ARRAY_BUFFER_ARB, mArrayHandle[i] );

			if ( mArrayHandle[i] ) {
				if ( i != VERTEX_FLAG_COLOR )
					glBufferDataARB( GL_ARRAY_BUFFER_ARB, mVertexArray[i].size() * sizeof(eeFloat), &( mVertexArray[i][0] ), usageType );
				else
					glBufferDataARB( GL_ARRAY_BUFFER_ARB, mColorArray.size(), &mColorArray[0], usageType );
			} else {
				return false;
			}

			glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
		}
	}

	//Create the VBO index array
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		glGenBuffersARB( 1, (GLuint *)&mElementHandle );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, mElementHandle );

		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GetIndexCount() * sizeof(Uint32), &mIndexArray[0], usageType );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	}

	mCompiled = true;

	return true;
}

void cVertexBufferVBO::Draw() {
	if ( !mCompiled )
		return;

	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		Int32 lSize = mElemDraw;

		if( mElemDraw < 0 )
			lSize = GetIndexCount();

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, mElementHandle );

		glDrawElements( mDrawType, lSize, GL_UNSIGNED_INT, (char*)NULL );

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	} else {
		glDrawArrays( mDrawType, 0, GetVertexCount() );
	}
}

void cVertexBufferVBO::SetVertexStates() {
	/// POSITION
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		glEnableClientState( GL_VERTEX_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, mArrayHandle[ VERTEX_FLAG_POSITION ] );
		glVertexPointer( eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FLOAT, 0, (char*)NULL );
	} else {
		glDisableClientState( GL_VERTEX_ARRAY );
	}

	/// COLOR
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		glEnableClientState( GL_COLOR_ARRAY );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, mArrayHandle[ VERTEX_FLAG_COLOR ] );
		glColorPointer( eeVertexElements[ VERTEX_FLAG_COLOR ], GL_UNSIGNED_BYTE, 0, (char*)NULL );
	} else {
		glDisableClientState( GL_COLOR_ARRAY );
	}

	/// TEXTURES
	for ( Int32 i = 0; i < 5; i++ ) {
		if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
			glClientActiveTextureARB( GL_TEXTURE0_ARB );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, mArrayHandle[ VERTEX_FLAG_TEXTURE0 + i ] );
			glTexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FLOAT, 0, (char*)NULL );
		} else {
			//glClientActiveTextureARB( GL_TEXTURE0_ARB );
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
			glDisable( GL_TEXTURE_2D );
		}
	}

	glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
}

}}
