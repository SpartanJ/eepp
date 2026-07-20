#ifndef EE_GRAPHICS_DRAWABLEMANAGER_HPP
#define EE_GRAPHICS_DRAWABLEMANAGER_HPP

#include <eepp/core/core.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/network/uri.hpp>

namespace EE { namespace Graphics {

class ResourceScope;

class EE_API DrawableSearcher {
  public:
	static DrawablePtr searchByName( const std::string& name, bool firstSearchSprite = false,
								   Network::URI referer = "",
								   ResourceScope* resourceScope = nullptr );

	static DrawablePtr searchById( const Uint32& id );

	static void setPrintWarnings( const bool& print );

	static bool getPrintWarnings();

  protected:
	static bool sPrintWarnings;
};

}} // namespace EE::Graphics

#endif
