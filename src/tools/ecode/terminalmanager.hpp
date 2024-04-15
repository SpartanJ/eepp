#ifndef ECODE_TERMINALMANAGER_HPP
#define ECODE_TERMINALMANAGER_HPP

#include <eepp/ui/uitabwidget.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace eterm::UI;
using namespace EE::UI;

namespace ecode {

class App;

class TerminalManager {
  public:
	TerminalManager( App* app );

	UITerminal* createNewTerminal( const std::string& title = "",
								   UITabWidget* inTabWidget = nullptr,
								   const std::string& workingDir = "", std::string program = "",
								   const std::vector<std::string>& args = {} );

	void applyTerminalColorScheme( const TerminalColorScheme& colorScheme );

	void setTerminalColorScheme( const std::string& name );

	void loadTerminalColorSchemes();

	static std::map<KeyBindings::Shortcut, std::string> getTerminalKeybindings();

	const std::string& getTerminalColorSchemesPath() const;

	void setTerminalColorSchemesPath( const std::string& terminalColorSchemesPath );

	UIMenu* createColorSchemeMenu();

	void updateMenuColorScheme( UIMenuSubMenu* colorSchemeMenu );

	const std::map<std::string, TerminalColorScheme>& getTerminalColorSchemes() const {
		return mTerminalColorSchemes;
	}

	void updateColorSchemeMenu();

	bool getUseFrameBuffer() const;

	void setUseFrameBuffer( bool useFrameBuffer );

	void configureTerminalShell();

	void configureTerminalScrollback();

	const std::string& getTerminalCurrentColorScheme() { return mTerminalCurrentColorScheme; }

	void setKeybindings( UITerminal* term );

	void displayError();

	void openInExternalTerminal( const std::string& cmd );

  protected:
	App* mApp;
	std::string mTerminalColorSchemesPath;
	std::map<std::string, TerminalColorScheme> mTerminalColorSchemes;
	std::string mTerminalCurrentColorScheme;

	Float mColorSchemeMenuesCreatedWithHeight{ 0 };
	std::vector<UIPopUpMenu*> mColorSchemeMenues;
	bool mUseFrameBuffer{ false };
};

} // namespace ecode

#endif // ECODE_TERMINALMANAGER_HPP
