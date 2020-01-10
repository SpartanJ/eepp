#ifndef EE_GRAPHICS_DRAWABLEMANAGER_HPP
#define EE_GRAPHICS_DRAWABLEMANAGER_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>

namespace EE { namespace Graphics {

class EE_API DrawableSearcher {
  public:
	static Drawable* searchByName( const std::string& name );

	static Drawable* searchById( const Uint32& id );

	static void setPrintWarnings( const bool& print );

	static bool getPrintWarnings();

  protected:
	static bool sPrintWarnings;
};

}} // namespace EE::Graphics

#endif
