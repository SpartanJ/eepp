#include <eepp/graphics/texturedrawable.hpp>

namespace EE { namespace Graphics {

TextureDrawablePtr TextureDrawable::New( TexturePtr texture ) {
	return makeResource<TextureDrawable>( std::move( texture ) );
}

TextureDrawable::TextureDrawable( TexturePtr texture ) :
	DrawableResource( Drawable::TEXTUREDRAWABLE, texture ? texture->getName() : "" ),
	mTexture( std::move( texture ) ) {
	if ( mTexture ) {
		mTextureChangeConnection =
			mTexture->connectResourceChange( [this]( DrawableResource& ) { onResourceChange(); } );
	}
}

Sizef TextureDrawable::getSize() {
	return mTexture ? mTexture->getSize() : Sizef{};
}

Sizef TextureDrawable::getPixelsSize() {
	return mTexture ? mTexture->getPixelsSize() : Sizef{};
}

void TextureDrawable::draw() {
	draw( mPosition );
}

void TextureDrawable::draw( const Vector2f& position ) {
	draw( position, getPixelsSize() );
}

void TextureDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( mTexture )
		mTexture->drawEx( position.x, position.y, size.x, size.y, 0, Vector2f::One, mColor, mColor,
						  mColor, mColor );
}

bool TextureDrawable::isStateful() {
	return false;
}

DrawablePtr TextureDrawable::createInstance() const {
	TextureDrawablePtr instance = New( mTexture );
	instance->setColor( mColor );
	instance->setPosition( mPosition );
	return instance;
}

const TexturePtr& TextureDrawable::getTexture() const {
	return mTexture;
}

}} // namespace EE::Graphics
