#ifndef ECODE_WIDGETCOMMANDEXECUTER_HPP
#define ECODE_WIDGETCOMMANDEXECUTER_HPP

#include <eepp/ee.hpp>

namespace ecode {

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

class UIMainLayout : public UIRelativeLayout, public WidgetCommandExecuter {
	public:
	static UIMainLayout* New() { return eeNew( UIMainLayout, () ); }

	UIMainLayout() :
		UIRelativeLayout( "mainlayout" ),
		WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}
};

} // namespace ecode

#endif // ECODE_WIDGETCOMMANDEXECUTER_HPP
