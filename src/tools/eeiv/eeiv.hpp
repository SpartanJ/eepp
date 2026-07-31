#pragma once

#include <eepp/ee.hpp>

#include "consolecommands.hpp"

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;

class App {
  public:
	struct Config {
		bool Fade{ true };
		bool LateLoading{ true };
		bool BlockWheelSpeed{ true };
		bool ShowInfo{ true };
		bool MaximizeAtStart{ true };
		bool VSync{ false };
		Uint32 FrameLimit{ ContextSettings::FrameRateLimitScreenRefreshRate };
		float TransitionTime{ 200 };
		int ConsoleFontSize{ 12 };
		int AppFontSize{ 12 };
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

  private:
	bool init();
	void loadConfig();
	void saveConfig();
	void updateConfig();
	void getImages();
	void registerConsoleCommands();
	void registerKeyBindings();
	void loadImagePath( const std::string& path, bool loadGallery = true );
	void loadImageUrl( const std::string& url );
	void syncLoadedImage();
	void updateSlideShow();
	void toggleHelp();
	void fitImage();
	Sprite* getImageSprite() const;

	std::string mInitialPath;
	Config mConfig;
	IniFile mIni;
	std::string mStorePath;
	std::string mTmpPath;
	std::string mFilePath;
	std::string mFile;
	std::vector<std::string> mFiles;
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
	bool mCursorVisible{ true };
	Texture::Filter mTextureFilter{ Texture::Filter::Linear };
	std::unique_ptr<ConsoleCommands> mConsoleCommands;
};
