#include <eepp/window/cwindow.hpp>
#include <eepp/window/cclipboard.hpp>
#include <eepp/window/cinput.hpp>
#include <eepp/window/ccursormanager.hpp>
#include <eepp/window/platform/null/cnullimpl.hpp>
#include <eepp/window/cplatformimpl.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>

#include <eepp/system/filesystem.hpp>
#include <eepp/version.hpp>

#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>

#ifdef EE_GLEW_AVAILABLE
	#if EE_PLATFORM == EE_PLATFORM_WIN
		#include <eepp/helper/glew/wglew.h>
		#undef GetDiskFreeSpace
	#elif defined( EE_X11_PLATFORM )
		#include <GL/glx.h>
	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
		#include <AGL/agl.h>
	#endif
#endif

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

cWindow::cFrameData::cFrameData() :
	FrameElapsed(NULL),
	ElapsedTime()
{}

cWindow::cFrameData::~cFrameData()
{
	eeSAFE_DELETE( FrameElapsed );
}

cWindow::cWindow( WindowSettings Settings, ContextSettings Context, cClipboard * Clipboard, cInput * Input, cCursorManager * CursorManager ) :
	mClipboard( Clipboard ),
	mInput( Input ),
	mCursorManager( CursorManager ),
	mPlatform( NULL ),
	mNumCallBacks( 0 )
{
	mWindow.WindowConfig	= Settings;
	mWindow.ContextConfig	= Context;
}

cWindow::~cWindow() {
	eeSAFE_DELETE( mClipboard );
	eeSAFE_DELETE( mInput );
	eeSAFE_DELETE( mCursorManager );
	eeSAFE_DELETE( mPlatform );
}

eeSize cWindow::Size() {
	return eeSize( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
}

const Uint32& cWindow::GetWidth() const {
	return mWindow.WindowConfig.Width;
}

const Uint32& cWindow::GetHeight() const {
	return mWindow.WindowConfig.Height;
}

const eeSize& cWindow::GetDesktopResolution() {
	return mWindow.DesktopResolution;
}

void cWindow::Size( Uint32 Width, Uint32 Height ) {
	Size( Width, Height, Windowed() );
}

bool cWindow::Windowed() const {
	return 0 != !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen );
}

bool cWindow::Resizeable() const {
	return 0 != ( mWindow.WindowConfig.Style & WindowStyle::Resize );
}

void cWindow::Set2DProjection( const Uint32& Width, const Uint32& Height ) {
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadIdentity();

	GLi->Ortho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
}

void cWindow::SetViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height, const bool& UpdateProjectionMatrix ) {
	GLi->Viewport( x, GetHeight() - ( y + Height ), Width, Height );

	if ( UpdateProjectionMatrix ) {
		Set2DProjection( Width, Height );
	}
}

void cWindow::SetView( const cView& View ) {
	mCurrentView = &View;

	eeRecti RView = mCurrentView->GetView();
	SetViewport( RView.Left, RView.Top, RView.Right, RView.Bottom );
}

const cView& cWindow::GetDefaultView() const {
	return mDefaultView;
}

const cView& cWindow::GetView() const {
    return *mCurrentView;
}

void cWindow::CreateView() {
	mDefaultView.SetView( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	mCurrentView = &mDefaultView;
}

void cWindow::Setup2D( const bool& KeepView ) {
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

const WindowInfo * cWindow::GetWindowInfo() const {
	return &mWindow;
}

void cWindow::BackColor( const eeColor& Color ) {
	mWindow.BackgroundColor = Color;
	GLi->ClearColor( static_cast<Float>( mWindow.BackgroundColor.R() ) / 255.0f, static_cast<Float>( mWindow.BackgroundColor.G() ) / 255.0f, static_cast<Float>( mWindow.BackgroundColor.B() ) / 255.0f, 255.0f );
}

const eeColor& cWindow::BackColor() const {
	return mWindow.BackgroundColor;
}

bool cWindow::TakeScreenshot( std::string filepath, const EE_SAVE_TYPE& Format ) {
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

bool cWindow::Running() const {
	return mWindow.Created;
}

bool cWindow::Created() const {
	return mWindow.Created;
}

void cWindow::Close() {
	mWindow.Created = false;
}

void cWindow::FrameRateLimit( const Uint32& FrameRateLimit ) {
	mFrameData.FPS.Limit = (Float)FrameRateLimit;
}

Uint32 cWindow::FrameRateLimit() {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 cWindow::FPS() const {
	return mFrameData.FPS.Current;
}

Time cWindow::Elapsed() const {
	return mFrameData.ElapsedTime;
}

void cWindow::GetElapsedTime() {
	if ( NULL == mFrameData.FrameElapsed ) {
		mFrameData.FrameElapsed = eeNew( Clock, () );
	}

	mFrameData.ElapsedTime = mFrameData.FrameElapsed->Elapsed();
}

void cWindow::CalculateFps() {
	if ( Sys::GetTicks() - mFrameData.FPS.LastCheck >= 1000 ) {
		mFrameData.FPS.Current = mFrameData.FPS.Count;
		mFrameData.FPS.Count = 0;
		mFrameData.FPS.LastCheck =	Sys::GetTicks();
	}

	mFrameData.FPS.Count++;
}

void cWindow::LimitFps() {
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

void cWindow::ViewCheckUpdate() {
	if ( mCurrentView->NeedUpdate() ) {
		SetView( *mCurrentView );
	}
}

void cWindow::Clear() {
	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void cWindow::Display( bool clear ) {
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

void cWindow::ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Scissor( x, GetHeight() - ( y + Height ), Width, Height );
	GLi->Enable( GL_SCISSOR_TEST );
}

void cWindow::ClipDisable() {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Disable( GL_SCISSOR_TEST );
}

void cWindow::ClipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneEnable( x, y, Width, Height );
}

void cWindow::ClipPlaneDisable() {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneDisable();
}

cClipboard * cWindow::GetClipboard() const {
	return mClipboard;
}

cInput * cWindow::GetInput() const {
	return mInput;
}

cCursorManager * cWindow::GetCursorManager() const {
	return mCursorManager;
}

Uint32 cWindow::PushResizeCallback( const WindowResizeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[ mNumCallBacks ] = cb;
	return mNumCallBacks;
}

void cWindow::PopResizeCallback( const Uint32& CallbackId ) {
	mCallbacks[ CallbackId ] = 0;
	mCallbacks.erase( mCallbacks.find(CallbackId) );
}

void cWindow::SendVideoResizeCb() {
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
		i->second( this );
	}
}

void cWindow::LogSuccessfulInit(const std::string& BackendName , const std::string&ProcessPath ) {
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

void cWindow::LogFailureInit( const std::string& ClassName, const std::string& BackendName ) {
	eePRINTL( "Error on %s::Init. Backend %s failed to start.", ClassName.c_str(), BackendName.c_str() );
}

std::string cWindow::Caption() {
	return mWindow.WindowConfig.Caption;
}

eeWindowContex cWindow::GetContext() const {
#if defined( EE_GLEW_AVAILABLE  ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_MACOSX )
	return mWindow.Context;
#else
	return 0;
#endif
}

void cWindow::GetMainContext() {
#ifdef EE_GLEW_AVAILABLE

#if EE_PLATFORM == EE_PLATFORM_WIN
	mWindow.Context = wglGetCurrentContext();
#elif defined( EE_X11_PLATFORM )
	mWindow.Context = glXGetCurrentContext();
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mWindow.Context = aglGetCurrentContext();
#endif

#endif
}

void cWindow::SetDefaultContext() {
#if defined( EE_GLEW_AVAILABLE ) && ( EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) )
	SetCurrentContext( mWindow.Context );
#endif
}

void cWindow::Minimize() {
	if ( NULL != mPlatform )
		mPlatform->MinimizeWindow();
}

void cWindow::Maximize() {
	if ( NULL != mPlatform )
		mPlatform->MaximizeWindow();
}

bool cWindow::IsMaximized() {
	if ( NULL != mPlatform )
		return mPlatform->IsWindowMaximized();

	return false;
}

void cWindow::Hide() {
	if ( NULL != mPlatform )
		mPlatform->HideWindow();
}

void cWindow::Raise() {
	if ( NULL != mPlatform )
		mPlatform->RaiseWindow();
}

void cWindow::Show() {
	if ( NULL != mPlatform )
		mPlatform->ShowWindow();
}

void cWindow::Position( Int16 Left, Int16 Top ) {
	if ( NULL != mPlatform )
		mPlatform->MoveWindow( Left, Top );
}

eeVector2i cWindow::Position() {
	if ( NULL != mPlatform )
		return mPlatform->Position();

	return eeVector2i();
}

void cWindow::SetCurrentContext( eeWindowContex Context ) {
	if ( NULL != mPlatform )
		mPlatform->SetContext( Context );
}

void cWindow::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );
	mPlatform = eeNew( Platform::cNullImpl, ( this ) );
}

void cWindow::SetCurrent() {
}

void cWindow::Center() {
	if ( Windowed() ) {
		Position( mWindow.DesktopResolution.Width() / 2 - mWindow.WindowConfig.Width / 2, mWindow.DesktopResolution.Height() / 2 - mWindow.WindowConfig.Height / 2 );
	}
}

Platform::cPlatformImpl * cWindow::GetPlatform() const {
	return mPlatform;
}

void cWindow::StartTextInput() {
}

bool cWindow::IsTextInputActive() {
	return false;
}

void cWindow::StopTextInput() {
}

void cWindow::SetTextInputRect( eeRecti& rect ) {
}

bool cWindow::HasScreenKeyboardSupport()
{
	return false;
}

bool cWindow::IsScreenKeyboardShown() {
	return false;
}

bool cWindow::IsThreadedGLContext() {
	return false;
}

void cWindow::SetGLContextThread() {
}

void cWindow::UnsetGLContextThread() {
}

void cWindow::RunMainLoop( void (*func)(), int fps ) {
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
void * cWindow::GetJNIEnv() {
	return NULL;
}

void * cWindow::GetActivity() {
	return NULL;
}

int cWindow::GetExternalStorageState() {
	return 0;
}

std::string cWindow::GetInternalStoragePath() {
	return std::string("");
}

std::string cWindow::GetExternalStoragePath() {
	return std::string("");
}

std::string cWindow::GetApkPath() {
	return std::string("");
}
#endif

}}
