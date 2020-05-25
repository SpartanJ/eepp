#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/vertexbufferogl.hpp>

namespace EE { namespace Graphics {

VertexBufferOGL::VertexBufferOGL( const Uint32& VertexFlags, PrimitiveType DrawType,
								  const Int32& ReserveVertexSize, const Int32& ReserveIndexSize,
								  VertexBufferUsageType UsageType ) :
	VertexBuffer( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ) {}

void VertexBufferOGL::bind() {
	GlobalBatchRenderer::instance()->draw();
	setVertexStates();
}

bool VertexBufferOGL::compile() {
	return true;
}

void VertexBufferOGL::draw() {
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_USE_INDICES ) ) {
		Int32 lSize = mElemDraw;

		if ( mElemDraw < 0 )
			lSize = getIndexCount();

		GLi->drawElements( mDrawType, lSize, GL_UNSIGNED_INT, &mIndexArray[0] );
	} else {
		GLi->drawArrays( mDrawType, 0, getVertexCount() );
	}
}

void VertexBufferOGL::setVertexStates() {
	Uint32 alloc = getVertexCount() * sizeof( Float ) * 2;
	Uint32 allocC = getVertexCount() * 4;

	/// TEXTURES
	if ( GLi->isExtension( EEGL_ARB_multitexture ) ) {
		for ( Int32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 + i ) ) {
				if ( mVertexArray[VERTEX_FLAG_TEXTURE0 + i].empty() )
					return;
				GLi->clientActiveTexture( GL_TEXTURE0 + i );
				GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );

				GLi->texCoordPointer( VertexElementCount[VERTEX_FLAG_TEXTURE0 + i], GL_FP,
									  sizeof( Float ) *
										  VertexElementCount[VERTEX_FLAG_TEXTURE0 + i],
									  &mVertexArray[VERTEX_FLAG_TEXTURE0 + i][0], alloc );
			} else {
				if ( 0 == i ) {
					GLi->disable( GL_TEXTURE_2D );
					GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );
					break;
				}
			}
		}
	} else {
		if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
			if ( mVertexArray[VERTEX_FLAG_TEXTURE0].empty() )
				return;
			GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
			GLi->texCoordPointer( VertexElementCount[VERTEX_FLAG_TEXTURE0], GL_FP,
								  sizeof( Float ) * VertexElementCount[VERTEX_FLAG_TEXTURE0],
								  &mVertexArray[VERTEX_FLAG_TEXTURE0][0], alloc );
		} else {
			GLi->disable( GL_TEXTURE_2D );
			GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );
		}
	}

	/// POSITION
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		if ( mVertexArray[VERTEX_FLAG_POSITION].empty() )
			return;
		GLi->enableClientState( GL_VERTEX_ARRAY );
		GLi->vertexPointer( VertexElementCount[VERTEX_FLAG_POSITION], GL_FP,
							sizeof( Float ) * VertexElementCount[VERTEX_FLAG_POSITION],
							&mVertexArray[VERTEX_FLAG_POSITION][0], alloc );
	} else {
		GLi->disableClientState( GL_VERTEX_ARRAY );
	}

	/// COLOR
	if ( VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		if ( mColorArray.empty() )
			return;
		GLi->enableClientState( GL_COLOR_ARRAY );
		GLi->colorPointer( VertexElementCount[VERTEX_FLAG_COLOR], GL_UNSIGNED_BYTE,
						   sizeof( Uint8 ) * VertexElementCount[VERTEX_FLAG_COLOR], &mColorArray[0],
						   allocC );
	} else {
		GLi->disableClientState( GL_COLOR_ARRAY );
	}

	GLi->clientActiveTexture( GL_TEXTURE0 );
}

void VertexBufferOGL::unbind() {
	if ( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_TEXTURE0 ) ) {
		GLi->enable( GL_TEXTURE_2D );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if ( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_POSITION ) ) {
		GLi->enableClientState( GL_VERTEX_ARRAY );
	}

	if ( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) ) {
		GLi->enableClientState( GL_COLOR_ARRAY );
	}
}

void VertexBufferOGL::update( const Uint32&, bool ) {}

void VertexBufferOGL::reload() {}

}} // namespace EE::Graphics
