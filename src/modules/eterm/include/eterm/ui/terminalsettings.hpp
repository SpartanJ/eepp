#ifndef ETERM_UI_TERMINALSETTINGS_HPP
#define ETERM_UI_TERMINALSETTINGS_HPP

#include <eterm/config.hpp>

#include <eepp/core/string.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace EE::UI {

class UISceneNode;

namespace Tools {

class UISettingsPanel;

} // namespace Tools

} // namespace EE::UI

namespace eterm::Terminal {

class TerminalColorScheme;
struct ProgramStatusPreferences;

} // namespace eterm::Terminal

namespace eterm::UI {

using EE::UI::UISceneNode;
using EE::UI::Tools::UISettingsPanel;

class ProgramStatusUI;
class UITerminal;
struct TerminalPreferences;

/** Registers terminal controls on the host's settings panel. The host owns the preferences. */
class EE_ETERM_API TerminalSettings {
  public:
	using VisitTerminals = std::function<void( const std::function<void( UITerminal* )>& )>;
	using Save = std::function<void()>;

	TerminalSettings( UISettingsPanel* panel, VisitTerminals visit, Save save );

	void addBehavior( TerminalPreferences& preferences );

	void addExclusiveMode( bool& enabled );

	void addShell( std::string& shell, std::string& arguments, Save applyShell = {},
				   Save applyArguments = {} );

	struct WorkingDirectoryOptions {
		// Context-specific choices precede User Home and Other.
		std::vector<EE::String> contextChoices;
		std::string path;
		std::function<std::string()> browseFrom;
		size_t selected{ 0 };
		EE::Uint32 dialogFlags{ 0 };
		bool singleClickNavigation{ false };
		bool commitPathOnFocusLoss{ false };
	};

	void addWorkingDirectory( WorkingDirectoryOptions options,
							  std::function<bool( size_t, std::string )> change );

	void addAppearance( TerminalPreferences& preferences );

	void addColorScheme( const std::map<std::string, Terminal::TerminalColorScheme>& schemes,
						 std::function<const std::string&()> get,
						 std::function<void( const std::string& )> set, bool includeDefault );

	void addFrameBuffer( bool& enabled );

	void addProgramStatus( Terminal::ProgramStatusPreferences& preferences,
						   ProgramStatusUI& status );

  private:
	struct Actions {
		VisitTerminals visit;
		Save save;
	};

	UISettingsPanel* mPanel;
	UISceneNode* mScene;
	std::shared_ptr<const Actions> mActions;
};

} // namespace eterm::UI

#endif
