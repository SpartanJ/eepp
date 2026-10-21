#include <eepp/window/backend/SDL3/base.hpp>

#ifdef EE_BACKEND_SDL3

#include <cstring>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/shaderprogramregistry.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/window/backend/SDL3/clipboardsdl3.hpp>
#include <eepp/window/backend/SDL3/cursormanagersdl3.hpp>
#include <eepp/window/backend/SDL3/displaymanagersdl3.hpp>
#include <eepp/window/backend/SDL3/dragpreviewsdl3.hpp>
#include <eepp/window/backend/SDL3/inputsdl3.hpp>
#include <eepp/window/backend/SDL3/windowsdl3.hpp>
#include <eepp/window/backend/SDL3/wminfo.hpp>
#include <eepp/window/backend/backendhelper.hpp>
#include <eepp/window/draganddrop/dragmanager.hpp>
#include <eepp/window/draganddrop/waylanddraganddrop.hpp>
#include <eepp/window/draganddrop/x11draganddrop.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/runtime.hpp>
#include <eepp/window/terminal/terminalruntime.hpp>

#ifdef EE_HAS_X11_DRAG_SOURCE
#include <SDL3/SDL_system.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/window/backend/SDL3/displaymanagersdl3.hpp>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS ||       \
	EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#define SDL3_THREADED_GLCONTEXT
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

NativeWindowContext WindowSDL::getNativeDragContext() {
	auto context = Window::getNativeDragContext();
	if ( !canUseNativeDrag() || !mSDLWindow )
		return context;
	const char* driver = SDL_GetCurrentVideoDriver();
	if ( !driver )
		return context;
	auto props = SDL_GetWindowProperties( mSDLWindow );
	if ( std::strcmp( driver, "x11" ) == 0 ) {
		context.handles.x11 = {
			SDL_GetPointerProperty( props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr ),
			static_cast<unsigned long>(
				SDL_GetNumberProperty( props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0 ) ) };
		if ( context.handles.x11.display && context.handles.x11.window )
			context.kind = NativeWindowContext::Kind::X11;
	} else if ( std::strcmp( driver, "wayland" ) == 0 ) {
		context.handles.wayland = {
			SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr ),
			SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr ) };
		if ( context.handles.wayland.display && context.handles.wayland.surface )
			context.kind = NativeWindowContext::Kind::Wayland;
	} else if ( std::strcmp( driver, "windows" ) == 0 ) {
		context.handles.window =
			SDL_GetPointerProperty( props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr );
		if ( context.handles.window )
			context.kind = NativeWindowContext::Kind::Win32;
	} else if ( std::strcmp( driver, "cocoa" ) == 0 ) {
		context.handles.window =
			SDL_GetPointerProperty( props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr );
		if ( context.handles.window )
			context.kind = NativeWindowContext::Kind::Cocoa;
	}
	return context;
}

bool WindowSDL::isNativeFileTargetSupported() const {
	if ( !canUseNativeDrag() || !mSDLWindow )
		return false;
	const char* driver = SDL_GetCurrentVideoDriver();
	return driver && ( std::strcmp( driver, "x11" ) == 0 || std::strcmp( driver, "wayland" ) == 0 ||
					   std::strcmp( driver, "windows" ) == 0 ||
					   std::strcmp( driver, "cocoa" ) == 0 || std::strcmp( driver, "haiku" ) == 0 );
}

bool WindowSDL::isNativeDataTargetSupported() const {
#ifdef EE_HAS_X11_DRAG_SOURCE
	// The X11 event hook lets eepp answer the drags SDL's own XDND support cannot read.
	const char* driver = SDL_GetCurrentVideoDriver();
	return canUseNativeDrag() && mSDLWindow && driver && std::strcmp( driver, "x11" ) == 0;
#else
	return false;
#endif
}

namespace {
#ifdef EE_HAS_X11_DRAG_SOURCE
// Process-global SDL state, enabled while a drag session or a data drop handler needs it.
bool nativeDragEventsEnabled = false;
bool SDLCALL nativeDragX11Hook( void*, XEvent* event ) {
	// Only our own protocol messages are consumed: those of our drag source and of the drags our
	// data drop target accepted. SDL retains input, clipboard and every other incoming XDND; in
	// particular it must not send a second SelectionNotify for our offer.
	return !Private::DragManager::handleEvent( event );
}
#endif
} // namespace

void WindowSDL::initializeNativeDragEvents() {
#ifdef EE_HAS_X11_DRAG_SOURCE
	const char* driver = SDL_GetCurrentVideoDriver();
	if ( nativeDragEventsEnabled || !canUseNativeDrag() || !driver ||
		 std::strcmp( driver, "x11" ) != 0 )
		return;
	nativeDragEventsEnabled = true;
	SDL_SetX11EventHook( nativeDragX11Hook, nullptr );
#endif
}

void WindowSDL::shutdownNativeDragEvents() {
#ifdef EE_HAS_X11_DRAG_SOURCE
	if ( !nativeDragEventsEnabled )
		return;
	nativeDragEventsEnabled = false;
	SDL_SetX11EventHook( nullptr, nullptr );
#endif
}

Private::NativeDragPreview* WindowSDL::createNativeDragPreview( const DragOffer& offer,
																bool manageCursor ) {
	return DragPreviewSDL::create( this, mSDLWindow, offer, manageCursor );
}

WindowSDL::WindowSDL( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardSDL, ( this ) ), eeNew( InputSDL, ( this ) ),
			eeNew( CursorManagerSDL, ( this ) ) ) {
	create( Settings, Context );
}

WindowSDL::~WindowSDL() {
	shutdownRuntimeRenderTarget();
	destroySDLResources();
}

void WindowSDL::destroySDLResources() {
	Private::DragManager::invalidateWindow( this );
#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
	Private::WaylandDragAndDrop::detachWindow( *this );
#endif
	if ( nullptr != mGLContext ) {
		SDL_GL_DestroyContext( mGLContext );
		mGLContext = nullptr;
	}

	if ( nullptr != mGLContextThread ) {
		SDL_GL_DestroyContext( mGLContextThread );
		mGLContextThread = nullptr;
	}

	eeSAFE_DELETE( mWMinfo );

	if ( mSDLWindowThread ) {
		SDL_DestroyWindow( mSDLWindowThread );
		mSDLWindowThread = nullptr;
	}

	if ( nullptr != mSDLWindow ) {
		SDL_DestroyWindow( mSDLWindow );
		mSDLWindow = nullptr;
	}

#if defined( EE_X11_PLATFORM )
	Backend::BackendHelper::hideOSK();
#endif
}

bool WindowSDL::create( WindowSettings Settings, ContextSettings Context ) {
	if ( mWindow.Created )
		return false;

	mWindow.WindowConfig = Settings;
	mWindow.ContextConfig = Context;

#if defined( EE_X11_PLATFORM )
	// Unmapped GLX windows do not provide reliable front-buffer storage. Keep hidden windows
	// double-buffered so rendering and readback use the drawable's back buffer.
	if ( mWindow.WindowConfig.Style & WindowStyle::Hidden )
		mWindow.ContextConfig.DoubleBuffering = true;
#endif

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) && !SDL_Init( SDL_INIT_VIDEO ) ) {
		Log::error( "Unable to initialize SDL: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode* dpm = SDL_GetDesktopDisplayMode( primaryDisplay );

	if ( dpm ) {
		mWindow.DesktopResolution = Sizei( dpm->w, dpm->h );
	} else {
		Log::error( "Failed to get desktop display mode" );
		mWindow.DesktopResolution = Sizei( 800, 600 );
	}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	mWindow.WindowConfig.Style = WindowStyle::Fullscreen | WindowStyle::UseDesktopResolution;
#endif

	if ( mWindow.WindowConfig.Style & WindowStyle::UseDesktopResolution ) {
		mWindow.WindowConfig.Width = mWindow.DesktopResolution.getWidth();
		mWindow.WindowConfig.Height = mWindow.DesktopResolution.getHeight();
	}

	mWindow.Flags =
		SDL_WINDOW_OPENGL |
		( ( mWindow.WindowConfig.Style & WindowStyle::Hidden ) ? SDL_WINDOW_HIDDEN : 0 ) |
		( ( !mWindow.WindowConfig.DisableHiDPI ? SDL_WINDOW_HIGH_PIXEL_DENSITY : 0 ) );

	if ( mWindow.WindowConfig.Style & WindowStyle::Resize ) {
		mWindow.Flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( mWindow.WindowConfig.Style & WindowStyle::Borderless ) {
		mWindow.Flags |= SDL_WINDOW_BORDERLESS;
	}

	// SDL's attributes survive window creation; every window must select its own configuration.
	SDL_GL_ResetAttributes();
	setGLConfig();

	Uint32 tmpFlags = mWindow.Flags;

	if ( mWindow.WindowConfig.Style & WindowStyle::Fullscreen ) {
		tmpFlags |= SDL_WINDOW_FULLSCREEN;
	}

	if ( mWindow.ContextConfig.Multisamples > 0 ) {
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, mWindow.ContextConfig.Multisamples );
	}

#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_IOS && \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( mWindow.WindowConfig.PixelDensity > 0 ) {
		mWindow.WindowConfig.Width *= mWindow.WindowConfig.PixelDensity;
		mWindow.WindowConfig.Height *= mWindow.WindowConfig.PixelDensity;
	}
#endif

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS
	Log::notice( "Choosing GL Version from: %d", Context.Version );

	if ( GLv_default != Context.Version ) {
		if ( GLv_ES1 == Context.Version || GLv_2 == Context.Version ) {
			if ( GLv_2 == Context.Version )
				mWindow.ContextConfig.Version = GLv_ES1;

			Log::notice( "Starting GLES1" );

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
		} else {
			Log::notice( "Starting GLES2" );

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
		}
	} else {
#if defined( EE_GLES2 )
		Log::notice( "Starting GLES2 default" );

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
#else
		Log::notice( "Starting GLES1 default" );

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
#endif
	}
#else
	if ( GLv_3CP == Context.Version ) {
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	}
#endif

	mSDLWindow = SDL_CreateWindow( mWindow.WindowConfig.Title.c_str(), mWindow.WindowConfig.Width,
								   mWindow.WindowConfig.Height, tmpFlags );

	if ( nullptr == mSDLWindow ) {
		Log::error( "Unable to create window: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	// Sharing window contexts is independent from supporting an auxiliary context on a worker
	// thread. Platforms without threaded GL contexts still need shared object namespaces for
	// resources used by multiple native windows.
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT,
						 mWindow.ContextConfig.SharedGLContext ? 1 : 0 );
#ifdef SDL3_THREADED_GLCONTEXT
	if ( mWindow.ContextConfig.SharedGLContext ) {
		if ( std::strcmp( SDL_GetCurrentVideoDriver(), "offscreen" ) == 0 ) {
			// EGL cannot bind one surface on two threads at once. The worker needs its own
			// drawable while sharing GL objects with the main window's context.
			mSDLWindowThread = SDL_CreateWindow( "eepp worker context", 1, 1,
												 SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
			if ( !mSDLWindowThread ) {
				Log::error( "Unable to create worker GL surface: %s", SDL_GetError() );
				return false;
			}
		}
		mGLContextThread = SDL_GL_CreateContext( mSDLWindowThread ? mSDLWindowThread : mSDLWindow );
	}
#endif
	mGLContext = SDL_GL_CreateContext( mSDLWindow );

	if ( nullptr == mGLContext
#ifdef SDL3_THREADED_GLCONTEXT
		 || ( mWindow.ContextConfig.SharedGLContext && nullptr == mGLContextThread )
#endif
	) {
		Log::error( "Unable to create context: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	int w, h;
	SDL_GetWindowSizeInPixels( mSDLWindow, &w, &h );

	if ( w > 0 && h > 0 ) {
		mWindow.WindowConfig.Width = w;
		mWindow.WindowConfig.Height = h;
		mWindow.WindowSize = Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
		mLastWindowedSize = mWindow.WindowSize;
	} else {
		Log::error( "Window failed to create!" );

		if ( nullptr != SDL_GetError() && SDL_GetError()[0] != '\0' ) {
			Log::error( "SDL Error: %s", SDL_GetError() );
		}

		return false;
	}

	SDL_GL_SetSwapInterval( ( mWindow.ContextConfig.VSync ? 1 : 0 ) );

	SDL_GL_MakeCurrent( mSDLWindow, mGLContext );

	mID = SDL_GetWindowID( mSDLWindow );

	mWMinfo = eeNew( WMInfo, ( mSDLWindow ) );

	if ( nullptr == Renderer::existsSingleton() ) {
		Renderer::createSingleton( mWindow.ContextConfig.Version );
		Renderer::instance()->init();
		if ( mWindow.ContextConfig.Multisamples > 0 )
			Renderer::instance()->multisample( true );
	}

	getMainContext();

	setTitle( mWindow.WindowConfig.Title );

	createView();

	setup2D( false );

	if ( !initializeRuntimeRenderTarget() )
		return false;

	mWindow.Created = true;

	if ( "" != mWindow.WindowConfig.Icon ) {
		setIcon( mWindow.WindowConfig.Icon );
	}

	static_cast<ClipboardSDL*>( mClipboard )->init();

	static_cast<InputSDL*>( mInput )->init();

	mCursorManager->set( Cursor::SysArrow );

#if EE_PLATFORM == EE_PLATFORM_WIN
	Backend::BackendHelper::setUserTheme( (HWND)getWindowHandler() );
#endif

	if ( mWindow.WindowConfig.Style & WindowStyle::Hidden )
		SDL_HideWindow( mSDLWindow );

#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
	Private::WaylandDragAndDrop::attachWindow( getNativeDragContext() );
#endif

	logSuccessfulInit( getVersion() );

	return true;
}

Uint32 WindowSDL::getWindowID() const {
	return mID;
}

void WindowSDL::makeCurrent() {
	SDL_GL_MakeCurrent( mSDLWindow, mGLContext );
}

void WindowSDL::close() {
	if ( !getDeferNativeResourceDestructionOnClose() )
		destroySDLResources();
	Window::close();
}

void WindowSDL::setCurrent() {
	makeCurrent();
}

bool WindowSDL::isThreadedGLContext() const {
#ifdef SDL3_THREADED_GLCONTEXT
	return mWindow.ContextConfig.SharedGLContext;
#else
	return false;
#endif
}

void WindowSDL::setGLContextThread() {
	mGLContextMutex.lock();
	if ( !SDL_GL_MakeCurrent( mSDLWindowThread ? mSDLWindowThread : mSDLWindow, mGLContextThread ) )
		Log::error( "Unable to bind worker GL context: %s", SDL_GetError() );
}

void WindowSDL::unsetGLContextThread() {
	if ( !SDL_GL_MakeCurrent( nullptr, nullptr ) )
		Log::error( "Unable to release worker GL context: %s", SDL_GetError() );
#if SDL_VERSION_ATLEAST( 3, 2, 0 )
	SDL_CleanupTLS();
#endif
	mGLContextMutex.unlock();
}

int WindowSDL::getCurrentDisplayIndex() const {
	if ( !mSDLWindow )
		return 0;
	SDL_DisplayID displayID = SDL_GetDisplayForWindow( mSDLWindow );
	// Map display ID to index using the display manager
	auto* dispMgr = static_cast<DisplayManagerSDL3*>( Engine::instance()->getDisplayManager() );
	int idx = dispMgr->getDisplayIndexFromID( displayID );
	return idx >= 0 ? idx : 0;
}

std::string WindowSDL::getVersion() {
	int ver = SDL_GetVersion();
	return String::format( "SDL %d.%d.%d", SDL_VERSIONNUM_MAJOR( ver ), SDL_VERSIONNUM_MINOR( ver ),
						   SDL_VERSIONNUM_MICRO( ver ) );
}

void WindowSDL::setGLConfig() {
	if ( mWindow.ContextConfig.DepthBufferSize )
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, mWindow.ContextConfig.DepthBufferSize );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, ( mWindow.ContextConfig.DoubleBuffering ? 1 : 0 ) );
	if ( mWindow.ContextConfig.StencilBufferSize )
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, mWindow.ContextConfig.StencilBufferSize );

	if ( mWindow.WindowConfig.BitsPerPixel == 16 ) {
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 4 );
	} else {
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	}
}

void WindowSDL::toggleFullscreen() {
	if ( Runtime::forceHiddenWindows() )
		return;
	Log::info( "toggleFullscreen called: %s", isWindowed() ? "is windowed" : "is fullscreen" );

	if ( isWindowed() ) {
		mWinPos = getPosition();
		mWindow.Maximized = isMaximized();
	}

	SDL_SetWindowFullscreen( mSDLWindow, !isWindowed() ? false : true );

	BitOp::setBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen,
							isWindowed() ? 1 : 0 );

	getCursorManager()->reload();

	if ( isWindowed() ) {
		setPosition( mWinPos.x, mWinPos.y );

		if ( mWindow.Maximized )
			maximize();
	}
}

void WindowSDL::setTitle( const std::string& title ) {
	if ( mWindow.WindowConfig.Title != title ) {
		mWindow.WindowConfig.Title = title;

		SDL_SetWindowTitle( mSDLWindow, title.c_str() );
	}
}

bool WindowSDL::isActive() const {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return TerminalRuntime::instance().isFocused();
	Uint64 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( ( flags & SDL_WINDOW_INPUT_FOCUS ) && ( flags & SDL_WINDOW_MOUSE_FOCUS ) );
}

bool WindowSDL::isVisible() const {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return true;
	Uint64 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( !( flags & SDL_WINDOW_HIDDEN ) && !( flags & SDL_WINDOW_MINIMIZED ) );
}

bool WindowSDL::hasFocus() const {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return TerminalRuntime::instance().isFocused();
	Uint64 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( flags & ( SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS ) );
}

bool WindowSDL::hasInputFocus() const {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return TerminalRuntime::instance().isFocused();
	Uint64 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( flags & SDL_WINDOW_INPUT_FOCUS );
}

bool WindowSDL::hasMouseFocus() const {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		return TerminalRuntime::instance().isFocused();
	Uint64 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( flags & SDL_WINDOW_MOUSE_FOCUS );
}

void WindowSDL::onWindowResize( Uint32 width, Uint32 height ) {
	if ( width == mWindow.WindowConfig.Width && height == mWindow.WindowConfig.Height )
		return;

	Log::debug( "onWindowResize: Width %d Height %d.", width, height );

	mWindow.WindowConfig.Width = width;
	mWindow.WindowConfig.Height = height;
	mWindow.WindowSize = Sizei( width, height );

	if ( isWindowed() )
		mLastWindowedSize = Sizei( width, height );

	mDefaultView.reset( Rectf( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) );
	resizeRuntimeRenderTarget( width, height );

	setup2D( false );

	SDL_PumpEvents();

	SDL_FlushEvent( SDL_EVENT_WINDOW_RESIZED );

	mCursorManager->reload();

	sendVideoResizeCb();
}

void WindowSDL::setSize( Uint32 width, Uint32 height, bool windowed ) {
	if ( Runtime::forceHiddenWindows() )
		windowed = true;
	if ( ( !width || !height ) ) {
		width = mWindow.DesktopResolution.getWidth();
		height = mWindow.DesktopResolution.getHeight();
	}

	if ( this->isWindowed() == windowed && width == mWindow.WindowConfig.Width &&
		 height == mWindow.WindowConfig.Height )
		return;

	Log::debug( "Switching from %s to %s. Width: %d Height %d.",
				this->isWindowed() ? "windowed" : "fullscreen",
				windowed ? "windowed" : "fullscreen", width, height );

	Uint32 oldWidth = mWindow.WindowConfig.Width;
	Uint32 oldHeight = mWindow.WindowConfig.Height;

	mWindow.WindowConfig.Width = width;
	mWindow.WindowConfig.Height = height;

	if ( windowed ) {
		mWindow.WindowSize = Sizei( width, height );
	} else {
		mWindow.WindowSize = Sizei( oldWidth, oldHeight );
	}

	if ( isWindowed() && !windowed ) {
		mWinPos = getPosition();
	} else {
		SDL_SetWindowFullscreen( mSDLWindow, !windowed );
	}

	if ( windowed )
		mLastWindowedSize = Sizei( width, height );

	SDL_SetWindowSize( mSDLWindow, width, height );

	if ( isWindowed() && !windowed ) {
		mWinPos = getPosition();

		setGLConfig();

		SDL_SetWindowFullscreen( mSDLWindow, !windowed );
	}

	if ( isWindowed() && windowed ) {
		setPosition( mWinPos.x, mWinPos.y );
	}

	BitOp::setBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !windowed );

	mDefaultView.reset( Rectf( 0, 0, width, height ) );

	setup2D( false );

	SDL_PumpEvents();

	SDL_FlushEvent( SDL_EVENT_WINDOW_RESIZED );

	mCursorManager->reload();

	sendVideoResizeCb();
}

void WindowSDL::setMinimumSize( Uint32 width, Uint32 height ) {
	SDL_SetWindowMinimumSize( mSDLWindow, static_cast<int>( width ), static_cast<int>( height ) );
}

bool WindowSDL::setModalFor( Window* parent ) {
	auto* parentWindow = dynamic_cast<WindowSDL*>( parent );
	if ( nullptr == parent ) {
		if ( !SDL_SetWindowModal( mSDLWindow, false ) )
			return false;
		return SDL_SetWindowParent( mSDLWindow, nullptr );
	}
	if ( nullptr == parentWindow || !SDL_SetWindowParent( mSDLWindow, parentWindow->mSDLWindow ) )
		return false;
	return SDL_SetWindowModal( mSDLWindow, true );
}

void WindowSDL::swapBuffers() {
	SDL_GL_SwapWindow( mSDLWindow );
}

std::vector<DisplayMode> WindowSDL::getDisplayModes() const {
	std::vector<DisplayMode> result;

	int displayCount = 0;
	SDL_DisplayID* displays = SDL_GetDisplays( &displayCount );

	if ( displays ) {
		for ( int x = 0; x < displayCount; x++ ) {
			int modeCount = 0;
			SDL_DisplayMode** modesArray = SDL_GetFullscreenDisplayModes( displays[x], &modeCount );

			if ( modesArray ) {
				for ( int i = 0; i < modeCount; i++ ) {
					SDL_DisplayMode* mode = modesArray[i];
					result.push_back( DisplayMode( mode->w, mode->h, (int)mode->refresh_rate, x ) );
				}
				SDL_free( modesArray );
			}
		}
		SDL_free( displays );
	}

	return result;
}

void WindowSDL::setGamma( Float Red, Float Green, Float Blue ) {
	// SDL3 removed gamma ramp functionality
}

eeWindowHandle WindowSDL::getWindowHandler() const {
	if ( nullptr != mWMinfo ) {
		return mWMinfo->getWindowHandler();
	}
	return 0;
}

bool WindowSDL::setIcon( const std::string& Path ) {
	int x, y, c;

	if ( !mWindow.Created ) {
		if ( Image::getInfo( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon = Path;

			return true;
		}

		return false;
	}

	Image Img( Path );

	if ( nullptr != Img.getPixelsPtr() ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( Img.getWidth() > 64 || Img.getHeight() > 64 ) {
			Img.resize( 64, 64 );
		}
#endif
		const Uint8* Ptr = Img.getPixelsPtr();
		x = Img.getWidth();
		y = Img.getHeight();
		c = Img.getChannels();

		if ( ( x % 8 ) == 0 && ( y % 8 ) == 0 ) {
			SDL_Surface* iconSurface = SDL_CreateSurface( x, y, SDL_PIXELFORMAT_RGBA32 );

			if ( iconSurface ) {
				memcpy( iconSurface->pixels, Ptr, x * y * c );

				SDL_SetWindowIcon( mSDLWindow, iconSurface );

				SDL_DestroySurface( iconSurface );

				return true;
			}
		}
	}

	return false;
}

void WindowSDL::minimize() {
	SDL_MinimizeWindow( mSDLWindow );
}

void WindowSDL::maximize() {
	if ( Runtime::isOffscreen() || Runtime::forceHiddenWindows() )
		return;
	SDL_MaximizeWindow( mSDLWindow );
}

bool WindowSDL::isMaximized() const {
	return SDL_GetWindowFlags( mSDLWindow ) & SDL_WINDOW_MAXIMIZED;
}

bool WindowSDL::isMinimized() const {
	return SDL_GetWindowFlags( mSDLWindow ) & SDL_WINDOW_MINIMIZED;
}

void WindowSDL::hide() {
	SDL_HideWindow( mSDLWindow );
}

void WindowSDL::raise() {
	if ( Runtime::forceHiddenWindows() )
		return;
	SDL_RaiseWindow( mSDLWindow );
}

void WindowSDL::restore() {
	if ( Runtime::forceHiddenWindows() )
		return;
	SDL_RestoreWindow( mSDLWindow );
}

void WindowSDL::flash( WindowFlashOperation op ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	SDL_FlashOperation sdlOp = SDL_FLASH_BRIEFLY;
	if ( op == WindowFlashOperation::Cancel )
		sdlOp = SDL_FLASH_CANCEL;
	else if ( op == WindowFlashOperation::UntilFocused )
		sdlOp = SDL_FLASH_UNTIL_FOCUSED;
	SDL_FlashWindow( mSDLWindow, sdlOp );
#endif
}

void WindowSDL::show() {
	if ( Runtime::forceHiddenWindows() )
		return;
	SDL_ShowWindow( mSDLWindow );
}

void WindowSDL::setPosition( int Left, int Top ) {
	SDL_SetWindowPosition( mSDLWindow, Left, Top );
}

Vector2i WindowSDL::getPosition() const {
	Vector2i p;

	SDL_GetWindowPosition( mSDLWindow, &p.x, &p.y );

	return p;
}

void WindowSDL::updateDesktopResolution() const {
	SDL_DisplayID displayID = SDL_GetDisplayForWindow( mSDLWindow );
	const SDL_DisplayMode* dpm = SDL_GetDesktopDisplayMode( displayID );
	if ( dpm ) {
		mWindow.DesktopResolution = Sizei( dpm->w, dpm->h );
	} else {
		Log::warning( "Failed to get desktop display mode for display %u", displayID );
	}
}

const Sizei& WindowSDL::getDesktopResolution() const {
	updateDesktopResolution();
	return Window::getDesktopResolution();
}

Rect WindowSDL::getBorderSize() const {
	Rect bordersSize;
	SDL_GetWindowBordersSize( mSDLWindow, &bordersSize.Top, &bordersSize.Left, &bordersSize.Bottom,
							  &bordersSize.Right );
	return bordersSize;
}

Float WindowSDL::getScale() const {
	if ( nullptr == mSDLWindow )
		return PixelDensity::getPixelDensity();

	int realX, realY;
	int scaledX, scaledY;
	SDL_GetWindowSizeInPixels( mSDLWindow, &realX, &realY );
	SDL_GetWindowSize( mSDLWindow, &scaledX, &scaledY );
	return (Float)realX / (Float)scaledX;
}

bool WindowSDL::hasNativeMessageBox() const {
	return true;
}

Uint32 toSDLMsgBoxType( const Window::MessageBoxType& type ) {
	switch ( type ) {
		case Window::MessageBoxType::Error:
			return SDL_MESSAGEBOX_ERROR;
		case Window::MessageBoxType::Warning:
			return SDL_MESSAGEBOX_WARNING;
		case Window::MessageBoxType::Information:
			return SDL_MESSAGEBOX_INFORMATION;
	}
	return SDL_MESSAGEBOX_INFORMATION;
}

bool WindowSDL::showMessageBox( const MessageBoxType& type, const std::string& title,
								const std::string& message ) {
	return SDL_ShowSimpleMessageBox( toSDLMsgBoxType( type ), title.c_str(), message.c_str(),
									 mSDLWindow );
}

SDL_Window* WindowSDL::getSDLWindow() const {
	return mSDLWindow;
}

void WindowSDL::startOnScreenKeyboard() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	Backend::BackendHelper::showOSK( getWindowHandler() );
#elif defined( EE_X11_PLATFORM )
	Backend::BackendHelper::showOSK();
#endif
}

void WindowSDL::stopOnScreenKeyboard() {
	Backend::BackendHelper::hideOSK();
}

bool WindowSDL::isOnScreenKeyboardActive() const {
	return Backend::BackendHelper::isOSKActive();
}

void WindowSDL::startTextInput() {
	if ( mWindow.WindowConfig.UseScreenKeyboard ) {
		startOnScreenKeyboard();
	} else {
		SDL_StartTextInput( mSDLWindow );
	}
}

bool WindowSDL::isTextInputActive() const {
	if ( mWindow.WindowConfig.UseScreenKeyboard )
		return isOnScreenKeyboardActive();
	return SDL_TextInputActive( mSDLWindow );
}

void WindowSDL::stopTextInput() {
	if ( mWindow.WindowConfig.UseScreenKeyboard ) {
		stopOnScreenKeyboard();
	} else {
		SDL_StopTextInput( mSDLWindow );
	}
}

void WindowSDL::setTextInputRect( const Rect& rect ) {
	SDL_Rect r;
	r.x = rect.Left;
	r.y = rect.Top;
	r.w = rect.getSize().getWidth();
	r.h = rect.getSize().getHeight();

	SDL_SetTextInputArea( mSDLWindow, &r, 0 );
}

void WindowSDL::clearComposition() {
	SDL_ClearComposition( mSDLWindow );
}

bool WindowSDL::hasScreenKeyboardSupport() const {
	return SDL_HasScreenKeyboardSupport();
}

bool WindowSDL::isScreenKeyboardShown() const {
	return SDL_ScreenKeyboardShown( mSDLWindow );
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
