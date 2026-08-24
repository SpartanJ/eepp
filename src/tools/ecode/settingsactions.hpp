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

	void setUIFontSize( const StyleSheetLength& size );

	void setEditorFontSize( const StyleSheetLength& size );

	void setTerminalFontSize( const StyleSheetLength& size );

	void setUIPanelFontSize( const StyleSheetLength& size );

	void setScreenshotSavePath();

  private:
	App* mApp{ nullptr };

	String i18n( const std::string& key, const String& def );

	void checkForUpdatesResponse( Http::Response&& response, bool fromStartup );
};

} // namespace ecode

#endif
