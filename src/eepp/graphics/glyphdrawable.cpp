#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

GlyphDrawable* GlyphDrawable::New( Texture* texture, const Rect& srcRect, const Sizef& destSize,
								   const std::string& resourceName ) {
	return eeNew( GlyphDrawable, ( texture, srcRect, destSize, resourceName ) );
}

GlyphDrawable::GlyphDrawable( Texture* texture, const Rect& srcRect, const Sizef& destSize,
							  const std::string& resourceName ) :
	DrawableResource( Drawable::GLYPH, resourceName ),
	mTexture( texture ),
	mSrcRect( srcRect.asFloat() ),
	mDestSize( destSize ),
	mAdvance( destSize.getWidth() ) {
	mPixelDensity = PixelDensity::getPixelDensity();
}

void GlyphDrawable::draw() {
	draw( mPosition );
}

void GlyphDrawable::draw( const Vector2f& position ) {
	draw( position,
		  mDestSize != Sizef::Zero ? mDestSize : Sizef( mSrcRect.Right, mSrcRect.Bottom ) );
}

void GlyphDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( position != mPosition )
		mPosition = position;

	BatchRenderer* BR = GlobalBatchRenderer::instance();
	BR->setTexture( mTexture, mTexture->getCoordinateType() );
	BR->setBlendMode( BlendMode::Alpha() );
	BR->quadsBegin();
	BR->quadsSetColor( mColor );
	BR->quadsSetTexCoord( mSrcRect.Left, mSrcRect.Top, mSrcRect.Left + mSrcRect.Right,
						  mSrcRect.Top + mSrcRect.Bottom );
	if ( mDrawMode == DrawMode::Image ) {
		BR->batchQuad( position.x, position.y, size.getWidth(), size.getHeight() );
	} else if ( mDrawMode == DrawMode::TextItalic && !mIsItalic ) {
		Float x = position.x + mGlyphOffset.x;
		Float y = position.y + mGlyphOffset.y;
		Float italic = 0.208f * size.getWidth(); // 12 degrees
		BR->batchQuadFree( x + italic, y, x, y + size.getHeight(), x + size.getWidth(),
						   y + size.getHeight(), x + size.getWidth() + italic, y );
	} else {
		BR->batchQuad( position.x + mGlyphOffset.x, position.y + mGlyphOffset.y, size.getWidth(),
					   size.getHeight() );
	}
	BR->drawOpt();
}

void GlyphDrawable::drawIntoVertexBuffer( VertexBuffer* vbo, const Vector2u& gridPos,
										  const Vector2f& pos, const Uint32& textureLevel ) {
	vbo->setQuadTexCoords( gridPos,
						   Rectf( mSrcRect.Left, mSrcRect.Top, mSrcRect.Left + mSrcRect.Right,
								  mSrcRect.Top + mSrcRect.Bottom ),
						   textureLevel );
	Sizef size( mDestSize != Sizef::Zero ? mDestSize : Sizef( mSrcRect.Right, mSrcRect.Bottom ) );
	if ( mDrawMode == DrawMode::Image ) {
		vbo->setQuad( gridPos, pos, size, mColor );
	} else if ( mDrawMode == DrawMode::TextItalic && !mIsItalic ) {
		Float x = pos.x + mGlyphOffset.x;
		Float y = pos.y + mGlyphOffset.y;
		Float italic = 0.208f * size.getWidth(); // 12 degrees
		vbo->setQuadFree( gridPos, { x + italic, y }, { x, y + size.getHeight() },
						  { x + size.getWidth(), y + size.getHeight() },
						  { x + size.getWidth() + italic, y }, mColor );
	} else {
		vbo->setQuad( gridPos, pos + mGlyphOffset, size, mColor );
	}
}

bool GlyphDrawable::isStateful() {
	return false;
}

Texture* GlyphDrawable::getTexture() {
	return mTexture;
}

const Rectf& GlyphDrawable::getSrcRect() const {
	return mSrcRect;
}

const Sizef& GlyphDrawable::getDestSize() const {
	return mDestSize;
}

Sizef GlyphDrawable::getSize() {
	if ( mDestSize != Sizef::Zero )
		return Sizef( mDestSize.getWidth() / mPixelDensity, mDestSize.getHeight() / mPixelDensity );
	return Sizef( mSrcRect.Right / mPixelDensity, mSrcRect.Bottom / mPixelDensity );
}

Sizef GlyphDrawable::getPixelsSize() {
	if ( mDestSize != Sizef::Zero )
		return mDestSize;
	return Sizef( mSrcRect.Right, mSrcRect.Bottom );
}

const Float& GlyphDrawable::getPixelDensity() const {
	return mPixelDensity;
}

void GlyphDrawable::setPixelDensity( const Float& pixelDensity ) {
	mPixelDensity = pixelDensity;
}

const Vector2f& GlyphDrawable::getGlyphOffset() const {
	return mGlyphOffset;
}

void GlyphDrawable::setGlyphOffset( const Vector2f& glyphOffset ) {
	mGlyphOffset = glyphOffset.roundDown();
}

const GlyphDrawable::DrawMode& GlyphDrawable::getDrawMode() const {
	return mDrawMode;
}

void GlyphDrawable::setDrawMode( const DrawMode& drawMode ) {
	mDrawMode = drawMode;
}

void GlyphDrawable::setIsItalic( bool isItalic ) {
	mIsItalic = isItalic;
}

const Float& GlyphDrawable::getAdvance() const {
	return mAdvance;
}

void GlyphDrawable::setAdvance( Float advance ) {
	mAdvance = advance;
}

}} // namespace EE::Graphics
