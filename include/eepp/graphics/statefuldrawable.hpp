#ifndef EE_GRAPHICS_STATEFULDRAWABLE_HPP
#define EE_GRAPHICS_STATEFULDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

class EE_API StatefulDrawable : public Drawable {
	public:
		StatefulDrawable( Type drawableType ) : Drawable( drawableType ) {}

		virtual StatefulDrawable * setState( int state ) = 0;

		virtual const int& getState() const = 0;
};

}}

#endif
