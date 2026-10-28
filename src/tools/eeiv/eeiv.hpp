#pragma once

#include <eepp/ee.hpp>

#include "consolecommands.hpp"
#include "uiwelcomescreen.hpp"

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;

class App {
  public:
	struct Config {
		std::string UIFontPath;
		std::string FallbackFontPath;
		bool Fade{ true };
		bool LateLoading{ true };
		bool BlockWheelSpeed{ true };
		bool ShowInfo{ true };
		bool MaximizeAtStart{ true };
		bool VSync{ false };
		bool BenchmarkMode{ false };
		Uint32 FrameLimit{ ContextSettings::FrameRateLimitScreenRefreshRate };
		float TransitionTime{ 200 };
		int ConsoleFontSize{ 12 };
		Float AppFontSize{ 12 };
		Float UIScale{ 0 };
		FontHinting Hinting{ FontHinting::Full };
		FontAntialiasing Antialiasing{ FontAntialiasing::Grayscale };
		ColorSchemeExtPreference ColorScheme{ ColorSchemeExtPreference::System };
		GraphicsLibraryVersion RendererVersion{ GLv_default };
		Uint32 Multisamples{ 0 };
		float DefaultImageZoom{ 1 };
		Uint32 WheelBlockTime{ 200 };
	};

	static bool isImage( const std::string& path );

	static bool isHttpUrl( const std::string& path );

	App( int argc, char* argv[] );

	~App();

	void process();

	void loadDir( const std::string& path, const bool& getImages = true );

	void fastLoadImage( const Uint32& imageNum );

	void setImgScale( Float scale );

	void createSlideShow( Uint32 time );

	void setBackgroundColor( const Color& color );

	const std::string& getFileDirectoryPath() const { return mFilePath; }

	const std::string& getFile() const { return mFile; }

	std::string getFilePath() const { return mFilePath + mFile; }

	bool getSlideShow() const { return mSlideShow; }

	void setSlideShow( bool enabled ) { mSlideShow = enabled; }

	size_t getFileCount() const { return mFiles.size(); }

	EE::Window::Window* getWindow() const;

	Config& getConfig() { return mConfig; }

	void showWelcomeScreen();

	void hideWelcomeScreen();

	void openFileDialog();

	void openFolderDialog();

	void showSettings();

  private:
	bool init();

	void loadConfig();

	void saveConfig();

	void updateConfig();

	void getImages();

	void registerConsoleCommands();

	/** Registers the viewer's commands and loads their shortcuts. */
	void registerKeyBindings();

	/** Default shortcuts, including UIImageViewer's. Symbolic modifiers use the current ones. */
	KeyBindings::ShortcutMap getDefaultKeybindings() const;

	/** Load keybindings.cfg, or only the defaults, and install it. On failure the active
	 * shortcuts are kept and the reason is stored in `error`. */
	bool loadKeybindings( bool defaultsOnly = false, std::string* error = nullptr );

	/** The Keyboard Shortcuts settings page. */
	UIWidget* createKeybindingsEditor( UIWidget* parent );

	/** Rebuild the help overlay from the active shortcuts. */
	void updateHelp();

	void loadImagePath( const std::string& path, bool loadGallery = true );

	void loadImageUrl( const std::string& url );

	void syncLoadedImage();

	void updateSlideShow();

	void toggleHelp();

	void fitImage();

	void resetImageZoom();

	Sprite* getImageSprite() const;

	void processDroppedFiles();

	UIWindow* createSettingsWindow();

	void openFontPicker( bool fallbackFont = false );

	void setUIFontSize( Float size );

	void setFallbackFont( FontPtr font );

	void setShowInfo( bool show );

	void setTextureFilter( Texture::Filter filter );

	/** Whether keyboard focus is outside dialogs, where viewer shortcuts apply. */
	bool isViewerFocused() const;

	std::string mInitialPath;
	Config mConfig;
	IniFile mIni;
	std::string mStorePath;
	std::string mTmpPath;
	std::string mFilePath;
	std::string mFile;
	std::vector<std::string> mFiles;
	std::vector<std::string> mPathsToLoad;
	Int32 mCurImg{ 0 };
	bool mSlideShow{ false };
	Uint32 mSlideTime{ 4000 };
	Uint32 mSlideTicks{ 0 };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::unique_ptr<UIApplication> mUIApplication;
	UIWidget* mMainLayout{ nullptr };
	UIImageViewer* mImageViewer{ nullptr };
	UITextView* mHelp{ nullptr };
	UIConsole* mConsole{ nullptr };
	UIWelcomeScreen* mWelcomeScreen{ nullptr };
	UIWindow* mSettingsWindow{ nullptr };
	bool mCursorVisible{ true };
	Texture::Filter mTextureFilter{ Texture::Filter::Nearest };
	std::unique_ptr<ConsoleCommands> mConsoleCommands;
};
