#include <SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/version.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

#ifdef EE_GLES1_LATE_INCLUDE
#if EE_PLATFORM == EE_PLATFORM_IOS
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#else
#include <GLES/gl.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES/glext.h>
#endif
#endif

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

namespace EE { namespace Window {

Window::FrameData::FrameData() : FrameElapsed( NULL ), ElapsedTime() {}

Window::FrameData::~FrameData() {
	eeSAFE_DELETE( FrameElapsed );
}

Window::Window( WindowSettings Settings, ContextSettings Context, Clipboard* Clipboard,
				Input* Input, CursorManager* CursorManager ) :
	mClipboard( Clipboard ),
	mInput( Input ),
	mCursorManager( CursorManager ),
	mCurrentView( NULL ),
	mNumCallBacks( 0 ),
	mIME( this ) {
	mWindow.WindowConfig = Settings;
	mWindow.ContextConfig = Context;
	setFrameRateLimit( Context.FrameRateLimit );
}

Window::~Window() {
	eeSAFE_DELETE( mClipboard );
	eeSAFE_DELETE( mInput );
	eeSAFE_DELETE( mCursorManager );
}

Sizei Window::getSize() const {
	return Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
}

Sizei Window::getSizeInScreenCoordinates() const {
	Float scale = getScale();
	return Sizei{ static_cast<int>( mWindow.WindowConfig.Width / scale ),
				  static_cast<int>( mWindow.WindowConfig.Height / scale ) };
}

Vector2f Window::getCenter() const {
	return Sizef( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) * 0.5f;
}

const Uint32& Window::getWidth() const {
	return mWindow.WindowConfig.Width;
}

const Uint32& Window::getHeight() const {
	return mWindow.WindowConfig.Height;
}

const Sizei& Window::getDesktopResolution() const {
	return mWindow.DesktopResolution;
}

void Window::setSize( Uint32 Width, Uint32 Height ) {
	setSize( Width, Height, isWindowed() );
}

bool Window::isWindowed() const {
	return 0 != !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen );
}

bool Window::isResizeable() const {
	return 0 != ( mWindow.WindowConfig.Style & WindowStyle::Resize );
}

void Window::set2DProjection( const Uint32& Width, const Uint32& Height ) {
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadIdentity();

	GLi->ortho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

void Window::setProjection( const Transform& transform ) {
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( transform.getMatrix() );

	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

Vector2f Window::mapPixelToCoords( const Vector2i& point ) const {
	return mapPixelToCoords( point, getView() );
}

Vector2f Window::mapPixelToCoords( const Vector2i& point, const View& view ) const {
	// First, convert from viewport coordinates to homogeneous coordinates
	Vector2f normalized;
	Rect viewport = getViewport( view );
	normalized.x = -1.f + 2.f * ( point.x - viewport.Left ) / viewport.Right;
	normalized.y = 1.f - 2.f * ( point.y - viewport.Top ) / viewport.Bottom;

	// Then transform by the inverse of the view matrix
	return view.getInverseTransform().transformPoint( normalized );
}

Vector2i Window::mapCoordsToPixel( const Vector2f& point ) const {
	return mapCoordsToPixel( point, getView() );
}

Vector2i Window::mapCoordsToPixel( const Vector2f& point, const View& view ) const {
	// First, transform the point by the view matrix
	Vector2f normalized = view.getTransform().transformPoint( point );

	// Then convert to viewport coordinates
	Vector2i pixel;
	Rect viewport = getViewport( view );
	pixel.x = static_cast<int>( ( normalized.x + 1.f ) / 2.f * viewport.Right + viewport.Left );
	pixel.y = static_cast<int>( ( -normalized.y + 1.f ) / 2.f * viewport.Bottom + viewport.Top );

	return pixel;
}

void Window::setCloseRequestCallback( const WindowRequestCloseCallback& closeRequestCallback ) {
	mCloseRequestCallback = closeRequestCallback;
}

void Window::setQuitCallback( const WindowQuitCallback& quitCallback ) {
	mQuitCallback = quitCallback;
}

void Window::setViewport( const Int32& x, const Int32& y, const Uint32& Width,
						  const Uint32& Height ) {
	GLi->viewport( x, getHeight() - ( y + Height ), Width, Height );
}

Rect Window::getViewport( const View& view ) const {
	float width = static_cast<float>( getSize().getWidth() );
	float height = static_cast<float>( getSize().getHeight() );
	const Rectf& viewport = view.getViewport();

	return Rect( static_cast<int>( 0.5f + width * viewport.Left ),
				 static_cast<int>( 0.5f + height * viewport.Top ),
				 static_cast<int>( 0.5f + width * viewport.Right ),
				 static_cast<int>( 0.5f + height * viewport.Bottom ) );
}

void Window::setView( const View& view, bool forceRefresh ) {
	const View* viewPtr = &view;

	if ( viewPtr != mCurrentView || viewPtr->isDirty() || forceRefresh ) {
		mCurrentView = viewPtr;

		Rect viewport = getViewport( *mCurrentView );

		setViewport( viewport.Left, viewport.Top, viewport.Right, viewport.Bottom );

		setProjection( viewPtr->getTransform() );
	}
}

const View& Window::getDefaultView() const {
	return mDefaultView;
}

const View& Window::getView() const {
	return *mCurrentView;
}

void Window::createView() {
	mDefaultView.reset( Rectf( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) );
	setView( mDefaultView );
}

void Window::setup2D( const bool& KeepView ) {
	EE::Window::Window* curWin = Engine::instance()->getCurrentWindow();
	if ( curWin != this )
		Engine::instance()->setCurrentWindow( this );

	GLi->pixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	GLi->pixelStorei( GL_PACK_ALIGNMENT, 1 );

	setClearColor( mWindow.ClearColor );

	GLi->lineSmooth();

	GLi->enable( GL_TEXTURE_2D ); // Enable Textures
	GLi->disable( GL_DEPTH_TEST );

	if ( GLv_2 == GLi->version() || GLv_ES1 == GLi->version() ) {
		GLi->disable( GL_LIGHTING );
	}

	if ( !KeepView ) {
		setView( mDefaultView, true );
	}

	BlendMode::setMode( BlendMode::Alpha(), true );

	if ( GLv_3CP != GLi->version() && GLv_3 != GLi->version() && GLv_ES2 != GLi->version() ) {
#if !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )
		glTexEnvi( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
#endif
	}

	if ( GLv_2 == GLi->version() || GLv_ES1 == GLi->version() ) {
		GLi->enableClientState( GL_VERTEX_ARRAY );
		GLi->enableClientState( GL_TEXTURE_COORD_ARRAY );
		GLi->enableClientState( GL_COLOR_ARRAY );
	}

	if ( curWin != this )
		Engine::instance()->setCurrentWindow( curWin );
}

const WindowInfo* Window::getWindowInfo() const {
	return &mWindow;
}

void Window::setClearColor( const RGB& Color ) {
	mWindow.ClearColor = Color;
	GLi->clearColor( static_cast<Float>( mWindow.ClearColor.r ) / 255.0f,
					 static_cast<Float>( mWindow.ClearColor.g ) / 255.0f,
					 static_cast<Float>( mWindow.ClearColor.b ) / 255.0f, 255.0f );
}

RGB Window::getClearColor() const {
	return mWindow.ClearColor;
}

bool Window::takeScreenshot( std::string filepath, const Image::SaveType& Format ) {
	GlobalBatchRenderer::instance()->draw();

	bool CreateNewFile = false;
	std::string File, Ext;

	if ( filepath.size() ) {
		File = filepath.substr( filepath.find_last_of( "/\\" ) + 1 );
		Ext = File.substr( File.find_last_of( "." ) + 1 );

		if ( FileSystem::isDirectory( filepath ) || !Ext.size() )
			CreateNewFile = true;
	} else {
		filepath = Sys::getProcessPath();
		CreateNewFile = true;
	}

	if ( CreateNewFile ) { // Search if file path is given, and if have and extension
		bool find = false;
		Int32 FileNum = 1;
		std::string TmpPath = filepath;

		if ( !FileSystem::isDirectory( filepath ) )
			FileSystem::makeDir( filepath );

		Ext = "." + Image::saveTypeToExtension( Format );

		while ( !find && FileNum < 10000 ) {
			TmpPath = String::format( "%s%05d%s", filepath.c_str(), FileNum, Ext.c_str() );

			FileNum++;

			if ( !FileSystem::fileExists( TmpPath ) )
				find = true;

			if ( FileNum == 10000 && find == false )
				return false;
		}

		return 0 != SOIL_save_screenshot( TmpPath.c_str(), (int)Format, 0, 0,
										  mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	} else {
		std::string Direc = FileSystem::fileRemoveFileName( filepath );

		if ( !FileSystem::isDirectory( Direc ) )
			FileSystem::makeDir( Direc );

		return 0 != SOIL_save_screenshot( filepath.c_str(), (int)Format, 0, 0,
										  mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	}
}

bool Window::isRunning() const {
	return mWindow.Created;
}

bool Window::isOpen() const {
	return mWindow.Created;
}

void Window::close() {
	mWindow.Created = false;
}

void Window::setFrameRateLimit( const Uint32& FrameRateLimit ) {
	mFrameData.FPS.Limit = (Float)FrameRateLimit;
}

Uint32 Window::getFrameRateLimit() const {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 Window::getFPS() const {
	return mFrameData.FPS.Current;
}

const Time& Window::getElapsed() const {
	return mFrameData.ElapsedTime;
}

void Window::updateElapsedTime() {
	if ( NULL == mFrameData.FrameElapsed ) {
		mFrameData.FrameElapsed = eeNew( Clock, () );
	}

	mFrameData.ElapsedTime = mFrameData.FrameElapsed->getElapsedTimeAndReset();
}

const Time& Window::getSleepTimePerSecond() const {
	return mFrameData.FPS.CurSleepTime;
}

const Time& Window::getRenderTimePerSecond() const {
	return mFrameData.FPS.CurRenderTime;
}

const Sizei& Window::getLastWindowedSize() const {
	return mLastWindowedSize;
}

Sizei Window::getLastWindowedSizeInScreenCoordinates() const {
	Float scale = getScale();
	return Sizei{ static_cast<int>( mLastWindowedSize.getWidth() / scale ),
				  static_cast<int>( mLastWindowedSize.getHeight() / scale ) };
}

bool Window::showMessageBox( const MessageBoxType&, const std::string&, const std::string& ) {
	return false;
}

InputMethod& Window::getIME() {
	return mIME;
}

const InputMethod& Window::getIME() const {
	return mIME;
}

void Window::calculateFps() {
	if ( mFrameData.FPS.LastCheck.getElapsedTime().asSeconds() >= 1.f ) {
		mFrameData.FPS.Current = mFrameData.FPS.Count;
		mFrameData.FPS.Count = 0;
		mFrameData.FPS.CurSleepTime = mFrameData.FPS.SleepTime;
		mFrameData.FPS.SleepTime = Time::Zero;
		mFrameData.FPS.CurRenderTime = mFrameData.FPS.RenderTime;
		mFrameData.FPS.RenderTime = Time::Zero;
		mFrameData.FPS.LastCheck.restart();
	}

	mFrameData.FPS.Count++;
}

void Window::limitFps() {
	if ( mFrameData.FPS.Limit > 0 ) {
		Time frameTime( Milliseconds( 1000.0 / mFrameData.FPS.Limit ) );
		Time remainingTime = frameTime - mFrameData.FPS.FrameTime;

		if ( frameTime - mFrameData.ElapsedTime > Time::Zero ) {
			remainingTime += mFrameData.FPS.Error;
		}

		mFrameData.FPS.Error = Time::Zero;

		if ( remainingTime > Time::Zero ) {
			Clock checkElapsed;

			Sys::sleep( remainingTime );

			Time elapsed = checkElapsed.getElapsedTimeAndReset();
			Time timeDiff = elapsed - remainingTime;

			mFrameData.FPS.SleepTime += elapsed;

			if ( timeDiff > Time::Zero ) {
				mFrameData.FPS.Error += timeDiff;
			}
		}
	}
}

void Window::clear() {
	GLi->clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void Window::display( bool clear ) {
	GlobalBatchRenderer::instance()->draw();

	swapBuffers();

	if ( mCurrentView->isDirty() )
		setView( *mCurrentView );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( clear )
		this->clear();
#endif

	mFrameData.FPS.FrameTime = mFrameData.FPS.RenderClock.getElapsedTime();
	mFrameData.FPS.RenderTime += mFrameData.FPS.FrameTime;

	updateElapsedTime();

	limitFps();

	calculateFps();

	mFrameData.FPS.RenderClock.restart();
}

Clipboard* Window::getClipboard() const {
	return mClipboard;
}

Input* Window::getInput() const {
	return mInput;
}

CursorManager* Window::getCursorManager() const {
	return mCursorManager;
}

Uint32 Window::pushResizeCallback( const WindowResizeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[mNumCallBacks] = cb;
	return mNumCallBacks;
}

void Window::popResizeCallback( const Uint32& CallbackId ) {
	mCallbacks[CallbackId] = 0;
	mCallbacks.erase( mCallbacks.find( CallbackId ) );
}

void Window::startOnScreenKeyboard() {}

void Window::stopOnScreenKeyboard() {}

bool Window::isOnScreenKeyboardActive() const {
	return false;
}

void Window::sendVideoResizeCb() {
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin();
		  i != mCallbacks.end(); ++i ) {
		i->second( this );
	}
}

void Window::logSuccessfulInit( const std::string& BackendName ) {
	std::string msg( String::format(
		"Engine Initialized Succesfully.\n\tVersion: %s (codename: \"%s\")\n\tBuild time: "
		"%s\n\tPlatform: %s\n\tOS: %s\n\tArch: %s\n\tCPU Cores: %d\n\tProcess Path: %s\n\tCurrent "
		"Working Directory: %s\n\tHome Directory: %s\n\tDisk Free Space: %s\n\tWindow/Input "
		"Backend: %s\n\tGL Backend: %s\n\tGL Vendor: %s\n\tGL Renderer: %s\n\tGL Version: "
		"%s\n\tGL Shading Language Version: %s\n\tResolution: %dx%d\n\tWindow scale: %.2f",
		Version::getVersionName(), Version::getCodename(), Version::getBuildTime(),
		Sys::getPlatform(), Sys::getOSName( true ), Sys::getOSArchitecture(), Sys::getCPUCount(),
		Sys::getProcessPath(), FileSystem::getCurrentWorkingDirectory(), Sys::getUserDirectory(),
		FileSystem::sizeToString( FileSystem::getDiskFreeSpace( Sys::getProcessPath() ) ),
		BackendName, GLi->versionStr(), GLi->getVendor(), GLi->getRenderer(), GLi->getVersion(),
		GLi->getShadingLanguageVersion(), getWidth(), getHeight(), getScale() ) );

#ifndef EE_SILENT
	Log::info( msg );
#else
	Log::instance()->writel( msg );
#endif
}

void Window::logFailureInit( const std::string& ClassName, const std::string& BackendName ) {
	Log::error( "Error on %s::Init. Backend %s failed to start.", ClassName.c_str(),
				BackendName.c_str() );
}

void Window::onCloseRequest() {
	if ( mCloseRequestCallback && !mCloseRequestCallback( this ) )
		return;

	close();
}

void Window::onQuit() {
	if ( mQuitCallback )
		mQuitCallback( this );
}

const std::string& Window::getTitle() const {
	return mWindow.WindowConfig.Title;
}

eeWindowContext Window::getContext() const {
#if defined( EE_GLEW_AVAILABLE ) &&                                   \
	( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) || \
	  EE_PLATFORM == EE_PLATFORM_MACOS )
	return mWindow.Context;
#else
	return 0;
#endif
}

void Window::getMainContext() {}

void Window::setDefaultContext() {
#if defined( EE_GLEW_AVAILABLE ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) )
	setCurrentContext( mWindow.Context );
#endif
}

void Window::minimize() {}

void Window::maximize() {}

bool Window::isMaximized() const {
	return false;
}

bool Window::isMinimized() const {
	return false;
}

void Window::hide() {}

void Window::raise() {}

void Window::restore() {}

void Window::flash( WindowFlashOperation ) {}

void Window::show() {}

void Window::setPosition( int, int ) {}

Vector2i Window::getPosition() const {
	return Vector2i::Zero;
}

void Window::setCurrentContext( eeWindowContext ) {}

void Window::setCurrent() {}

void Window::centerToDisplay() {
	if ( isWindowed() ) {
		setPosition( mWindow.DesktopResolution.getWidth() / 2 - mWindow.WindowConfig.Width / 2,
					 mWindow.DesktopResolution.getHeight() / 2 - mWindow.WindowConfig.Height / 2 );
	}
}

Rect Window::getBorderSize() const {
	return Rect();
}

Float Window::getScale() const {
	return 1.f;
}

void Window::startTextInput() {}

bool Window::isTextInputActive() const {
	return false;
}

void Window::stopTextInput() {}

void Window::setTextInputRect( const Rect& ) {}

void Window::clearComposition() {}

bool Window::hasScreenKeyboardSupport() const {
	return false;
}

bool Window::isScreenKeyboardShown() const {
	return false;
}

bool Window::isThreadedGLContext() const {
	return false;
}

void Window::setGLContextThread() {}

void Window::unsetGLContextThread() {}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
static void eepp_mainloop() {
	Engine::instance()->getCurrentWindow()->getMainLoop()();
}
#endif

void Window::runMainLoop( std::function<void()> func, int fps ) {
	mMainLoop = std::move( func );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_set_main_loop( eepp_mainloop, eemax( (int)mFrameData.FPS.Limit, eemax( 0, fps ) ),
							  1 );
#else
	if ( fps >= 0 )
		setFrameRateLimit( fps );

	while ( isRunning() ) {
		mMainLoop();
	}
#endif
}

int Window::getCurrentDisplayIndex() const {
	return 0;
}

const std::function<void()>& Window::getMainLoop() const {
	return mMainLoop;
}

}} // namespace EE::Window
