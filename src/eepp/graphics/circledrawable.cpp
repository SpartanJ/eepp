#include <eepp/graphics/circledrawable.hpp>

namespace EE { namespace Graphics {

CircleDrawable::CircleDrawable() :
	ArcDrawable( 0, 64 )
{
}

CircleDrawable::CircleDrawable( const Float& radius, const Uint32& segmentsCount ) :
	ArcDrawable( radius, segmentsCount )
{}

}}
