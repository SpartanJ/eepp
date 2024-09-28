#ifndef ECODE_UIWELCOMESCREEN_HPP
#define ECODE_UIWELCOMESCREEN_HPP

#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

using namespace EE;
using namespace EE::UI;

namespace ecode {

class App;

class UIWelcomeScreen : public UIRelativeLayout, public WidgetCommandExecuter {
  public:
	static UIWelcomeScreen* createWelcomeScreen( App* app, UITabWidget* inTabWidget = nullptr );

	static UIWelcomeScreen* New( App* app );

	explicit UIWelcomeScreen( App* app );

	virtual Uint32 onKeyDown( const KeyEvent& event ) {
		return WidgetCommandExecuter::onKeyDown( event );
	}

	void refresh();

	Uint32 getType() const;

	bool isType( const Uint32& type ) const;

  protected:
	App* mApp{ nullptr };
};

} // namespace ecode

#endif // ECODE_UIWELCOMESCREEN_HPP
