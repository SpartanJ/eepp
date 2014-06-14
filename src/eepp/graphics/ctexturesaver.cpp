#include <eepp/graphics/ctexturesaver.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Graphics { namespace Private {

cTextureSaver::cTextureSaver( int textureBind ) :
	mTextureBinded( 0 ),
	mTextureToBind( textureBind )
{
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &mTextureBinded );

	if ( mTextureToBind > 0 && mTextureBinded != mTextureToBind )
		GLi->BindTexture( GL_TEXTURE_2D, mTextureToBind );
}

cTextureSaver::~cTextureSaver() {
	if ( mTextureBinded != mTextureToBind )
		GLi->BindTexture( GL_TEXTURE_2D, mTextureBinded );
}

}}} 
