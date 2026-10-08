#pragma once

#include <eepp/ui/uirelativelayout.hpp>

using namespace EE;
using namespace EE::UI;

class App;

class UIWelcomeScreen : public UIRelativeLayout {
  public:
	static UIWelcomeScreen* New( App* app );

	explicit UIWelcomeScreen( App* app );

  protected:
	App* mApp{ nullptr };
};
