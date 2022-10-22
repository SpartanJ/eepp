#ifndef EE_GRAPHICS_CIRCLEDRAWABLE_HPP
#define EE_GRAPHICS_CIRCLEDRAWABLE_HPP

#include <eepp/graphics/arcdrawable.hpp>

namespace EE { namespace Graphics {

class EE_API CircleDrawable : public ArcDrawable {
  public:
	static CircleDrawable* New();

	static CircleDrawable* New( const Float& radius, const Uint32& segmentsCount );

	CircleDrawable();

	CircleDrawable( const Float& radius, const Uint32& segmentsCount );
};

}} // namespace EE::Graphics

#endif
