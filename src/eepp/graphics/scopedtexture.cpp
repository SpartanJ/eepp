#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/scopedtexture.hpp>

namespace EE { namespace Graphics { namespace Private {

ScopedTexture::ScopedTexture( int textureBind ) :
	mTextureBinded( 0 ), mTextureToBind( textureBind ) {
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &mTextureBinded );

	if ( mTextureToBind > 0 && mTextureBinded != mTextureToBind )
		GLi->bindTexture( GL_TEXTURE_2D, mTextureToBind );
}

ScopedTexture::~ScopedTexture() {
	if ( mTextureBinded != mTextureToBind )
		GLi->bindTexture( GL_TEXTURE_2D, mTextureBinded );
}

}}} // namespace EE::Graphics::Private
