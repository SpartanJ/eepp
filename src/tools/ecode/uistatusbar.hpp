#ifndef ECODE_UISTATUSBAR_HPP
#define ECODE_UISTATUSBAR_HPP

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

using namespace EE;
using namespace EE::UI;

namespace ecode {

class App;

class UIStatusBar : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static UIStatusBar* New();

	UIStatusBar();

	void updateState();

  protected:
	friend class App;
	virtual Uint32 onMessage( const NodeMessage* msg );

	void setApp( App* app );

	App* mApp{ nullptr };

	virtual void onVisibilityChange();

	virtual void onChildCountChange( Node* child, const bool& removed );
};

} // namespace ecode

#endif // ECODE_UISTATUSBAR_HPP
