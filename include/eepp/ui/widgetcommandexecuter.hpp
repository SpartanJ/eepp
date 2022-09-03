#ifndef EE_UI_WIDGETCOMMANDEXECUTER_HPP
#define EE_UI_WIDGETCOMMANDEXECUTER_HPP

#include <eepp/scene/keyevent.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <functional>
#include <memory>

using namespace EE::Scene;

namespace EE { namespace UI {

class EE_API WidgetCommandExecuter {
  public:
	typedef std::function<void()> CommandCallback;

	WidgetCommandExecuter( const KeyBindings& keybindings ) : mKeyBindings( keybindings ) {}

	void addCommand( const std::string& name, const CommandCallback& cb ) { mCommands[name] = cb; }

	void execute( const std::string& command ) {
		auto cmdIt = mCommands.find( command );
		if ( cmdIt != mCommands.end() )
			cmdIt->second();
	}

	KeyBindings& getKeyBindings() { return mKeyBindings; }

  protected:
	KeyBindings mKeyBindings;
	std::unordered_map<std::string, std::function<void()>> mCommands;

	Uint32 onKeyDown( const KeyEvent& event ) {
		std::string cmd =
			mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
		if ( !cmd.empty() ) {
			auto cmdIt = mCommands.find( cmd );
			if ( cmdIt != mCommands.end() ) {
				cmdIt->second();
				return 1;
			}
		}
		return 0;
	}
};

}} // namespace EE::UI

#endif // EE_UI_WIDGETCOMMANDEXECUTER_HPP
