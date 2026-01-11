#ifndef ECODE_WIDGETCOMMANDEXECUTER_HPP
#define ECODE_WIDGETCOMMANDEXECUTER_HPP

#include <eepp/ee.hpp>

namespace ecode {

class UISearchBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UISearchBar* New() { return eeNew( UISearchBar, () ); }

	UISearchBar() :
		UILinearLayout( "searchbar", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

class UILocateBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UILocateBar* New() { return eeNew( UILocateBar, () ); }
	UILocateBar() :
		UILinearLayout( "locatebar", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

class UIGlobalSearchBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIGlobalSearchBar* New() { return eeNew( UIGlobalSearchBar, () ); }

	UIGlobalSearchBar() :
		UILinearLayout( "globalsearchbar", UIOrientation::Vertical ),
		WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

class UIMainLayout : public UIRelativeLayout, public WidgetCommandExecuter {
  public:
	static UIMainLayout* New() { return eeNew( UIMainLayout, () ); }

	UIMainLayout() : UIRelativeLayout( "mainlayout" ), WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UIRelativeLayout::onKeyDown( event );
	}
};

class UIRelativeLayoutCommandExecuter : public UIRelativeLayout, public WidgetCommandExecuter {
  public:
	static UIRelativeLayoutCommandExecuter* New() {
		return eeNew( UIRelativeLayoutCommandExecuter, () );
	}

	UIRelativeLayoutCommandExecuter() :
		UIRelativeLayout( "rellayce" ), WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UIRelativeLayout::onKeyDown( event );
	}
};

class UIHLinearLayoutCommandExecuter : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIHLinearLayoutCommandExecuter* New() {
		return eeNew( UIHLinearLayoutCommandExecuter, () );
	}

	UIHLinearLayoutCommandExecuter() :
		UILinearLayout( "hboxce", UIOrientation::Horizontal ),
		WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

class UIVLinearLayoutCommandExecuter : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIVLinearLayoutCommandExecuter* New() {
		return eeNew( UIVLinearLayoutCommandExecuter, () );
	}

	UIVLinearLayoutCommandExecuter() :
		UILinearLayout( "vboxce", UIOrientation::Vertical ), WidgetCommandExecuter( getInput() ) {}

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event ) || UILinearLayout::onKeyDown( event );
	}
};

} // namespace ecode

#endif // ECODE_WIDGETCOMMANDEXECUTER_HPP
