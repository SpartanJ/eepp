#include <eepp/window/window.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/platform/null/nullimpl.hpp>
#include <eepp/window/platformimpl.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
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

Sizei Window::Size() {
	return Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
}

const Uint32& Window::GetWidth() const {
	return mWindow.WindowConfig.Width;
}

const Uint32& Window::GetHeight() const {
	return mWindow.WindowConfig.Height;
}

const Sizei& Window::GetDesktopResolution() {
	return mWindow.DesktopResolution;
}

void Window::Size( Uint32 Width, Uint32 Height ) {
	Size( Width, Height, Windowed() );
}

bool Window::Windowed() const {
	return 0 != !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen );
}

bool Window::Resizeable() const {
	return 0 != ( mWindow.WindowConfig.Style & WindowStyle::Resize );
}

void Window::Set2DProjection( const Uint32& Width, const Uint32& Height ) {
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadIdentity();

	GLi->Ortho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
}

void Window::SetViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height, const bool& UpdateProjectionMatrix ) {
	GLi->Viewport( x, GetHeight() - ( y + Height ), Width, Height );

	if ( UpdateProjectionMatrix ) {
		Set2DProjection( Width, Height );
	}
}

void Window::SetView( const View& View ) {
	mCurrentView = &View;

	Recti RView = mCurrentView->GetView();
	SetViewport( RView.Left, RView.Top, RView.Right, RView.Bottom );
}

const View& Window::GetDefaultView() const {
	return mDefaultView;
}

const View& Window::GetView() const {
    return *mCurrentView;
}

void Window::CreateView() {
	mDefaultView.SetView( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	mCurrentView = &mDefaultView;
}

void Window::Setup2D( const bool& KeepView ) {
	GLi->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	GLi->PixelStorei( GL_PACK_ALIGNMENT, 1 );

	BackColor( mWindow.BackgroundColor );

	GLi->LineSmooth();

	GLi->Enable	( GL_TEXTURE_2D ); 						// Enable Textures
	GLi->Disable( GL_DEPTH_TEST );

	if ( GLv_2 == GLi->Version() || GLv_ES1 == GLi->Version() ) {
		GLi->Disable( GL_LIGHTING );
	}

	if ( !KeepView ) {
		SetView( mDefaultView );

		mCurrentView->NeedUpdate();
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

const WindowInfo * Window::GetWindowInfo() const {
	return &mWindow;
}

void Window::BackColor( const RGB& Color ) {
	mWindow.BackgroundColor = Color;
	GLi->ClearColor( static_cast<Float>( mWindow.BackgroundColor.R() ) / 255.0f, static_cast<Float>( mWindow.BackgroundColor.G() ) / 255.0f, static_cast<Float>( mWindow.BackgroundColor.B() ) / 255.0f, 255.0f );
}

const RGB& Window::BackColor() const {
	return mWindow.BackgroundColor;
}

bool Window::TakeScreenshot( std::string filepath, const EE_SAVE_TYPE& Format ) {
	cGlobalBatchRenderer::instance()->Draw();

	bool CreateNewFile = false;
	std::string File, Ext;

	if ( filepath.size() ) {
		File = filepath.substr( filepath.find_last_of("/\\") + 1 );
		Ext = File.substr( File.find_last_of(".") + 1 );

		if ( FileSystem::IsDirectory( filepath ) || !Ext.size() )
			CreateNewFile = true;
	} else {
		filepath = Sys::GetProcessPath();
		CreateNewFile = true;
	}

	if ( CreateNewFile ) { // Search if file path is given, and if have and extension
		bool find = false;
		Int32 FileNum = 1;
		std::string TmpPath = filepath, Ext;

		if ( !FileSystem::IsDirectory( filepath ) )
			FileSystem::MakeDir( filepath );

		Ext = "." + cImage::SaveTypeToExtension( Format );

		while ( !find && FileNum < 10000 ) {
			TmpPath = String::StrFormated( "%s%05d%s", filepath.c_str(), FileNum, Ext.c_str() );

			FileNum++;

			if ( !FileSystem::FileExists( TmpPath ) )
				find = true;

			if ( FileNum == 10000 && find == false )
				return false;
		}

		return 0 != SOIL_save_screenshot(TmpPath.c_str(), Format, 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	} else {
		std::string Direc = FileSystem::FileRemoveFileName( filepath );

		if ( !FileSystem::IsDirectory( Direc ) )
			FileSystem::MakeDir( Direc );

		return 0 != SOIL_save_screenshot(filepath.c_str(), Format, 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	}
}

bool Window::Running() const {
	return mWindow.Created;
}

bool Window::Created() const {
	return mWindow.Created;
}

void Window::Close() {
	mWindow.Created = false;
}

void Window::FrameRateLimit( const Uint32& FrameRateLimit ) {
	mFrameData.FPS.Limit = (Float)FrameRateLimit;
}

Uint32 Window::FrameRateLimit() {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 Window::FPS() const {
	return mFrameData.FPS.Current;
}

Time Window::Elapsed() const {
	return mFrameData.ElapsedTime;
}

void Window::GetElapsedTime() {
	if ( NULL == mFrameData.FrameElapsed ) {
		mFrameData.FrameElapsed = eeNew( Clock, () );
	}

	mFrameData.ElapsedTime = mFrameData.FrameElapsed->Elapsed();
}

void Window::CalculateFps() {
	if ( Sys::GetTicks() - mFrameData.FPS.LastCheck >= 1000 ) {
		mFrameData.FPS.Current = mFrameData.FPS.Count;
		mFrameData.FPS.Count = 0;
		mFrameData.FPS.LastCheck =	Sys::GetTicks();
	}

	mFrameData.FPS.Count++;
}

void Window::LimitFps() {
	if ( mFrameData.FPS.Limit > 0 ) {
		mFrameData.FPS.Error = 0;
		double RemainT = 1000.0 / mFrameData.FPS.Limit - ( mFrameData.ElapsedTime.AsMilliseconds() * 0.1f );

		if ( RemainT < 0 ) {
			mFrameData.FPS.Error = 0;
		} else {
			mFrameData.FPS.Error += 1000 % (Int32)mFrameData.FPS.Limit;

			if ( mFrameData.FPS.Error > (Int32)mFrameData.FPS.Limit ) {
				++RemainT;
				mFrameData.FPS.Error -= (Int32)mFrameData.FPS.Limit;
			}

			if ( RemainT > 0 ) {
				Sys::Sleep( (Uint32) RemainT );
			}
		}
	}
}

void Window::ViewCheckUpdate() {
	if ( mCurrentView->NeedUpdate() ) {
		SetView( *mCurrentView );
	}
}

void Window::Clear() {
	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void Window::Display( bool clear ) {
	cGlobalBatchRenderer::instance()->Draw();

	if ( mCurrentView->NeedUpdate() )
		SetView( *mCurrentView );

	SwapBuffers();

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( clear )
		Clear();
	#endif

	GetElapsedTime();

	CalculateFps();

	LimitFps();
}

void Window::ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Scissor( x, GetHeight() - ( y + Height ), Width, Height );
	GLi->Enable( GL_SCISSOR_TEST );
}

void Window::ClipDisable() {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Disable( GL_SCISSOR_TEST );
}

void Window::ClipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneEnable( x, y, Width, Height );
}

void Window::ClipPlaneDisable() {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneDisable();
}

Clipboard * Window::GetClipboard() const {
	return mClipboard;
}

Input * Window::GetInput() const {
	return mInput;
}

CursorManager * Window::GetCursorManager() const {
	return mCursorManager;
}

Uint32 Window::PushResizeCallback( const WindowResizeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[ mNumCallBacks ] = cb;
	return mNumCallBacks;
}

void Window::PopResizeCallback( const Uint32& CallbackId ) {
	mCallbacks[ CallbackId ] = 0;
	mCallbacks.erase( mCallbacks.find(CallbackId) );
}

void Window::SendVideoResizeCb() {
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
		i->second( this );
	}
}

void Window::LogSuccessfulInit(const std::string& BackendName , const std::string&ProcessPath ) {
	std::string msg( "Engine Initialized Succesfully.\n\tVersion: " + Version::GetVersionName() + " (codename: \"" + Version::GetCodename() + "\")" +
							 "\n\tOS: " + Sys::GetOSName() +
							 "\n\tArch: " + Sys::GetOSArchitecture() +
							 "\n\tCPU Cores: " + String::ToStr( Sys::GetCPUCount() ) +
							 "\n\tProcess Path: " + ( !ProcessPath.empty() ? ProcessPath : Sys::GetProcessPath() ) +
							 "\n\tDisk Free Space: " + String::ToStr( FileSystem::SizeToString( Sys::GetDiskFreeSpace( Sys::GetProcessPath() ) ) ) +
							 "\n\tWindow/Input Backend: " + BackendName +
							 "\n\tGL Backend: " + GLi->VersionStr() +
							 "\n\tGL Vendor: " + GLi->GetVendor() +
							 "\n\tGL Renderer: " + GLi->GetRenderer() +
							 "\n\tGL Version: " + GLi->GetVersion() +
							 "\n\tGL Shading Language Version: " + GLi->GetShadingLanguageVersion() +
							 "\n\tResolution: " + String::ToStr( GetWidth() ) + "x" + String::ToStr( GetHeight() ) +
							 "\n\tGL extensions supported:\n\t\t" + GLi->GetExtensions()
	);

	#ifndef EE_SILENT
	eePRINTL( msg.c_str() );
	#else
	Log::instance()->Write( msg );
	#endif
}

void Window::LogFailureInit( const std::string& ClassName, const std::string& BackendName ) {
	eePRINTL( "Error on %s::Init. Backend %s failed to start.", ClassName.c_str(), BackendName.c_str() );
}

std::string Window::Caption() {
	return mWindow.WindowConfig.Caption;
}

eeWindowContex Window::GetContext() const {
#if defined( EE_GLEW_AVAILABLE  ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_MACOSX )
	return mWindow.Context;
#else
	return 0;
#endif
}

void Window::GetMainContext() {
#ifdef EE_GLEW_AVAILABLE
	if ( NULL != mPlatform )
		mWindow.Context = mPlatform->GetWindowContext();
#endif
}

void Window::SetDefaultContext() {
#if defined( EE_GLEW_AVAILABLE ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) )
	SetCurrentContext( mWindow.Context );
#endif
}

void Window::Minimize() {
	if ( NULL != mPlatform )
		mPlatform->MinimizeWindow();
}

void Window::Maximize() {
	if ( NULL != mPlatform )
		mPlatform->MaximizeWindow();
}

bool Window::IsMaximized() {
	if ( NULL != mPlatform )
		return mPlatform->IsWindowMaximized();

	return false;
}

void Window::Hide() {
	if ( NULL != mPlatform )
		mPlatform->HideWindow();
}

void Window::Raise() {
	if ( NULL != mPlatform )
		mPlatform->RaiseWindow();
}

void Window::Show() {
	if ( NULL != mPlatform )
		mPlatform->ShowWindow();
}

void Window::Position( Int16 Left, Int16 Top ) {
	if ( NULL != mPlatform )
		mPlatform->MoveWindow( Left, Top );
}

Vector2i Window::Position() {
	if ( NULL != mPlatform )
		return mPlatform->Position();

	return Vector2i();
}

void Window::SetCurrentContext( eeWindowContex Context ) {
	if ( NULL != mPlatform )
		mPlatform->SetContext( Context );
}

void Window::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );
	mPlatform = eeNew( Platform::cNullImpl, ( this ) );
}

void Window::SetCurrent() {
}

void Window::Center() {
	if ( Windowed() ) {
		Position( mWindow.DesktopResolution.Width() / 2 - mWindow.WindowConfig.Width / 2, mWindow.DesktopResolution.Height() / 2 - mWindow.WindowConfig.Height / 2 );
	}
}

Platform::PlatformImpl * Window::GetPlatform() const {
	return mPlatform;
}

void Window::StartTextInput() {
}

bool Window::IsTextInputActive() {
	return false;
}

void Window::StopTextInput() {
}

void Window::SetTextInputRect( Recti& rect ) {
}

bool Window::HasScreenKeyboardSupport()
{
	return false;
}

bool Window::IsScreenKeyboardShown() {
	return false;
}

bool Window::IsThreadedGLContext() {
	return false;
}

void Window::SetGLContextThread() {
}

void Window::UnsetGLContextThread() {
}

void Window::RunMainLoop( void (*func)(), int fps ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_set_main_loop(func, fps, 1);
#else
	FrameRateLimit( fps );

	while ( Running() ) {
		func();
	}
#endif
}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
void * Window::GetJNIEnv() {
	return NULL;
}

void * Window::GetActivity() {
	return NULL;
}

int Window::GetExternalStorageState() {
	return 0;
}

std::string Window::GetInternalStoragePath() {
	return std::string("");
}

std::string Window::GetExternalStoragePath() {
	return std::string("");
}

std::string Window::GetApkPath() {
	return std::string("");
}
#endif

}}
