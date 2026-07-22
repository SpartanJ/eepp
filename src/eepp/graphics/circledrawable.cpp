#include <eepp/graphics/circledrawable.hpp>

namespace EE { namespace Graphics {

CircleDrawable* CircleDrawable::New() {
	return eeNew( CircleDrawable, () );
}

CircleDrawable* CircleDrawable::New( const Float& radius, const Uint32& segmentsCount ) {
	return eeNew( CircleDrawable, ( radius, segmentsCount ) );
}

CircleDrawable::CircleDrawable() : ArcDrawable( 0, 64 ) {}

CircleDrawable::CircleDrawable( const Float& radius, const Uint32& segmentsCount ) :
	ArcDrawable( radius, segmentsCount ) {}

DrawablePtr CircleDrawable::clone() const {
	auto instance = makeResource<CircleDrawable>( mRadius, mSegmentsCount );
	instance->mArcAngle = mArcAngle;
	instance->mArcStartAngle = mArcStartAngle;
	instance->mOffset = mOffset;
	instance->mFillMode = mFillMode;
	instance->mBlendMode = mBlendMode;
	instance->mLineWidth = mLineWidth;
	instance->mSmooth = mSmooth;
	instance->mColor = mColor;
	instance->mPosition = mPosition;
	return instance;
}

}} // namespace EE::Graphics
