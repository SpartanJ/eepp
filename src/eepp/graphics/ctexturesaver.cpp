#include <eepp/graphics/ctexturesaver.hpp>

namespace EE { namespace Graphics { namespace Private {

cTextureSaver::cTextureSaver( GLint textureBind ) :
	mTextureBinded( 0 ),
	mTextureToBind( textureBind )
{
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &mTextureBinded );

	if ( mTextureToBind > 0 && mTextureBinded != mTextureToBind )
		glBindTexture( GL_TEXTURE_2D, mTextureToBind );
}

cTextureSaver::~cTextureSaver() {
	if ( mTextureBinded != mTextureToBind )
		glBindTexture( GL_TEXTURE_2D, mTextureBinded );
}

}}} 
