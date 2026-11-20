#ifndef ETERM_UI_PROGRAMSTATUSUI_HPP
#define ETERM_UI_PROGRAMSTATUSUI_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/string.hpp>
#include <eterm/terminal/programstatusalerts.hpp>

namespace EE::UI {

class UISceneNode;

} // namespace EE::UI

namespace eterm::UI {

using EE::String;
using EE::UnorderedMap;
using EE::UI::UISceneNode;

class UITerminal;

/** Tracks status for the application's terminal widgets and shares one alert limiter. */
class ProgramStatusUI {
  public:
	explicit ProgramStatusUI( const char* applicationName );

	/** The preferences and icon name must outlive the terminal's registration. */
	void watch( UITerminal* terminal, const Terminal::ProgramStatusPreferences& preferences,
				const char* defaultIcon );

	void refresh( UITerminal* terminal );

	static void installStyles( UISceneNode* scene );

	const char* applicationName() const { return mApplicationName; }

	Terminal::ProgramStatusAlerts& alerts() { return mAlerts; }

  private:
	struct State {
		std::shared_ptr<const Terminal::ProgramStatusSnapshot> previous;
		std::shared_ptr<const Terminal::ProgramStatusSnapshot> seen;
		const Terminal::ProgramStatusPreferences* preferences;
		const char* defaultIcon;
	};

	static bool active( const UITerminal* terminal );

	static String describe( UISceneNode* scene, const Terminal::PresentedProgramStatus& status );

	void statusChanged( UITerminal* terminal );

	const char* mApplicationName;
	UnorderedMap<UITerminal*, State> mStates;
	Terminal::ProgramStatusAlerts mAlerts;
	// Widgets and displays can outlive their application-level controller during scene teardown.
	std::shared_ptr<const bool> mLifetime{ std::make_shared<const bool>( true ) };
};

} // namespace eterm::UI

#endif
