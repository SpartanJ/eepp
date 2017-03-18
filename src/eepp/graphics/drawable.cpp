#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

Drawable::Drawable() :
	mColorFilter(Color::White),
	mAlpha(255)
{}

void Drawable::setAlpha( Uint8 alpha ) {
	if ( mAlpha != alpha ) {
		mAlpha = alpha;

		onAlphaChange();
	}
}

const Uint8& Drawable::getAlpha() {
	return mAlpha;
}

void Drawable::setColorFilter( Color color ) {
	if ( mColorFilter != color ) {
		mColorFilter = color;

		onColorFilterChange();
	}
}

const Color& Drawable::getColorFilter() {
	return mColorFilter;
}

void Drawable::clearColorFilter() {
	if ( mColorFilter != Color::White ) {
		mColorFilter = Color::White;

		onColorFilterChange();
	}
}

void Drawable::resetAlpha() {
	if ( mAlpha != 255 ) {
		mAlpha = 255;

		onAlphaChange();
	}
}

ColorA Drawable::getColorFilterAlpha() {
	return ColorA( mColorFilter.r, mColorFilter.g, mColorFilter.b, mAlpha );
}

void Drawable::onAlphaChange() {
}

void Drawable::onColorFilterChange() {
}

}}
