#pragma once

#include <eepp/ee.hpp>

using namespace EE;
using namespace EE::UI;

namespace eterm {

class App;

class SettingsActions {
  public:
	explicit SettingsActions( App* app );

	void showSettings();

	void openFontPicker( bool uiFont, bool fallbackFont = false );

	void setUIFontSize( Float size );

	void setTerminalFontSize( Float size );

  private:
	App* mApp{ nullptr };
	UIWindow* mSettingsWindow{ nullptr };
};

} // namespace eterm
