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

	void setCommand( const std::string& name, const CommandCallback& cb ) { mCommands[name] = cb; }

	bool hasCommand( const std::string& name ) const {
		return mCommands.find( name ) != mCommands.end();
	}

	void execute( const std::string& command ) {
		auto cmdIt = mCommands.find( command );
		if ( cmdIt != mCommands.end() )
			cmdIt->second();
	}

	size_t commandCount() const { return mCommands.size(); }

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
