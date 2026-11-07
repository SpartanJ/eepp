#pragma once

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

namespace eterm {

using namespace EE::UI;

class UIMainLayout : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIMainLayout* New() { return eeNew( UIMainLayout, () ); }

	UIMainLayout() : UILinearLayout(), WidgetCommandExecuter( getInput() ) {}

	Uint32 onKeyDown( const KeyEvent& event ) override {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

} // namespace eterm
