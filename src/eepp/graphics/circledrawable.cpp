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

}} // namespace EE::Graphics
