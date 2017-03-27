#ifndef EE_GRAPHICS_CIRCLEDRAWABLE_HPP
#define EE_GRAPHICS_CIRCLEDRAWABLE_HPP

#include <eepp/graphics/arcdrawable.hpp>

namespace EE { namespace Graphics {

class CircleDrawable : public ArcDrawable {
	public:
		CircleDrawable();

		CircleDrawable( const Float& radius, const Uint32& segmentsCount );
};

}}

#endif
