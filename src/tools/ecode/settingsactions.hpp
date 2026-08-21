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

	void setTabOutChars();

	void setFoldRefreshFreq();

	void setUIScaleFactor();

	void setUIFontSize();
	void setUIFontSize( const StyleSheetLength& size );

	void setEditorFontSize();
	void setEditorFontSize( const StyleSheetLength& size );

	void setTerminalFontSize();
	void setTerminalFontSize( const StyleSheetLength& size );

	void setUIPanelFontSize();

	void setScreenshotSavePath();

	void setScreenshotFilenamePattern();

	void setScreenshotSaveFormat();

  private:
	App* mApp{ nullptr };

	String i18n( const std::string& key, const String& def );

	void checkForUpdatesResponse( Http::Response&& response, bool fromStartup );
};

} // namespace ecode

#endif
