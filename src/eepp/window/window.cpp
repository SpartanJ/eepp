#include <eepp/window/window.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/platform/null/nullimpl.hpp>
#include <eepp/window/platformimpl.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/version.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>

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

Sizei Window::size() {
	return Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
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

void Window::size( Uint32 Width, Uint32 Height ) {
	size( Width, Height, isWindowed() );
}

bool Window::isWindowed() const {
	return 0 != !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen );
}

bool Window::isResizeable() const {
	return 0 != ( mWindow.WindowConfig.Style & WindowStyle::Resize );
}

void Window::set2DProjection( const Uint32& Width, const Uint32& Height ) {
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadIdentity();

	GLi->Ortho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
}

void Window::setViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height, const bool& UpdateProjectionMatrix ) {
	GLi->Viewport( x, getHeight() - ( y + Height ), Width, Height );

	if ( UpdateProjectionMatrix ) {
		set2DProjection( Width, Height );
	}
}

void Window::setView( const View& View ) {
	mCurrentView = &View;

	Recti RView = mCurrentView->getView();
	setViewport( RView.Left, RView.Top, RView.Right, RView.Bottom );
}

const View& Window::getDefaultView() const {
	return mDefaultView;
}

const View& Window::getView() const {
	return *mCurrentView;
}

void Window::createView() {
	mDefaultView.setView( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	mCurrentView = &mDefaultView;
}

void Window::setup2D( const bool& KeepView ) {
	GLi->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	GLi->PixelStorei( GL_PACK_ALIGNMENT, 1 );

	backColor( mWindow.BackgroundColor );

	GLi->LineSmooth();

	GLi->Enable	( GL_TEXTURE_2D ); 						// Enable Textures
	GLi->Disable( GL_DEPTH_TEST );

	if ( GLv_2 == GLi->Version() || GLv_ES1 == GLi->Version() ) {
		GLi->Disable( GL_LIGHTING );
	}

	if ( !KeepView ) {
		setView( mDefaultView );

		mCurrentView->needUpdate();
	}

	BlendMode::SetMode( ALPHA_NORMAL, true );

	if ( GLv_3 != GLi->Version() ) {
		#if !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )
		GLi->TexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		GLi->TexEnvi( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
		#endif
	}

	if ( GLv_2 == GLi->Version() || GLv_ES1 == GLi->Version() ) {
		GLi->EnableClientState( GL_VERTEX_ARRAY );
		GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
		GLi->EnableClientState( GL_COLOR_ARRAY );
	}
}

const WindowInfo * Window::getWindowInfo() const {
	return &mWindow;
}

void Window::backColor( const RGB& Color ) {
	mWindow.BackgroundColor = Color;
	GLi->ClearColor( static_cast<Float>( mWindow.BackgroundColor.r() ) / 255.0f, static_cast<Float>( mWindow.BackgroundColor.g() ) / 255.0f, static_cast<Float>( mWindow.BackgroundColor.b() ) / 255.0f, 255.0f );
}

const RGB& Window::backColor() const {
	return mWindow.BackgroundColor;
}

bool Window::takeScreenshot( std::string filepath, const EE_SAVE_TYPE& Format ) {
	GlobalBatchRenderer::instance()->Draw();

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

		Ext = "." + Image::SaveTypeToExtension( Format );

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

bool Window::created() const {
	return mWindow.Created;
}

void Window::close() {
	mWindow.Created = false;
}

void Window::frameRateLimit( const Uint32& FrameRateLimit ) {
	mFrameData.FPS.Limit = (Float)FrameRateLimit;
}

Uint32 Window::frameRateLimit() {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 Window::FPS() const {
	return mFrameData.FPS.Current;
}

Time Window::elapsed() const {
	return mFrameData.ElapsedTime;
}

void Window::getElapsedTime() {
	if ( NULL == mFrameData.FrameElapsed ) {
		mFrameData.FrameElapsed = eeNew( Clock, () );
	}

	mFrameData.ElapsedTime = mFrameData.FrameElapsed->elapsed();
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

void Window::viewCheckUpdate() {
	if ( mCurrentView->needUpdate() ) {
		setView( *mCurrentView );
	}
}

void Window::clear() {
	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void Window::display( bool clear ) {
	GlobalBatchRenderer::instance()->Draw();

	if ( mCurrentView->needUpdate() )
		setView( *mCurrentView );

	swapBuffers();

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( clear )
		this->clear();
	#endif

	getElapsedTime();

	calculateFps();

	limitFps();
}

void Window::clipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	GlobalBatchRenderer::instance()->Draw();
	GLi->Scissor( x, getHeight() - ( y + Height ), Width, Height );
	GLi->Enable( GL_SCISSOR_TEST );
}

void Window::clipDisable() {
	GlobalBatchRenderer::instance()->Draw();
	GLi->Disable( GL_SCISSOR_TEST );
}

void Window::clipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	GlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneEnable( x, y, Width, Height );
}

void Window::clipPlaneDisable() {
	GlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneDisable();
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
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
		i->second( this );
	}
}

void Window::logSuccessfulInit(const std::string& BackendName , const std::string&ProcessPath ) {
	std::string msg( "Engine Initialized Succesfully.\n\tVersion: " + Version::getVersionName() + " (codename: \"" + Version::getCodename() + "\")" +
							 "\n\tBuild time: " + Version::getBuildTime() +
							 "\n\tOS: " + Sys::getOSName(true) +
							 "\n\tArch: " + Sys::getOSArchitecture() +
							 "\n\tCPU Cores: " + String::toStr( Sys::getCPUCount() ) +
							 "\n\tProcess Path: " + ( !ProcessPath.empty() ? ProcessPath : Sys::getProcessPath() ) +
							 "\n\tDisk Free Space: " + String::toStr( FileSystem::sizeToString( Sys::getDiskFreeSpace( Sys::getProcessPath() ) ) ) +
							 "\n\tWindow/Input Backend: " + BackendName +
							 "\n\tGL Backend: " + GLi->VersionStr() +
							 "\n\tGL Vendor: " + GLi->GetVendor() +
							 "\n\tGL Renderer: " + GLi->GetRenderer() +
							 "\n\tGL Version: " + GLi->GetVersion() +
							 "\n\tGL Shading Language Version: " + GLi->GetShadingLanguageVersion() +
							 "\n\tResolution: " + String::toStr( getWidth() ) + "x" + String::toStr( getHeight() ) +
							 "\n\tGL extensions supported:\n\t\t" + GLi->GetExtensions()
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

std::string Window::caption() {
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

void Window::position( Int16 Left, Int16 Top ) {
	if ( NULL != mPlatform )
		mPlatform->moveWindow( Left, Top );
}

Vector2i Window::position() {
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

void Window::center() {
	if ( isWindowed() ) {
		position( mWindow.DesktopResolution.width() / 2 - mWindow.WindowConfig.Width / 2, mWindow.DesktopResolution.height() / 2 - mWindow.WindowConfig.Height / 2 );
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

void Window::setTextInputRect( Recti& rect ) {
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
	frameRateLimit( fps );

	while ( isRunning() ) {
		func();
	}
#endif
}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
void * Window::getJNIEnv() {
	return NULL;
}

void * Window::getActivity() {
	return NULL;
}

int Window::getExternalStorageState() {
	return 0;
}

std::string Window::getInternalStoragePath() {
	return std::string("");
}

std::string Window::getExternalStoragePath() {
	return std::string("");
}

std::string Window::getApkPath() {
	return std::string("");
}
#endif

}}
