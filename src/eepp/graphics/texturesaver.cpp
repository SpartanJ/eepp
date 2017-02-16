#include <eepp/graphics/texturesaver.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics { namespace Private {

TextureSaver::TextureSaver( int textureBind ) :
	mTextureBinded( 0 ),
	mTextureToBind( textureBind )
{
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &mTextureBinded );

	if ( mTextureToBind > 0 && mTextureBinded != mTextureToBind )
		GLi->bindTexture( GL_TEXTURE_2D, mTextureToBind );
}

TextureSaver::~TextureSaver() {
	if ( mTextureBinded != mTextureToBind )
		GLi->bindTexture( GL_TEXTURE_2D, mTextureBinded );
}

}}} 
