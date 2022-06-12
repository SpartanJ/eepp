#ifndef ECODE_WIDGETCOMMANDEXECUTER_HPP
#define ECODE_WIDGETCOMMANDEXECUTER_HPP

#include <eepp/ee.hpp>

namespace ecode {

class WidgetCommandExecuter {
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

class UISearchBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UISearchBar* New() { return eeNew( UISearchBar, () ); }

	UISearchBar() :
		UILinearLayout( "searchbar", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

class UILocateBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UILocateBar* New() { return eeNew( UILocateBar, () ); }
	UILocateBar() :
		UILinearLayout( "locatebar", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

class UIGlobalSearchBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIGlobalSearchBar* New() { return eeNew( UIGlobalSearchBar, () ); }

	UIGlobalSearchBar() :
		UILinearLayout( "globalsearchbar", UIOrientation::Vertical ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

} // namespace ecode

#endif // ECODE_WIDGETCOMMANDEXECUTER_HPP
