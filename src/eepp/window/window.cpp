#include <eepp/window/window.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/platform/null/nullimpl.hpp>
#include <eepp/window/platformimpl.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/version.hpp>
#include <SOIL2/src/SOIL2/SOIL2.h>

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

Window::FrameData::FrameData() :
	FrameElapsed(NULL),
	ElapsedTime()
{}

Window::FrameData::~FrameData()
{
	eeSAFE_DELETE( FrameElapsed );
}

Window::Window( WindowSettings Settings, ContextSettings Context, Clipboard * Clipboard, Input * Input, CursorManager * CursorManager ) :
	mClipboard( Clipboard ),
	mInput( Input ),
	mCursorManager( CursorManager ),
	mPlatform( NULL ),
	mCurrentView( NULL ),
	mNumCallBacks( 0 )
{
	mWindow.WindowConfig	= Settings;
	mWindow.ContextConfig	= Context;
}

Window::~Window() {
	eeSAFE_DELETE( mClipboard );
	eeSAFE_DELETE( mInput );
	eeSAFE_DELETE( mCursorManager );
	eeSAFE_DELETE( mPlatform );
}

Sizei Window::getSize() {
	return Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
}

Vector2f Window::getCenter() {
	return Sizef( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) * 0.5f;
}

const Uint32& Window::getWidth() const {
	return mWindow.WindowConfig.Width;
}

const Uint32& Window::getHeight() const {
	return mWindow.WindowConfig.Height;
}

const Sizei& Window::getDesktopResolution() {
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

void Window::setProjection(const Transform & transform) {
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( transform.getMatrix() );

	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

Vector2f Window::mapPixelToCoords(const Vector2i& point) {
	return mapPixelToCoords(point, getView());
}

Vector2f Window::mapPixelToCoords(const Vector2i& point, const View& view) {
	// First, convert from viewport coordinates to homogeneous coordinates
	Vector2f normalized;
	Rect viewport = getViewport(view);
	normalized.x = -1.f + 2.f * (point.x - viewport.Left) / viewport.Right;
	normalized.y =  1.f - 2.f * (point.y - viewport.Top)  / viewport.Bottom;

	// Then transform by the inverse of the view matrix
	return view.getInverseTransform().transformPoint(normalized);
}

Vector2i Window::mapCoordsToPixel(const Vector2f& point) {
	return mapCoordsToPixel(point, getView());
}

Vector2i Window::mapCoordsToPixel(const Vector2f& point, const View& view) {
	// First, transform the point by the view matrix
	Vector2f normalized = view.getTransform().transformPoint(point);

	// Then convert to viewport coordinates
	Vector2i pixel;
	Rect viewport = getViewport(view);
	pixel.x = static_cast<int>(( normalized.x + 1.f) / 2.f * viewport.Right  + viewport.Left);
	pixel.y = static_cast<int>((-normalized.y + 1.f) / 2.f * viewport.Bottom + viewport.Top);

	return pixel;
}

void Window::setViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	GLi->viewport( x, getHeight() - ( y + Height ), Width, Height );
}

Rect Window::getViewport( const View& view ) {
	float width  = static_cast<float>(getSize().getWidth());
	float height = static_cast<float>(getSize().getHeight());
	const Rectf& viewport = view.getViewport();

	return Rect(   static_cast<int>(0.5f + width  * viewport.Left),
				   static_cast<int>(0.5f + height * viewport.Top),
				   static_cast<int>(0.5f + width  * viewport.Right),
				   static_cast<int>(0.5f + height * viewport.Bottom));
}

void Window::setView( const View& view , bool forceRefresh ) {
	const View * viewPtr = &view;

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
	GLi->pixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	GLi->pixelStorei( GL_PACK_ALIGNMENT, 1 );

	setClearColor( mWindow.ClearColor );

	GLi->lineSmooth();

	GLi->enable	( GL_TEXTURE_2D ); 						// Enable Textures
	GLi->disable( GL_DEPTH_TEST );

	if ( GLv_2 == GLi->version() || GLv_ES1 == GLi->version() ) {
		GLi->disable( GL_LIGHTING );
	}

	if ( !KeepView ) {
		setView( mDefaultView, true );
	}

	BlendMode::setMode( BlendAlpha, true );

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
}

const WindowInfo * Window::getWindowInfo() const {
	return &mWindow;
}

void Window::setClearColor( const RGB& Color ) {
	mWindow.ClearColor = Color;
	GLi->clearColor( static_cast<Float>( mWindow.ClearColor.r ) / 255.0f, static_cast<Float>( mWindow.ClearColor.g ) / 255.0f, static_cast<Float>( mWindow.ClearColor.b ) / 255.0f, 255.0f );
}

RGB Window::getClearColor() const {
	return mWindow.ClearColor;
}

bool Window::takeScreenshot(std::string filepath, const Image::SaveType & Format ) {
	GlobalBatchRenderer::instance()->draw();

	bool CreateNewFile = false;
	std::string File, Ext;

	if ( filepath.size() ) {
		File = filepath.substr( filepath.find_last_of("/\\") + 1 );
		Ext = File.substr( File.find_last_of(".") + 1 );

		if ( FileSystem::isDirectory( filepath ) || !Ext.size() )
			CreateNewFile = true;
	} else {
		filepath = Sys::getProcessPath();
		CreateNewFile = true;
	}

	if ( CreateNewFile ) { // Search if file path is given, and if have and extension
		bool find = false;
		Int32 FileNum = 1;
		std::string TmpPath = filepath, Ext;

		if ( !FileSystem::isDirectory( filepath ) )
			FileSystem::makeDir( filepath );

		Ext = "." + Image::saveTypeToExtension( Format );

		while ( !find && FileNum < 10000 ) {
			TmpPath = String::strFormated( "%s%05d%s", filepath.c_str(), FileNum, Ext.c_str() );

			FileNum++;

			if ( !FileSystem::fileExists( TmpPath ) )
				find = true;

			if ( FileNum == 10000 && find == false )
				return false;
		}

		return 0 != SOIL_save_screenshot(TmpPath.c_str(), Format, 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	} else {
		std::string Direc = FileSystem::fileRemoveFileName( filepath );

		if ( !FileSystem::isDirectory( Direc ) )
			FileSystem::makeDir( Direc );

		return 0 != SOIL_save_screenshot(filepath.c_str(), Format, 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
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

Uint32 Window::getFrameRateLimit() {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 Window::getFPS() const {
	return mFrameData.FPS.Current;
}

Time Window::getElapsed() const {
	return mFrameData.ElapsedTime;
}

void Window::getElapsedTime() {
	if ( NULL == mFrameData.FrameElapsed ) {
		mFrameData.FrameElapsed = eeNew( Clock, () );
	}

	mFrameData.ElapsedTime = mFrameData.FrameElapsed->getElapsed();
}

void Window::calculateFps() {
	if ( Sys::getTicks() - mFrameData.FPS.LastCheck >= 1000 ) {
		mFrameData.FPS.Current = mFrameData.FPS.Count;
		mFrameData.FPS.Count = 0;
		mFrameData.FPS.LastCheck =	Sys::getTicks();
	}

	mFrameData.FPS.Count++;
}

void Window::limitFps() {
	if ( mFrameData.FPS.Limit > 0 ) {
		mFrameData.FPS.Error = 0;
		double RemainT = 1000.0 / mFrameData.FPS.Limit - ( mFrameData.ElapsedTime.asMilliseconds() * 0.1f );

		if ( RemainT < 0 ) {
			mFrameData.FPS.Error = 0;
		} else {
			mFrameData.FPS.Error += 1000 % (Int32)mFrameData.FPS.Limit;

			if ( mFrameData.FPS.Error > (Int32)mFrameData.FPS.Limit ) {
				++RemainT;
				mFrameData.FPS.Error -= (Int32)mFrameData.FPS.Limit;
			}

			if ( RemainT > 0 ) {
				Sys::sleep( (Uint32) RemainT );
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

	getElapsedTime();

	calculateFps();

	limitFps();
}

Clipboard * Window::getClipboard() const {
	return mClipboard;
}

Input * Window::getInput() const {
	return mInput;
}

CursorManager * Window::getCursorManager() const {
	return mCursorManager;
}

Uint32 Window::pushResizeCallback( const WindowResizeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[ mNumCallBacks ] = cb;
	return mNumCallBacks;
}

void Window::popResizeCallback( const Uint32& CallbackId ) {
	mCallbacks[ CallbackId ] = 0;
	mCallbacks.erase( mCallbacks.find(CallbackId) );
}

void Window::sendVideoResizeCb() {
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); ++i ) {
		i->second( this );
	}
}

void Window::logSuccessfulInit(const std::string& BackendName ) {
	std::string msg( "Engine Initialized Succesfully.\n\tVersion: " + Version::getVersionName() + " (codename: \"" + Version::getCodename() + "\")" +
							 "\n\tBuild time: " + Version::getBuildTime() +
							 "\n\tPlatform: " + Sys::getPlatform() +
							 "\n\tOS: " + Sys::getOSName(true) +
							 "\n\tArch: " + Sys::getOSArchitecture() +
							 "\n\tCPU Cores: " + String::toStr( Sys::getCPUCount() ) +
							 "\n\tProcess Path: " + Sys::getProcessPath() +
							 "\n\tCurrent Working Directory: " + FileSystem::getCurrentWorkingDirectory() +
							 "\n\tDisk Free Space: " + String::toStr( FileSystem::sizeToString( FileSystem::getDiskFreeSpace( Sys::getProcessPath() ) ) ) +
							 "\n\tWindow/Input Backend: " + BackendName +
							 "\n\tGL Backend: " + GLi->versionStr() +
							 "\n\tGL Vendor: " + GLi->getVendor() +
							 "\n\tGL Renderer: " + GLi->getRenderer() +
							 "\n\tGL Version: " + GLi->getVersion() +
							 "\n\tGL Shading Language Version: " + GLi->getShadingLanguageVersion() +
							 "\n\tResolution: " + String::toStr( getWidth() ) + "x" + String::toStr( getHeight() )
	);

	#ifndef EE_SILENT
	eePRINTL( msg.c_str() );
	#else
	Log::instance()->write( msg );
	#endif
}

void Window::logFailureInit( const std::string& ClassName, const std::string& BackendName ) {
	eePRINTL( "Error on %s::Init. Backend %s failed to start.", ClassName.c_str(), BackendName.c_str() );
}

std::string Window::getCaption() {
	return mWindow.WindowConfig.Caption;
}

eeWindowContex Window::getContext() const {
#if defined( EE_GLEW_AVAILABLE  ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_MACOSX )
	return mWindow.Context;
#else
	return 0;
#endif
}

void Window::getMainContext() {
#ifdef EE_GLEW_AVAILABLE
	if ( NULL != mPlatform )
		mWindow.Context = mPlatform->getWindowContext();
#endif
}

void Window::setDefaultContext() {
#if defined( EE_GLEW_AVAILABLE ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) )
	setCurrentContext( mWindow.Context );
#endif
}

void Window::minimize() {
	if ( NULL != mPlatform )
		mPlatform->minimizeWindow();
}

void Window::maximize() {
	if ( NULL != mPlatform )
		mPlatform->maximizeWindow();
}

bool Window::isMaximized() {
	if ( NULL != mPlatform )
		return mPlatform->isWindowMaximized();

	return false;
}

void Window::hide() {
	if ( NULL != mPlatform )
		mPlatform->hideWindow();
}

void Window::raise() {
	if ( NULL != mPlatform )
		mPlatform->raiseWindow();
}

void Window::show() {
	if ( NULL != mPlatform )
		mPlatform->showWindow();
}

void Window::setPosition( Int16 Left, Int16 Top ) {
	if ( NULL != mPlatform )
		mPlatform->moveWindow( Left, Top );
}

Vector2i Window::getPosition() {
	if ( NULL != mPlatform )
		return mPlatform->getPosition();

	return Vector2i();
}

void Window::setCurrentContext( eeWindowContex Context ) {
	if ( NULL != mPlatform )
		mPlatform->setContext( Context );
}

void Window::createPlatform() {
	eeSAFE_DELETE( mPlatform );
	mPlatform = eeNew( Platform::NullImpl, ( this ) );
}

void Window::setCurrent() {
}

void Window::centerToDisplay() {
	if ( isWindowed() ) {
		setPosition( mWindow.DesktopResolution.getWidth() / 2 - mWindow.WindowConfig.Width / 2, mWindow.DesktopResolution.getHeight() / 2 - mWindow.WindowConfig.Height / 2 );
	}
}

Platform::PlatformImpl * Window::getPlatform() const {
	return mPlatform;
}

void Window::startTextInput() {
}

bool Window::isTextInputActive() {
	return false;
}

void Window::stopTextInput() {
}

void Window::setTextInputRect( Rect& rect ) {
}

bool Window::hasScreenKeyboardSupport()
{
	return false;
}

bool Window::isScreenKeyboardShown() {
	return false;
}

bool Window::isThreadedGLContext() {
	return false;
}

void Window::setGLContextThread() {
}

void Window::unsetGLContextThread() {
}

void Window::runMainLoop( void (*func)(), int fps ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_set_main_loop(func, fps, 1);
#else
	setFrameRateLimit( fps );

	while ( isRunning() ) {
		func();
	}
#endif
}

int Window::getCurrentDisplayIndex() {
	return 0;
}

}}
