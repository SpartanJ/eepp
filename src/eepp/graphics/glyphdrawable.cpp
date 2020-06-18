#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/pixeldensity.hpp>

namespace EE { namespace Graphics {

GlyphDrawable* GlyphDrawable::New( Texture* texture, const Rect& srcRect,
								   const std::string& resourceName ) {
	return eeNew( GlyphDrawable, ( texture, srcRect, resourceName ) );
}

GlyphDrawable::GlyphDrawable( Texture* texture, const Rect& srcRect,
							  const std::string& resourceName ) :
	DrawableResource( Drawable::GLYPH, resourceName ),
	mTexture( texture ),
	mSrcRect( srcRect.asFloat() ) {
	mPixelDensity = PixelDensity::getPixelDensity();
}

void GlyphDrawable::draw() {
	draw( mPosition );
}

void GlyphDrawable::draw( const Vector2f& position ) {
	draw( position, Sizef( mSrcRect.Right, mSrcRect.Bottom ) );
}

void GlyphDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( position != mPosition )
		mPosition = position;

	BatchRenderer* BR = GlobalBatchRenderer::instance();
	BR->setTexture( mTexture, mTexture->getCoordinateType() );
	BR->setBlendMode( BlendAlpha );
	BR->quadsBegin();
	BR->quadsSetColor( mColor );
	BR->quadsSetTexCoord( mSrcRect.Left, mSrcRect.Top, mSrcRect.Left + mSrcRect.Right,
						  mSrcRect.Top + mSrcRect.Bottom );
	BR->batchQuad( position.x, position.y, size.getWidth(), size.getHeight() );
	BR->drawOpt();
}

bool GlyphDrawable::isStateful() {
	return false;
}

Texture* GlyphDrawable::getTexture() {
	return mTexture;
}

Sizef GlyphDrawable::getSize() {
	return Sizef( mSrcRect.Right / mPixelDensity, mSrcRect.Bottom / mPixelDensity );
}

const Float& GlyphDrawable::getPixelDensity() const {
	return mPixelDensity;
}

void GlyphDrawable::setPixelDensity( const Float& pixelDensity ) {
	mPixelDensity = pixelDensity;
}

}} // namespace EE::Graphics
