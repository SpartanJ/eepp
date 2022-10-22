#ifndef EE_GRAPHICS_STATEFULDRAWABLE_HPP
#define EE_GRAPHICS_STATEFULDRAWABLE_HPP

#include <eepp/graphics/drawableresource.hpp>

namespace EE { namespace Graphics {

class EE_API StatefulDrawable : public DrawableResource {
  public:
	StatefulDrawable( Type drawableType ) : DrawableResource( drawableType ) {}

	StatefulDrawable( Type drawableType, const std::string& name ) :
		DrawableResource( drawableType, name ) {}

	virtual StatefulDrawable* setState( Uint32 state ) = 0;

	virtual const Uint32& getState() const = 0;
};

}} // namespace EE::Graphics

#endif
