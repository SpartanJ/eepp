#ifndef EE_UI_UIAPPLICATION
#define EE_UI_UIAPPLICATION

#include <eepp/window/window.hpp>

#include <optional>

using namespace EE::Window;

namespace EE { namespace UI {

class UISceneNode;

class EE_API UIApplication {
  public:
	struct Settings {
		Settings() {}

		Settings( std::optional<Float> pixelDensity, bool loadBaseResources = true,
				  Font* baseFont = nullptr, std::optional<std::string> baseStyleSheetPath = {}, Font* emojiFont = nullptr );

		//! Not setting anything will automatically try to detect the main screen pixel density
		std::optional<Float> pixelDensity;
		//! Must be set to true in order to initialize the basic UI resources (font and UI theme).
		//! Otherwise it will initialize with an empty UI scene node
		bool loadBaseResources{ true };
		//! The default base font for the UI. If not provided it will load NotoSans-Regular ( will
		//! look at "assets/fonts/NotoSans-Regular.ttf" )
		Font* baseFont{ nullptr };
		//! The style sheet path is the path of the base UI theme stylesheet ( will look at
		//! "assets/ui/breeze.css" by default )
		std::optional<std::string> baseStyleSheetPath;
		//! The default emoji font for the UI. If not provided it will load NotoEmoji-Regular ( will
		//! look at "assets/fonts/NotoEmoji-Regular.ttf" )
		Font* emojiFont{ nullptr };
	};

	UIApplication( const WindowSettings& windowSettings, const Settings& appSettings = Settings(),
				   const ContextSettings& contextSettings = ContextSettings() );

	//! All resources allocated by the library will be safetely released
	virtual ~UIApplication();

	//! The main window
	EE::Window::Window* getWindow() const;

	//! The UI scene node, this node handles the whole UI. This is the equivalent to the HTML DOM
	//! Document
	UISceneNode* getUI() const;

	int run();

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	EE::Window::Window* mWindow{ nullptr };
	bool mDidRun{ false };
};

}} // namespace EE::UI

#endif
