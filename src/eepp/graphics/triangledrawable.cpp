#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/triangledrawable.hpp>

namespace EE { namespace Graphics {

TriangleDrawable* TriangleDrawable::New() {
	return eeNew( TriangleDrawable, () );
}

TriangleDrawable* TriangleDrawable::New( const Vector2f& position, const Sizef& size ) {
	return eeNew( TriangleDrawable, ( position, size ) );
}

TriangleDrawable::TriangleDrawable() : PrimitiveDrawable( Drawable::TRIANGLE ) {}

TriangleDrawable::TriangleDrawable( const Vector2f& position, const Sizef& size ) :
	PrimitiveDrawable( Drawable::TRIANGLE ), mSize( size ), mCustomColors( false ) {
	mPosition = position;
}

Sizef TriangleDrawable::getSize() {
	return mTriangle.getSize();
}

void TriangleDrawable::draw() {
	draw( mPosition );
}

void TriangleDrawable::draw( const Vector2f& ) {
	draw( mPosition, mSize );
}

void TriangleDrawable::draw( const Vector2f& position, const Sizef& size ) {
	setSize( size );

	if ( position != mPosition ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( mNeedsUpdate )
		updateVertex();

	Primitives p;
	p.setFillMode( mFillMode );

	if ( mCustomColors ) {
		p.drawTriangle( mComputedTriangle, mColors[0], mColors[1], mColors[2] );
	} else {
		p.drawTriangle( mComputedTriangle, mColor, mColor, mColor );
	}
}

void TriangleDrawable::setSize( const Sizef& size ) {
	if ( mSize != size ) {
		mSize = size;
		mNeedsUpdate = true;
	}
}

const Triangle2f& TriangleDrawable::getTriangle() const {
	return mTriangle;
}

void TriangleDrawable::setTriangle( const Triangle2f& triangle ) {
	mTriangle = triangle;
	mNeedsUpdate = true;
}

void TriangleDrawable::setTriangleColors( const Color& color1, const Color& color2,
										  const Color& color3 ) {
	mCustomColors = true;
	mColors[0] = color1;
	mColors[1] = color2;
	mColors[2] = color3;
}

void TriangleDrawable::onColorFilterChange() {
	PrimitiveDrawable::onColorFilterChange();
	mCustomColors = false;
}

void TriangleDrawable::updateVertex() {
	Sizef size( mTriangle.getSize() );

	if ( size != Sizef::Zero ) {
		Vector2f scale( mSize / size );
		for ( size_t i = 0; i < 3; i++ ) {
			mComputedTriangle.V[i] = mPosition + mTriangle.V[i] * scale;
		}
	}
}

}} // namespace EE::Graphics
