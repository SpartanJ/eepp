#ifndef EE_GRAPHICS_DRAWABLEMANAGER_HPP
#define EE_GRAPHICS_DRAWABLEMANAGER_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

class DrawableSearcher {
	public:
		static Drawable * searchByName( const std::string& name );

		static Drawable * searchById( const Uint32 & id );
};

}}

#endif
