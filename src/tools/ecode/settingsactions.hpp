#ifndef SETTINGSACTIONS_HPP
#define SETTINGSACTIONS_HPP

#include <eepp/ee.hpp>

namespace ecode {

class App;

class SettingsActions {
  public:
	explicit SettingsActions( App* app ) : mApp( app ) {}

	void checkForUpdates( bool fromStartup = false );

	void aboutEcode();

	void ecodeSource();

	void setLineBreakingColumn();

	void setLineSpacing();

	void setCursorBlinkingTime();

	void setIndentTabCharacter();

	void setFoldRefreshFreq();

	void setUIScaleFactor();

	void setUIFontSize();

	void setEditorFontSize();

	void setTerminalFontSize();

	void setUIPanelFontSize();

  private:
	App* mApp{ nullptr };

	String i18n( const std::string& key, const String& def );

	void checkForUpdatesResponse( Http::Response&& response, bool fromStartup );

};

} // namespace ecode

#endif
