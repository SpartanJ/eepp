#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

Drawable::Drawable(EE_DRAWABLE_TYPE drawableType) :
	mDrawableType( drawableType ),
	mColor( Color::White )
{}

void Drawable::setAlpha( Uint8 alpha ) {
	if ( mColor.a != alpha ) {
		mColor.a = alpha;

		onAlphaChange();
	}
}

const Uint8& Drawable::getAlpha() {
	return mColor.a;
}

void Drawable::setColor(const ColorA & color) {
	mColor = color;
	onColorFilterChange();
	onAlphaChange();
}

const ColorA& Drawable::getColor() const {
	return mColor;
}

void Drawable::setColorFilter( const ColorA& color ) {
	if ( mColor.r != color.r || mColor.g != color.g || mColor.b != color.b ) {
		mColor.r = color.r;
		mColor.g = color.g;
		mColor.b = color.b;

		onColorFilterChange();
	}
}

Color Drawable::getColorFilter() {
	return Color( mColor.r, mColor.g, mColor.b );
}

void Drawable::clearColor() {
	if ( mColor != ColorA::White ) {
		mColor = ColorA::White;

		onColorFilterChange();
		onAlphaChange();
	}
}

void Drawable::clearColorFilter() {
	if ( mColor.r != 255 || mColor.g != 255 || mColor.b != 255 ) {
		mColor.r = mColor.g = mColor.b = 255;

		onColorFilterChange();
	}
}

void Drawable::resetAlpha() {
	if ( mColor.a != 255 ) {
		mColor.a = 255;

		onAlphaChange();
	}
}

EE_DRAWABLE_TYPE Drawable::getDrawableType() const {
	return mDrawableType;
}

const Vector2f& Drawable::getPosition() const {
	return mPosition;
}

void Drawable::setPosition( const Vector2f& position ) {
	if ( position != mPosition ) {
		mPosition = position;
		onPositionChange();
	}
}

void Drawable::onAlphaChange() {
}

void Drawable::onColorFilterChange() {
}

void Drawable::onPositionChange() {
}

}}
