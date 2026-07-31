#ifndef CAPP_H
#define CAPP_H

#include <eepp/ee.hpp>

#include "consolecommands.hpp"

class App {
  public:
	struct ImageData {
		std::string Path;
		std::vector<TexturePtr> Tex;
		Float animFps{ 60 };
	};

	struct Config {
		Uint32 Width;
		Uint32 Height;
		Uint8 BitColor;
		bool Windowed;
		bool Resizeable;
		bool VSync;
		bool DoubleBuffering;
		bool UseDesktopResolution;
		bool NoFrame;
		bool MaximizeAtStart;
		bool Fade;
		bool LateLoading;
		bool BlockWheelSpeed;
		bool ShowInfo;
		Uint32 FrameLimit;
		float TransitionTime;
		int ConsoleFontSize;
		int AppFontSize;
		float DefaultImageZoom;
		Uint32 WheelBlockTime;
	};

	static bool isRawImage( const std::string& path );

	static bool isImage( const std::string& _path );

	static bool isHttpUrl( const std::string& path );

	App( int argc, char* argv[] );

	~App();

	void process();

	void loadDir( const std::string& path, const bool& getimages = true );

	const std::string& getFileDirectoryPath() const { return mFilePath; }

	const std::string& getFile() const { return mFile; }

	std::string getFilePath() const { return mFilePath + mFile; }

	bool getSlideShow() const { return mSlideShow; }

	void setSlideShow( bool set ) { mSlideShow = set; }

	void createSlideShow( Uint32 time );

	void fastLoadImage( const Uint32& ImgNum );

	void setImgScale( Float scale );

	const std::vector<ImageData> getFiles() const { return mFiles; }

	EE::Window::Window* getWindow() const { return mWindow; }

	Config& getConfig() { return mConfig; }

  protected:
	void updateConfig();
	bool init();
	void input();
	void render();
	void end();
	void prepareFrame();
	void scaleToScreen( const bool& force = false );
	void getImages();
	Uint32 curImagePos( const std::string& path );
	std::pair<std::vector<TexturePtr>, Float> loadImage( const std::string& path,
														 const bool& SetAsCurrent = false );
	void loadNextImage();
	void loadPrevImage();
	void zoomImage();
	void unloadImage( const Uint32& img );
	void updateImages();
	void setImage( const std::vector<TexturePtr>& Tex, const std::string& path,
				   Float animFps = 60 );
	void doFade();
	void createFade();
	void optUpdate();
	void printHelp();
	void loadFirstImage();
	void loadLastImage();
	void loadConfigValues();
	void loadConfig();
	void saveConfig();
	void clearTempDir();
	void videoResize();
	void restoreMouse();
	void setWindowCaption();
	void switchFade();
	void doSlideShow();
	void disableSlideShow();

	Config mConfig;

	Engine* EE;
	TextureFactory* TF;
	System::Log* Log;
	EE::Window::Window* mWindow{ nullptr };
	EE::Window::Input* KM;

	std::string MyPath;

	Font* Fon; //! Default App Font
	Font* Mon; //! Console App Font
	Text FonCache;

	FontTrueTypePtr TTF, TTFMon;

	UIConsole* Con; //! Console Instance

	Vector2i Mouse; //! Mouse Position on Screen
	double ET;		//! Elapsed Time Between Frames
	double RET;		//! Relative Elapsed Time ( skip time in Input() )

	Float Width, Height;   //! Width and Height of the Window
	Float HWidth, HHeight; //! Half Width and Height of the Window

	std::string SLASH; //! Default SLASH string

	std::vector<ImageData> mFiles; //! Directory image file list

	Int32 mLastTicks, mZoomTicks;
	std::string mInfo; //! Window caption string

	std::string mFilePath; //! Directory path of files
	std::string mFile;	   //! Last used file name

	Int32 mCurImg; //! Current Image Selected from mFile
	Sprite mImg, mOldImg;

	bool mFading;
	Float mAlpha;
	Uint8 mCurAlpha;

	bool mLaterLoad;
	Int32 mLastLaterTick;

	Clock TEP;

	bool mCursor;

	bool mMouseLeftPressing;
	Vector2i mMouseLeftStartClick, mMouseLeftClick;

	bool mMouseMiddlePressing;
	Vector2i mMouseMiddleStartClick, mMouseMiddleClick;

	RenderMode mImgRT;
	Texture::Filter mFilter;

	Int32 mLastWheelUse;
	bool mShowHelp;

	bool mFirstLoad;

	std::string mStorePath;
	std::string mTmpPath;
	bool mUsedTempDir;
	bool mLockZoomAndPosition;

	Clock TE;

	Text* mHelpCache;

	bool mSlideShow;
	Uint32 mSlideTime;
	Uint32 mSlideTicks;

	IniFile Ini;

	Image::FormatConfiguration formatConfiguration;
	std::unique_ptr<ConsoleCommands> mConCmds;
};

#endif
