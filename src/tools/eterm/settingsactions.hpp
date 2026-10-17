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

	/** UI font (when not null) and size in dp, for new and existing text. */
	void applyUIFont( Font* font, Float sizeDp );

	/** Applies the hinting and anti-aliasing settings to every font, live. */
	void applyFontRendering();

  private:
	App* mApp{ nullptr };
	UIWindow* mSettingsWindow{ nullptr };
};

} // namespace eterm
