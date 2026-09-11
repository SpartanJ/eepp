#ifndef EE_UI_UIAPPLICATION
#define EE_UI_UIAPPLICATION

#include <eepp/graphics/font.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/window/window.hpp>

#include <memory>
#include <optional>
#include <vector>

using namespace EE::Window;

namespace EE { namespace System {
class ThreadPool;
}} // namespace EE::System

namespace EE { namespace UI {

class UISceneNode;

namespace Private {
class UIApplicationSystemFontState;
}

class EE_API UIApplication {
  public:
	/** Controls when UIApplication stops its main loop.
	 * @see setQuitPolicy() */
	enum class QuitPolicy : Uint8 {
		/** Stops after the last application-owned window closes. */
		OnLastWindowClosed,
		/** Stops when the primary window closes and closes all secondary windows. */
		OnPrimaryWindowClosed,
		/** Window closure never stops the loop; requestQuit() must be called explicitly. */
		Explicit
	};

	struct EE_API Settings {
		Settings() {}

		Settings( std::optional<std::string> basePath, std::optional<Float> pixelDensity = {},
				  bool loadBaseResources = true, Font* baseFont = nullptr,
				  std::optional<std::string> baseStyleSheetPath = {}, Font* emojiFont = nullptr,
				  Font* fallbackFont = nullptr,
				  std::shared_ptr<System::ThreadPool> threadPool = nullptr );

		//! By default it will use the current process path as the base path. This will set the
		//! default working directory.
		std::optional<std::string> basePath;
		//! Not setting anything will automatically try to detect the main screen pixel density
		std::optional<Float> pixelDensity;
		//! Must be set to true in order to initialize the basic UI resources (font and UI theme).
		//! Otherwise it will initialize with an empty UI scene node
		bool loadBaseResources{ true };
		//! Loads the bundled icon fonts and initializes IconManager when base resources are
		//! enabled.
		bool loadIconResources{ true };
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
		//! The hinting policy applied to fonts owned by the default and UI resource scopes.
		FontHinting fontHinting{ FontHinting::Full };
		//! The antialiasing policy applied to fonts owned by the default and UI resource scopes.
		FontAntialiasing fontAntialiasing{ FontAntialiasing::Grayscale };
		//! Enables system font fallback and warms the system font list on a background thread. If
		//! not set, UIApplication::systemFontsEnabledByDefault() is used.
		std::optional<bool> enableSystemFonts;
		//! Optional worker pool shared by the primary and secondary UI scenes.
		std::shared_ptr<System::ThreadPool> threadPool;
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

	/** Creates a secondary application-owned native window and its UISceneNode. The new scene
	 * inherits the primary scene's font, theme, icon theme, stylesheet, and rendering policies.
	 * The returned scene remains owned by UIApplication.
	 * @return The new UI scene, or nullptr if native window creation failed. */
	UISceneNode* createWindow( const WindowSettings& windowSettings,
							   const ContextSettings& contextSettings = ContextSettings() );

	/** Returns the application-owned UI scene associated with @p window, or nullptr when the
	 * window is not owned by this application. */
	UISceneNode* getUI( EE::Window::Window* window ) const;

	/** Returns the number of application-owned windows pending or participating in the loop. */
	size_t getWindowCount() const;

	/** Requests destruction of an application-owned window. The native window is hidden
	 * immediately and its scene and backend resources are destroyed at the safe point at the end
	 * of the current frame. */
	void closeWindow( EE::Window::Window* window );

	/** Requests termination of the application loop without changing the open state of its
	 * windows. Backend resources are released by UIApplication destruction. */
	void requestQuit();

	/** Returns whether the application main loop is currently running. */
	bool isRunning() const;

	/** Sets the condition under which window closure stops the application loop. The default is
	 * QuitPolicy::OnPrimaryWindowClosed. */
	void setQuitPolicy( QuitPolicy policy );

	/** Returns the current window-close policy. */
	QuitPolicy getQuitPolicy() const;

	//! Runs the application until window is closed
	//! @return EXIT_SUCCESS if application run successfully
	int run();

	//! Set if the application must show the memory manager result after closing the main window.
	void setShowMemoryManagerResult( bool show );

	bool showMemoryManagerResult() const;

	//! Controls the system-font fallback policy used by Settings::enableSystemFonts when unset.
	//! Enabled by default. Test runners can disable it before constructing any UIApplication.
	static void setSystemFontsEnabledByDefault( bool enabled );

	static bool systemFontsEnabledByDefault();

	String::HashType getStyleSheetDefaultMarker() const { return mStyleSheetMarker; }

  protected:
	/** Associates an application-owned native window with its UI scene and deferred-destruction
	 * state. */
	struct WindowEntry {
		EE::Window::Window* window{ nullptr };
		UISceneNode* ui{ nullptr };
		bool primary{ false };
		bool pendingDestroy{ false };
	};

	/** Shared primary/secondary window creation implementation. */
	WindowEntry* createWindowInternal( const WindowSettings& windowSettings,
									   const ContextSettings& contextSettings, bool primary );

	/** Applies application-level resources and rendering policies to a newly created UI scene. */
	void configureUIScene( UISceneNode* ui );

	/** Processes one application frame across all open windows. */
	void tick();

	/** Marks windows closed externally for destruction and applies the active quit policy. */
	void processClosedWindows();

	/** Destroys scenes and native windows previously marked for deferred destruction. */
	void processPendingWindowDestruction();

	UISceneNode* mUISceneNode{ nullptr };
	EE::Window::Window* mWindow{ nullptr };
	String::HashType mStyleSheetMarker{ 0 };
	bool mDidRun{ false };
	bool mShowMemoryManagerResult{ false };
	bool mRunning{ false };
	QuitPolicy mQuitPolicy{ QuitPolicy::OnPrimaryWindowClosed };
	std::vector<WindowEntry> mWindows;
	Settings mSettings;
	System::Clock mFrameClock;
	std::unique_ptr<Private::UIApplicationSystemFontState> mSystemFontState;
};

}} // namespace EE::UI

#endif
