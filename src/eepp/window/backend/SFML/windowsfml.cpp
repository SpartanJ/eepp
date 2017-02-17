#include <eepp/config.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#if defined( EE_X11_PLATFORM )
	#include <X11/Xlib.h>
#endif

#undef None

#endif

#include <eepp/window/backend/SFML/windowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#include <SFML/Graphics.hpp>

#include <eepp/window/backend/SFML/clipboardsfml.hpp>
#include <eepp/window/backend/SFML/inputsfml.hpp>
#include <eepp/window/backend/SFML/cursormanagersfml.hpp>
#include <eepp/window/platform/platformimpl.hpp>

#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

WindowSFML::WindowSFML( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardSFML, ( this ) ), eeNew( InputSFML, ( this ) ), eeNew( CursorManagerSFML, ( this ) ) ),
	mWinHandler( 0 ),
	mVisible( false )
{
	create( Settings, Context );
}

WindowSFML::~WindowSFML() {
}

bool WindowSFML::create( WindowSettings Settings, ContextSettings Context ) {
	if ( mWindow.Created )
		return false;

	sf::VideoMode mode			= sf::VideoMode::getDesktopMode();
	mWindow.WindowConfig		= Settings;
	mWindow.ContextConfig		= Context;
	mWindow.DesktopResolution	= Sizei( mode.width, mode.height );

	if ( mWindow.WindowConfig.Style & WindowStyle::Titlebar )
		mWindow.Flags |= sf::Style::Titlebar;

	if ( mWindow.WindowConfig.Style & WindowStyle::Resize )
		mWindow.Flags |= sf::Style::Resize;

	if ( mWindow.WindowConfig.Style & WindowStyle::NoBorder )
		mWindow.Flags = sf::Style::None;

	if ( mWindow.WindowConfig.Style & WindowStyle::UseDesktopResolution ) {
		mWindow.WindowConfig.Width	= mode.width;
		mWindow.WindowConfig.Height	= mode.height;
	}

	Uint32 TmpFlags = mWindow.Flags;

	if ( mWindow.WindowConfig.Style & WindowStyle::Fullscreen )
		TmpFlags |= sf::Style::Fullscreen;

	mSFMLWindow.create( sf::VideoMode( Settings.Width, Settings.Height, Settings.BitsPerPixel ), mWindow.WindowConfig.Caption, TmpFlags, sf::ContextSettings( Context.DepthBufferSize, Context.StencilBufferSize ) );

	mSFMLWindow.setVerticalSyncEnabled( Context.VSync );

	if ( NULL == cGL::existsSingleton() ) {
		cGL::createSingleton( mWindow.ContextConfig.Version );
		cGL::instance()->init();
	}

	createPlatform();

	getMainContext();

	createView();

	setup2D();

	mWindow.Created = true;
	mVisible = true;

	if ( "" != mWindow.WindowConfig.Icon ) {
		setIcon( mWindow.WindowConfig.Icon );
	}

	/// Init the clipboard after the window creation
	reinterpret_cast<ClipboardSFML*> ( mClipboard )->init();

	/// Init the input after the window creation
	reinterpret_cast<InputSFML*> ( mInput )->init();

	logSuccessfulInit( getVersion() );

	return true;
}

std::string WindowSFML::getVersion() {
	return std::string( "SFML 2" );
}

void WindowSFML::createPlatform() {
#if defined( EE_X11_PLATFORM )
	if ( 0 != getWindowHandler() ) {
		mPlatform = eeNew( Platform::X11Impl, ( this, getWindowHandler(), mSFMLWindow.getSystemHandle(), mSFMLWindow.getSystemHandle(), NULL, NULL ) );
	} else {
		Window::createPlatform();
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::WinImpl, ( this, GetWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::OSXImpl, ( this ) );
#else
	Window::CreatePlatform();
#endif
}

void WindowSFML::toggleFullscreen() {
}

void WindowSFML::setCaption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	mSFMLWindow.setTitle( Caption );
}

bool WindowSFML::setIcon( const std::string& Path ) {
	mWindow.WindowConfig.Icon 	= Path;

	Image Img( Path );

	mSFMLWindow.setIcon( Img.width(), Img.height(), Img.getPixelsPtr() );

	return true;
}

void WindowSFML::hide() {
	mSFMLWindow.setVisible( false );
	mVisible = false;
}

void WindowSFML::show() {
	mSFMLWindow.setVisible( true );
	mVisible = true;
}

void WindowSFML::setPosition( Int16 Left, Int16 Top ) {
	mSFMLWindow.setPosition( sf::Vector2i( Left, Top ) );
}

bool WindowSFML::isActive() {
	return reinterpret_cast<InputSFML*> ( mInput )->mWinActive;
}

bool WindowSFML::isVisible() {
	return mVisible;
}

Vector2i WindowSFML::getPosition() {
	sf::Vector2i v( mSFMLWindow.getPosition() );
	return Vector2i( v.x, v.y );
}

void WindowSFML::setSize( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.width();
		Height	= mWindow.DesktopResolution.height();
	}

	if ( this->isWindowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	sf::Vector2u v( Width, Height );

	mSFMLWindow.setSize( v );
}

void WindowSFML::videoResize( Uint32 Width, Uint32 Height ) {
	mWindow.WindowConfig.Width	= Width;
	mWindow.WindowConfig.Height	= Height;

	mDefaultView.setView( 0, 0, Width, Height );

	setup2D();

	mCursorManager->reload();

	sendVideoResizeCb();
}

void WindowSFML::swapBuffers() {
	mSFMLWindow.display();
}

std::vector<DisplayMode> WindowSFML::getDisplayModes() const {
	std::vector<DisplayMode> result;

	std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();

	for (std::size_t i = 0; i < modes.size(); ++i) {
		sf::VideoMode mode = modes[i];

		result.push_back( DisplayMode( mode.width, mode.height, 60, 0 ) );
	}

	return result;
}

void WindowSFML::setGamma( Float Red, Float Green, Float Blue ) {
}

eeWindowContex WindowSFML::getContext() const {
	return 0;
}

void WindowSFML::getMainContext() {
}

eeWindowHandle	WindowSFML::getWindowHandler() {
#if defined( EE_X11_PLATFORM )
	if ( 0 == mWinHandler ) {
		#ifdef EE_SUPPORT_EXCEPTIONS
		try {
		#endif
			mWinHandler = XOpenDisplay( NULL );
		#ifdef EE_SUPPORT_EXCEPTIONS
		} catch (...) {
			mWinHandler = 0;
		}
		#endif
	}

	return mWinHandler;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return (eeWindowHandle)mSFMLWindow.getSystemHandle();
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	return (eeWindowHandle)mSFMLWindow.getSystemHandle();
#else
	return 0;
#endif
}

void WindowSFML::setDefaultContext() {
}

sf::Window * WindowSFML::getSFMLWindow() {
	return &mSFMLWindow;
}

}}}}

#endif
