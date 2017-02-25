#include <eepp/graphics/pixeldensity.hpp>

namespace EE { namespace Graphics {

Float PixelDensity::sPixelDensity = 1.f;

const Float& PixelDensity::getPixelDensity() {
	return sPixelDensity;
}

void PixelDensity::setPixelDensity( const Float& pixelDensity ) {
	sPixelDensity = pixelDensity;
}

void PixelDensity::setPixelDensity( const EE_PIXEL_DENSITY& pixelDensity ) {
	sPixelDensity = toFloat( pixelDensity );
}

Float PixelDensity::dpToPx( Float dp ) {
	return dp * getPixelDensity();
}

Int32 PixelDensity::dpToPxI( Float dp ) {
	return (Int32)dpToPx( dp );
}

Float PixelDensity::pxToDp( Float px ) {
	return px / getPixelDensity();
}

Int32 PixelDensity::pxToDpI( Float px ) {
	return (Int32)pxToDp( px );
}

Sizei PixelDensity::dpToPxI( Sizei size ) {
	return Sizei( dpToPxI( size.x ), dpToPxI( size.y ) );
}

Sizei PixelDensity::pxToDpI( Sizei size ) {
	return Sizei( pxToDpI( size.x ), pxToDpI( size.y ) );
}

Recti PixelDensity::dpToPxI( Recti rect ) {
	return rect * getPixelDensity();
}

Recti PixelDensity::pxToDpI( Recti rect ) {
	return rect / getPixelDensity();
}

Rectf PixelDensity::dpToPx( Rectf rect ) {
	return rect * getPixelDensity();
}

Rectf PixelDensity::pxToDp( Rectf rect ) {
	return rect / getPixelDensity();
}

Sizef PixelDensity::dpToPx( Sizef size ) {
	return size * getPixelDensity();
}

Sizef PixelDensity::pxToDp( Sizef size ) {
	return size * getPixelDensity();
}

Sizei PixelDensity::dpToPxI( Sizef size ) {
	return Sizei( dpToPxI( size.x ), dpToPxI( size.y ) );
}

Sizei PixelDensity::pxToDpI( Sizef size ) {
	return Sizei( pxToDpI( size.x ), pxToDpI( size.y ) );
}

Vector2i PixelDensity::dpToPxI( Vector2i pos ) {
	return Sizei( dpToPxI( pos.x ), dpToPxI( pos.y ) );
}

Vector2i PixelDensity::pxToDpI( Vector2i pos ) {
	return Sizei( pxToDpI( pos.x ), pxToDpI( pos.y ) );
}

Vector2f PixelDensity::dpToPx( Vector2f pos ) {
	return Vector2f( dpToPx( pos.x ), dpToPx( pos.y ) );
}

Vector2f PixelDensity::pxToDp( Vector2f pos ) {
	return Vector2f( pxToDp( pos.x ), pxToDp( pos.y ) );
}

}}
