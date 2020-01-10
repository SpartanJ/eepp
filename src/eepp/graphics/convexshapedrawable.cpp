#include <eepp/graphics/convexshapedrawable.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

ConvexShapeDrawable* ConvexShapeDrawable::New() {
	return eeNew( ConvexShapeDrawable, () );
}

ConvexShapeDrawable::ConvexShapeDrawable() : PrimitiveDrawable( Drawable::CONVEXSHAPE ) {}

Sizef ConvexShapeDrawable::getSize() {
	return mPolygon.getBounds().getSize();
}

void ConvexShapeDrawable::draw() {
	draw( mPosition, getSize() );
}

void ConvexShapeDrawable::draw( const Vector2f& position ) {
	draw( position, getSize() );
}

void ConvexShapeDrawable::draw( const Vector2f& position, const Sizef& size ) {
	PrimitiveDrawable::draw( position, size );
}

void ConvexShapeDrawable::setPolygon( const Polygon2f& polygon ) {
	mPolygon = polygon;
	mNeedsUpdate = true;
}

void ConvexShapeDrawable::addPoint( const Vector2f& point ) {
	mPolygon.pushBack( point );
	mNeedsUpdate = true;
}

void ConvexShapeDrawable::resetPoints() {
	mPolygon.clear();
	mNeedsUpdate = true;
}

void ConvexShapeDrawable::updateVertex() {
	prepareVertexBuffer( mFillMode == DRAW_LINE ? PRIMITIVE_LINE_LOOP : PRIMITIVE_TRIANGLE_FAN );

	if ( mPolygon.getSize() == 0 )
		return;

	switch ( mFillMode ) {
		case DRAW_LINE: {
			for ( Uint32 i = 0; i < mPolygon.getSize(); i++ ) {
				mVertexBuffer->addVertex( mPosition + mPolygon[i] );
				mVertexBuffer->addColor( mColor );
			}

			break;
		}
		case DRAW_FILL: {
			mVertexBuffer->addVertex( mPosition + mPolygon.getBounds().getCenter() );
			mVertexBuffer->addColor( mColor );

			for ( Uint32 i = 0; i < mPolygon.getSize(); i++ ) {
				mVertexBuffer->addVertex( mPosition + mPolygon[i] );
				mVertexBuffer->addColor( mColor );
			}

			mVertexBuffer->addVertex( mPosition + mPolygon[0] );
			mVertexBuffer->addColor( mColor );

			break;
		}
	}

	mNeedsUpdate = false;
}

}} // namespace EE::Graphics
