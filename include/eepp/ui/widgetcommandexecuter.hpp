#ifndef EE_UI_WIDGETCOMMANDEXECUTER_HPP
#define EE_UI_WIDGETCOMMANDEXECUTER_HPP

#include <algorithm>
#include <functional>
#include <span>

#include <eepp/scene/keyevent.hpp>
#include <eepp/ui/keyboardshortcut.hpp>

using namespace EE::Scene;

namespace EE { namespace UI {

class EE_API WidgetCommandExecuter {
  public:
	typedef std::function<void()> CommandCallback;

	WidgetCommandExecuter( const KeyBindings& keybindings ) : mKeyBindings( keybindings ) {}

	void setCommand( const std::string& name, const CommandCallback& cb ) {
		auto cmdIt = mCommands.find( name );
		if ( cmdIt == mCommands.end() ) {
			mCommands[name] = cb;
			mCommandList.emplace_back( name );
		} else {
			cmdIt->second = cb;
		}
	}

	void unsetCommand( const std::string& name ) {
		mCommands.erase( name );
		auto it = std::find( mCommandList.begin(), mCommandList.end(), name );
		if ( it != mCommandList.end() )
			mCommandList.erase( it );
	}

	void unsetCommands( const std::span<std::string>& names ) {
		for ( const auto& name : names ) {
			mCommands.erase( name );
			auto it = std::find( mCommandList.begin(), mCommandList.end(), name );
			if ( it != mCommandList.end() )
				mCommandList.erase( it );
		}
	}

	bool hasCommand( const std::string& name ) const {
		return mCommands.find( name ) != mCommands.end();
	}

	bool execute( const std::string& command ) {
		auto cmdIt = mCommands.find( command );
		if ( cmdIt != mCommands.end() ) {
			cmdIt->second();
			return true;
		}
		return false;
	}

	size_t commandCount() const { return mCommands.size(); }

	KeyBindings& getKeyBindings() { return mKeyBindings; }

	const std::vector<std::string>& getCommandList() const { return mCommandList; }

  protected:
	KeyBindings mKeyBindings;
	std::unordered_map<std::string, std::function<void()>> mCommands;
	std::vector<std::string> mCommandList;

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
