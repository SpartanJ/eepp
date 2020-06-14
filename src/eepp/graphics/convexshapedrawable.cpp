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

void ConvexShapeDrawable::addPoint( const Vector2f& point, const Color& color ) {
	mPolygon.pushBack( point );
	mIndexColor.push_back( color );
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
				if ( mIndexColor.empty() ) {
					mVertexBuffer->addColor( mColor );
				} else {
					if ( mColor.a == 255 ) {
						mVertexBuffer->addColor( mIndexColor[i & mIndexColor.size()] );
					} else {
						mVertexBuffer->addColor(
							Color( mIndexColor[i & mIndexColor.size()] ).blendAlpha( mColor.a ) );
					}
				}
			}

			break;
		}
		case DRAW_FILL: {
			mVertexBuffer->addVertex( mPosition + mPolygon.getBounds().getCenter() );
			if ( mIndexColor.empty() ) {
				mVertexBuffer->addColor( mColor );
			} else {
				mVertexBuffer->addColor( Color( mIndexColor[0] ).blendAlpha( mColor.a ) );
			}

			for ( Uint32 i = 0; i < mPolygon.getSize(); i++ ) {
				mVertexBuffer->addVertex( mPosition + mPolygon[i] );
				if ( mIndexColor.empty() ) {
					mVertexBuffer->addColor( mColor );
				} else {
					if ( mColor.a == 255 ) {
						mVertexBuffer->addColor( mIndexColor[i & mIndexColor.size()] );
					} else {
						mVertexBuffer->addColor(
							Color( mIndexColor[i & mIndexColor.size()] ).blendAlpha( mColor.a ) );
					}
				}
			}

			mVertexBuffer->addVertex( mPosition + mPolygon[0] );
			if ( mIndexColor.empty() ) {
				mVertexBuffer->addColor( mColor );
			} else {
				mVertexBuffer->addColor(
					Color( mIndexColor[mIndexColor.size() - 1] ).blendAlpha( mColor.a ) );
			}

			break;
		}
	}

	mNeedsUpdate = false;
}

}} // namespace EE::Graphics
