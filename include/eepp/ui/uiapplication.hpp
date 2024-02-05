#ifndef EE_UI_UIAPPLICATION
#define EE_UI_UIAPPLICATION

#include <eepp/window/window.hpp>
using namespace EE::Window;

namespace EE { namespace UI {

class UISceneNode;

class UIApplication {
  public:
	UIApplication( const WindowSettings& windowSettings, bool loadBaseResources = true,
				   const ContextSettings& contextSettings = ContextSettings() );

	virtual ~UIApplication();

	EE::Window::Window* getWindow() const;

	UISceneNode* getUI() const;

	int run();

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	EE::Window::Window* mWindow{ nullptr };
	bool mDidRun{ false };
};

}} // namespace EE::UI

#endif
