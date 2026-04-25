#ifndef EE_UI_UILAYOUTERMANAGER_HPP
#define EE_UI_UILAYOUTERMANAGER_HPP

#include <eepp/config.hpp>
#include <eepp/ui/csslayouttypes.hpp>

namespace EE { namespace UI {

class UILayouter;
class UIWidget;

class EE_API UILayouterManager {
  public:
	static UILayouter* create( CSSDisplay display, UIWidget* container );
};

}} // namespace EE::UI

#endif
