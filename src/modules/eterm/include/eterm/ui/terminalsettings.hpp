#ifndef ETERM_UI_TERMINALSETTINGS_HPP
#define ETERM_UI_TERMINALSETTINGS_HPP

#include <eepp/core/string.hpp>
#include <functional>
#include <map>
#include <memory>

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
class TerminalSettings {
  public:
	using VisitTerminals = std::function<void( const std::function<void( UITerminal* )>& )>;
	using Save = std::function<void()>;

	TerminalSettings( UISettingsPanel* panel, VisitTerminals visit, Save save );

	void addBehavior( TerminalPreferences& preferences );

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
