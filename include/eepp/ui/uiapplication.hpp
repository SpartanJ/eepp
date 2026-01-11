#ifndef EE_UI_UIAPPLICATION
#define EE_UI_UIAPPLICATION

#include <eepp/window/window.hpp>

#include <optional>

using namespace EE::Window;

namespace EE { namespace UI {

class UISceneNode;

class EE_API UIApplication {
  public:
	struct EE_API Settings {
		Settings() {}

		Settings( std::optional<std::string> basePath, std::optional<Float> pixelDensity = {},
				  bool loadBaseResources = true, Font* baseFont = nullptr,
				  std::optional<std::string> baseStyleSheetPath = {}, Font* emojiFont = nullptr,
				  Font* fallbackFont = nullptr );

		//! By default it will use the current process path as the base path. This will set the
		//! default working directory.
		std::optional<std::string> basePath;
		//! Not setting anything will automatically try to detect the main screen pixel density
		std::optional<Float> pixelDensity;
		//! Must be set to true in order to initialize the basic UI resources (font and UI theme).
		//! Otherwise it will initialize with an empty UI scene node
		bool loadBaseResources{ true };
		//! The default base font for the UI. If not provided it will load NotoSans-Regular ( will
		//! look at "assets/fonts/NotoSans-Regular.ttf" )
		Font* baseFont{ nullptr };
		//! The default base monospace font for the UI. If not provided it will load DejaVuSansMono
		//! ( will look at "assets/fonts/DejaVuSansMono.ttf" )
		Font* monospaceFont{ nullptr };
		//! The style sheet path is the path of the base UI theme stylesheet ( will look at
		//! "assets/ui/breeze.css" by default )
		std::optional<std::string> baseStyleSheetPath;
		//! The default emoji font for the UI. If not provided it will load Noto Color Emoji ( it
		//! will look at "assets/fonts/NotoColorEmoji.ttf" ) otherwise it will try NotoEmoji-Regular
		//! ( it will look at "assets/fonts/NotoEmoji-Regular.ttf" )
		Font* emojiFont{ nullptr };
		//! The default fallback font for the UI. If not provided it will load Droid Sans Fallback
		//! Full ( it will look at "assets/fonts/DroidSansFallbackFull.ttf" )
		Font* fallbackFont{ nullptr };
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

	//! Runs the application until window is closed
	//! @return EXIT_SUCCESS if application run successfully
	int run();

	//! Set if the application must show the memory manager result after closing the main window.
	void setShowMemoryManagerResult( bool show );
	bool showMemoryManagerResult() const;

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	EE::Window::Window* mWindow{ nullptr };
	bool mDidRun{ false };
	bool mShowMemoryManagerResult{ false };
};

}} // namespace EE::UI

#endif
