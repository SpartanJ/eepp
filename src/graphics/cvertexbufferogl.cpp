#include "cvertexbufferogl.hpp"
#include "glhelper.hpp"

namespace EE { namespace Graphics {

cVertexBufferOGL::cVertexBufferOGL( const Uint32& VertexFlags, EE_DRAW_MODE DrawType, const Int32& ReserveVertexSize, const Int32& ReserveIndexSize, EE_VBO_USAGE_TYPE UsageType ) :
	cVertexBuffer( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType )
{
}

cVertexBufferOGL::~cVertexBufferOGL() {
}

void cVertexBufferOGL::Bind() {
	SetVertexStates();
}

bool cVertexBufferOGL::Compile() {
	return true;
}

void cVertexBufferOGL::Draw() {
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		Int32 lSize = mElemDraw;

		if( mElemDraw < 0 )
			lSize = GetIndexCount();

		glDrawElements( mDrawType, lSize, GL_UNSIGNED_INT, &mIndexArray[0] );
	} else {
		glDrawArrays( mDrawType, 0, GetVertexCount() );
	}
}

void cVertexBufferOGL::SetVertexStates() {
	/// POSITION
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( eeVertexElements[ VERTEX_FLAG_POSITION ], GL_FLOAT, sizeof(float) * eeVertexElements[ VERTEX_FLAG_POSITION ], &mVertexArray[ VERTEX_FLAG_POSITION ][0] );
	} else {
		//glDisableClientState( GL_VERTEX_ARRAY );
	}

	/// COLOR
	if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		glEnableClientState( GL_COLOR_ARRAY );
		glColorPointer( eeVertexElements[ VERTEX_FLAG_COLOR ], GL_UNSIGNED_BYTE, sizeof(Uint8) * eeVertexElements[ VERTEX_FLAG_COLOR ], &mColorArray[0] );
	} else {
		//glDisableClientState( GL_COLOR_ARRAY );
	}

	/// TEXTURES
	if ( GLi->IsExtension( EEGL_ARB_multitexture ) ) {
		for ( Int32 i = 0; i < 5; i++ ) {
			if( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
				glClientActiveTextureARB( GL_TEXTURE0_ARB + i );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );

				glTexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], GL_FLOAT, sizeof(float) * eeVertexElements[ VERTEX_FLAG_TEXTURE0 + i ], &mVertexArray[ VERTEX_FLAG_TEXTURE0 + i ][0] );
			} else {
				//glDisableClientState( GL_TEXTURE_COORD_ARRAY );
				glDisable( GL_TEXTURE_2D );
			}
		}
	} else {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer( eeVertexElements[ VERTEX_FLAG_TEXTURE0 ], GL_FLOAT, sizeof(float) * eeVertexElements[ VERTEX_FLAG_TEXTURE0 ], &mVertexArray[ VERTEX_FLAG_TEXTURE0 ][0] );
		} else {
			//glDisableClientState( GL_TEXTURE_COORD_ARRAY );
			glDisable( GL_TEXTURE_2D );
		}
	}

	glActiveTextureARB( GL_TEXTURE0_ARB );
	glClientActiveTextureARB( GL_TEXTURE0_ARB );
}

void cVertexBufferOGL::Update( const Uint32& Types, bool Indices ) {
}

void cVertexBufferOGL::Reload() {
}

}}
