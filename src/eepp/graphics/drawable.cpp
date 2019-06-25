#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/statefuldrawable.hpp>
#include <eepp/core/debug.hpp>

namespace EE { namespace Graphics {

Drawable::Drawable(Type drawableType ) :
	mDrawableType( drawableType ),
	mColor( Color(255,255,255,255) )
{
}

Drawable::~Drawable()
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

void Drawable::setColor(const Color & color) {
	if ( mColor != color ) {
		mColor = color;
		onColorFilterChange();
		onAlphaChange();
	}
}

Color Drawable::getColor() const {
	return mColor;
}

void Drawable::setColorFilter( const Color& color ) {
	if ( mColor.r != color.r || mColor.g != color.g || mColor.b != color.b ) {
		mColor.r = color.r;
		mColor.g = color.g;
		mColor.b = color.b;

		onColorFilterChange();
	}
}

RGB Drawable::getColorFilter() {
	return RGB( mColor.r, mColor.g, mColor.b );
}

void Drawable::clearColor() {
	if ( mColor != Color::White ) {
		mColor = Color::White;

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

Drawable::Type Drawable::getDrawableType() const {
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

StatefulDrawable * Drawable::asStatefulDrawable() {
	eeASSERT( isStateful() );
	return static_cast<StatefulDrawable*>( this );
}

bool Drawable::isDrawableResource() const {
	return false;
}

void Drawable::onAlphaChange() {
}

void Drawable::onColorFilterChange() {
}

void Drawable::onPositionChange() {
}

}}
