#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

RectangleDrawable::RectangleDrawable() :
	PrimitiveDrawable( DRAWABLE_RECTANGLE ),
	mRotation( 0 ),
	mScale( 1, 1 ),
	mCorners( 0 ),
	mUsingRectColors( false )
{
}

RectangleDrawable::RectangleDrawable(const Vector2f & position, const Sizef & size) :
	PrimitiveDrawable( DRAWABLE_RECTANGLE ),
	mSize( size ),
	mRotation( 0 ),
	mScale( 1, 1 ),
	mCorners( 0 ),
	mUsingRectColors( false )
{
	mPosition = position;
}

Sizef RectangleDrawable::getSize() {
	return mSize;
}

void RectangleDrawable::draw() {
	draw( mPosition );
}

void RectangleDrawable::draw(const Vector2f & position) {
	draw( mPosition, mSize );
}

void RectangleDrawable::draw(const Vector2f & position, const Sizef & size) {
	if ( size != mSize ) {
		mSize = size;
		mNeedsUpdate = true;
	}

	if ( mCorners == 0 ) {
		if ( mUsingRectColors ) {
			drawRectangle( Rectf( mPosition, mSize ), mRectColors.TopLeft, mRectColors.BottomLeft, mRectColors.BottomRight, mRectColors.TopRight, mRotation, mScale );
		} else {
			drawRectangle( Rectf( mPosition, mSize ), mColor, mColor, mColor, mColor, mRotation, mScale );
		}
	} else {
		PrimitiveDrawable::draw( position, size );
	}
}

Float RectangleDrawable::getRotation() const {
	return mRotation;
}

void RectangleDrawable::setRotation(const Float & rotation) {
	mRotation = rotation;
	mNeedsUpdate = true;
}

Vector2f RectangleDrawable::getScale() const {
	return mScale;
}

void RectangleDrawable::setScale(const Vector2f & scale) {
	mScale = scale;
	mNeedsUpdate = true;
}

void RectangleDrawable::setSize(const Sizef & size) {
	mSize = size;
	mNeedsUpdate = true;
}

Uint32 RectangleDrawable::getCorners() const {
	return mCorners;
}

void RectangleDrawable::setCorners(const Uint32 & corners) {
	mCorners = corners;
	mNeedsUpdate = true;
	mRecreateVertexBuffer = true;
}

RectColors RectangleDrawable::getRectColors() const {
	return mRectColors;
}

void RectangleDrawable::setRectColors(const RectColors & rectColors) {
	mRectColors = rectColors;
	mUsingRectColors = true;
}

void RectangleDrawable::drawRectangle( const Rectf& R, const Color& TopLeft, const Color& BottomLeft, const Color& BottomRight, const Color& TopRight, const Float& Angle, const Vector2f& Scale ) {
	BatchRenderer * sBR = GlobalBatchRenderer::instance();
	sBR->setTexture( NULL );
	sBR->setBlendMode( mBlendMode );

	switch( mFillMode ) {
		case DRAW_FILL:
		{
			sBR->quadsBegin();
			sBR->quadsSetColorFree( TopLeft, BottomLeft, BottomRight, TopRight );

			Sizef size = const_cast<Rectf*>(&R)->getSize();

			sBR->batchQuadEx( R.Left, R.Top, size.getWidth(), size.getHeight(), Angle, Scale );
			break;
		}
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			sBR->lineLoopBegin();
			sBR->lineLoopSetColorFree( TopLeft, BottomLeft );

			if ( Scale != 1.0f || Angle != 0.0f ) {
				Quad2f Q( R );
				Sizef size = const_cast<Rectf*>(&R)->getSize();

				Q.scale( Scale );
				Q.rotate( Angle, Vector2f( R.Left + size.getWidth() * 0.5f, R.Top + size.getHeight() * 0.5f ) );

				sBR->batchLineLoop( Q[0].x, Q[0].y, Q[1].x, Q[1].y );
				sBR->lineLoopSetColorFree( BottomRight, TopRight );
				sBR->batchLineLoop( Q[2].x, Q[2].y, Q[3].x, Q[3].y );
			} else {
				sBR->batchLineLoop( R.Left, R.Top, R.Left, R.Bottom );
				sBR->lineLoopSetColorFree( BottomRight, TopRight );
				sBR->batchLineLoop( R.Right, R.Bottom, R.Right, R.Top );
			}

			break;
		}
	}

	sBR->draw();
}

void RectangleDrawable::updateVertex() {
	if ( mCorners == 0 )
		return;

	prepareVertexBuffer( mFillMode == DRAW_LINE ? DM_LINE_LOOP : DM_POLYGON );

	unsigned int i;
	Sizef size			= mSize;
	Float xscalediff	= size.getWidth()	* mScale.x - size.getWidth();
	Float yscalediff	= size.getHeight()	* mScale.y - size.getHeight();
	Vector2f Center( mPosition.x + size.getWidth() * 0.5f + xscalediff, mPosition.y + size.getHeight() * 0.5f + yscalediff );
	Polygon2f Poly	= Polygon2f::createRoundedRectangle( mPosition.x - xscalediff, mPosition.y - yscalediff, size.getWidth() + xscalediff, size.getHeight() + yscalediff, mCorners );
	Vector2f poly;
	Poly.rotate( mRotation, Center );

	if ( !mUsingRectColors ) {
		for ( i = 0; i < Poly.getSize(); i++ ) {
			mVertexBuffer->addVertex( Poly[i] );
			mVertexBuffer->addColor( mColor );
		}
	} else {
		for ( i = 0; i < Poly.getSize(); i++ ) {
			poly = Poly[i];

			if ( poly.x <= Center.x && poly.y <= Center.y )
				mVertexBuffer->addColor( mRectColors.TopLeft );
			else if ( poly.x <= Center.x && poly.y >= Center.y )
				mVertexBuffer->addColor( mRectColors.BottomLeft );
			else if ( poly.x > Center.x && poly.y > Center.y )
				mVertexBuffer->addColor( mRectColors.BottomRight );
			else if ( poly.x > Center.x && poly.y < Center.y )
				mVertexBuffer->addColor( mRectColors.TopRight );
			else
				mVertexBuffer->addColor( mRectColors.TopLeft );

			mVertexBuffer->addVertex( poly );
		}
	}

	mNeedsUpdate = false;
}

void RectangleDrawable::onColorFilterChange() {
	PrimitiveDrawable::onColorFilterChange();
	mUsingRectColors = false;
}

}}
