#ifndef EEFM_SETTINGSACTIONS_HPP
#define EEFM_SETTINGSACTIONS_HPP

#include <eepp/graphics/font.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace eefm {

using namespace EE;
using namespace EE::Graphics;
using namespace EE::UI;

class App;

/** @brief The settings window and the actions that apply the preferences live. */
class SettingsActions {
  public:
	explicit SettingsActions( App& app );

	void showSettings();

	void savePreferences();

	/** Applies the hinting and anti-aliasing settings to every font, live. */
	void applyFontRendering();

	/** UI font (when not null) and size, for new and existing text. */
	void applyUIFont( Font* font, Float sizeDp );

	/** Applies the Files settings to every open tab. */
	void applyFilesConfig();

  private:
	/** Font picker for the UI font, the fallback font or the terminal font. */
	enum class FontTarget { UI, Fallback, Terminal };

	void openFontPicker( FontTarget target );

	App& mApp;
	UISceneNode* mUI;
	UIWindow* mSettingsWindow{ nullptr };
};

} // namespace eefm

#endif
