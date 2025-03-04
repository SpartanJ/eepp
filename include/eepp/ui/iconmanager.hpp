#ifndef EE_UI_ICONMANAGER_HPP
#define EE_UI_ICONMANAGER_HPP

#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/ui/uiscenenode.hpp>

using namespace EE;
using namespace EE::Graphics;

namespace EE { namespace UI {

class UIIconTheme;

class IconManager {
  public:
	static UIIconTheme* init( const std::string& iconThemeName, FontTrueType* remixIconFont,
							  FontTrueType* noniconFont, FontTrueType* codIconFont );
};

}} // namespace EE::UI

#endif // EE_UI_ICONMANAGER_HPP
