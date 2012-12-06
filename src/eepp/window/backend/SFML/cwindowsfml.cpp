#include <eepp/window/backend/SFML/cwindowsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

#include <SFML/Graphics.hpp>

#include <eepp/window/backend/SFML/cclipboardsfml.hpp>
#include <eepp/window/backend/SFML/cinputsfml.hpp>
#include <eepp/window/backend/SFML/ccursormanagersfml.hpp>
#include <eepp/window/platform/platformimpl.hpp>

#include <eepp/graphics/renderer/cgl.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

cWindowSFML::cWindowSFML( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardSFML, ( this ) ), eeNew( cInputSFML, ( this ) ), eeNew( cCursorManagerSFML, ( this ) ) ),
	mWinHandler( 0 ),
	mVisible( false )
{
	Create( Settings, Context );
}

cWindowSFML::~cWindowSFML() {
}

bool cWindowSFML::Create( WindowSettings Settings, ContextSettings Context ) {
	if ( mWindow.Created )
		return false;

	sf::VideoMode mode			= sf::VideoMode::getDesktopMode();
	mWindow.WindowConfig		= Settings;
	mWindow.ContextConfig		= Context;
	mWindow.DesktopResolution	= eeSize( mode.width, mode.height );

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

	if ( NULL == cGL::ExistsSingleton() ) {
		cGL::CreateSingleton( mWindow.ContextConfig.Version );
		cGL::instance()->Init();
	}

	CreatePlatform();

	GetMainContext();

	CreateView();

	Setup2D();

	mWindow.Created = true;
	mVisible = true;

	if ( "" != mWindow.WindowConfig.Icon ) {
		Icon( mWindow.WindowConfig.Icon );
	}

	/// Init the clipboard after the window creation
	reinterpret_cast<cClipboardSFML*> ( mClipboard )->Init();

	/// Init the input after the window creation
	reinterpret_cast<cInputSFML*> ( mInput )->Init();

	LogSuccessfulInit( GetVersion() );

	return true;
}

std::string cWindowSFML::GetVersion() {
	return std::string( "SFML 2" );
}

void cWindowSFML::CreatePlatform() {
#if defined( EE_X11_PLATFORM )
	if ( 0 != GetWindowHandler() ) {
		mPlatform = eeNew( Platform::cX11Impl, ( this, GetWindowHandler(), mSFMLWindow.getSystemHandle(), mSFMLWindow.getSystemHandle(), NULL, NULL ) );
	} else {
		cWindow::CreatePlatform();
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::cWinImpl, ( this, GetWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::cOSXImpl, ( this ) );
#else
	cWindow::CreatePlatform();
#endif
}

void cWindowSFML::ToggleFullscreen() {
}

void cWindowSFML::Caption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	mSFMLWindow.setTitle( Caption );
}

bool cWindowSFML::Icon( const std::string& Path ) {
	mWindow.WindowConfig.Icon 	= Path;

	cImage Img( Path );

	mSFMLWindow.setIcon( Img.Width(), Img.Height(), Img.GetPixelsPtr() );

	return true;
}

void cWindowSFML::Hide() {
	mSFMLWindow.setVisible( false );
	mVisible = false;
}

void cWindowSFML::Show() {
	mSFMLWindow.setVisible( true );
	mVisible = true;
}

void cWindowSFML::Position( Int16 Left, Int16 Top ) {
	mSFMLWindow.setPosition( sf::Vector2i( Left, Top ) );
}

bool cWindowSFML::Active() {
	return reinterpret_cast<cInputSFML*> ( mInput )->mWinActive;
}

bool cWindowSFML::Visible() {
	return mVisible;
}

eeVector2i cWindowSFML::Position() {
	sf::Vector2i v( mSFMLWindow.getPosition() );
	return eeVector2i( v.x, v.y );
}

void cWindowSFML::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.Width();
		Height	= mWindow.DesktopResolution.Height();
	}

	if ( this->Windowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	sf::Vector2u v( Width, Height );

	mSFMLWindow.setSize( v );
}

void cWindowSFML::VideoResize( Uint32 Width, Uint32 Height ) {
	mWindow.WindowConfig.Width    = Width;
	mWindow.WindowConfig.Height   = Height;

	mDefaultView.SetView( 0, 0, Width, Height );

	Setup2D();

	mCursorManager->Reload();

	SendVideoResizeCb();
}

void cWindowSFML::SwapBuffers() {
	mSFMLWindow.display();
}

std::vector< std::pair<unsigned int, unsigned int> > cWindowSFML::GetPossibleResolutions() const {
	return std::vector< std::pair<unsigned int, unsigned int> >();
}

void cWindowSFML::SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue ) {
}

eeWindowContex cWindowSFML::GetContext() const {
	return 0;
}

void cWindowSFML::GetMainContext() {
}

eeWindowHandle	cWindowSFML::GetWindowHandler() {
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

void cWindowSFML::SetDefaultContext() {
}

sf::Window * cWindowSFML::GetSFMLWindow() {
	return &mSFMLWindow;
}

}}}}

#endif
