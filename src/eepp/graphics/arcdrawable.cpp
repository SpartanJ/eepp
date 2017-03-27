#include <eepp/graphics/arcdrawable.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>

namespace EE { namespace Graphics {

ArcDrawable::ArcDrawable( const Float & radius, Uint32 segmentsCount, const Float & arcAngle, const Float & arcStartAngle ) :
	PrimitiveDrawable( DRAWABLE_CUSTOM ),
	mRadius( radius ),
	mSegmentsCount( segmentsCount ),
	mArcAngle( arcAngle ),
	mArcStartAngle( arcStartAngle )
{
}

Sizef ArcDrawable::getSize() {
	return Sizef( mRadius * 2, mRadius * 2 );
}

void ArcDrawable::draw( const Vector2f& position ) {
	draw( position, getSize() );
}

void ArcDrawable::draw( const Vector2f& position, const Sizef& size ) {
	BatchRenderer * sBR = GlobalBatchRenderer::instance();

	if ( size.getWidth() * 0.5f != mRadius ) {
		mRadius = size.getWidth() * 0.5f;
	}

	if(mSegmentsCount < 6) mSegmentsCount = 6;
	mSegmentsCount = mSegmentsCount > 360 ? 360 : mSegmentsCount;

	Float angleShift =  360 / static_cast<Float>(mSegmentsCount);
	Float arcAngleA = mArcAngle > 360 ? mArcAngle - 360 * std::floor( mArcAngle / 360 ) : mArcAngle;

	sBR->setTexture( NULL );

	switch( mFillMode ) {
		case DRAW_LINE:
		{
			sBR->setLineWidth( mLineWidth );

			mSegmentsCount = Uint32( (Float)mSegmentsCount * (Float)eeabs( arcAngleA ) / 360 );
			Float startAngle = Math::radians(mArcStartAngle);
			Float theta = Math::radians(arcAngleA) / Float(mSegmentsCount - 1);
			Float tangetialFactor = eetan(theta);
			Float radialFactor = eecos(theta);
			Float x = mRadius * eecos(startAngle);
			Float y = mRadius * eesin(startAngle);

			sBR->lineStripBegin();
			sBR->lineStripSetColor( mColor );

			for( Uint32 ii = 0; ii < mSegmentsCount; ii++ ) {
				sBR->batchLineStrip(x + position.x, y + position.y);

				Float tx = -y;
				Float ty = x;

				x += tx * tangetialFactor;
				y += ty * tangetialFactor;

				x *= radialFactor;
				y *= radialFactor;
			}

			break;
		}
		case DRAW_FILL:
		{
			sBR->triangleFanBegin();
			sBR->triangleFanSetColor( mColor );

			for( Float i = 0; i < arcAngleA; i+= angleShift ) {
				Float startAngle = mArcStartAngle + i;

				sBR->batchTriangleFan( position.x , position.y,
									   position.x + mRadius * Math::sinAng( startAngle ), position.y + mRadius * Math::cosAng( startAngle ),
									   position.x + mRadius * Math::sinAng( startAngle + angleShift ), position.y + mRadius * Math::cosAng( startAngle + angleShift ) );
			}

			break;
		}
	}

	sBR->draw();
}

}}
