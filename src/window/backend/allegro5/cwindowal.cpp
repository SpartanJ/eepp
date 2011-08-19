#include "cwindowal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include "../../../graphics/cglobalbatchrenderer.hpp"
#include "../../../graphics/cshaderprogrammanager.hpp"
#include "../../../graphics/cvertexbuffermanager.hpp"
#include "../../../graphics/cframebuffermanager.hpp"
#include "../../../graphics/ctexturefactory.hpp"

#include "../../platform/platformimpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <allegro5/allegro_windows.h>
#define WGL_NV_video_out
#elif defined( EE_X11_PLATFORM )

#include <allegro5/platform/aintuthr.h>
#include <allegro5/internal/aintern_system.h>

struct ALLEGRO_SYSTEM_XGLX
{
   ALLEGRO_SYSTEM system;
   Display *x11display;
   Display *gfxdisplay;
   Atom AllegroAtom;
   #ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   int xfvm_available;
   int xfvm_screen_count;
   struct {
	  int mode_count;
	  void **modes;
	  void *original_mode;
   } *xfvm_screen;
   #endif
   _AL_THREAD thread;
   _AL_MUTEX lock;
   _AL_COND resized;
   ALLEGRO_DISPLAY *mouse_grab_display;
   int toggle_mouse_grab_keycode;
   unsigned int toggle_mouse_grab_modifiers;
   bool inhibit_screensaver;

   bool mmon_interface_inited;
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   int xinerama_available;
   int xinerama_screen_count;
   void *xinerama_screen_info;
#endif
#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   int xrandr_available;
   int xrandr_event_base;

   _AL_VECTOR xrandr_screens;
   _AL_VECTOR xrandr_adaptermap;
#endif
   uint32_t adapter_use_count;
   int adapter_map[32];
};

struct ALLEGRO_DISPLAY_XGLX
{
   ALLEGRO_DISPLAY display;
   Window window;
   int xscreen;
   int adapter;
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
   XVisualInfo *xvinfo;
   GLXFBConfig *fbc;
   int glx_version;
   _AL_COND mapped;
   bool is_mapped;
   int resize_count;
   bool programmatic_resize;
   X11Cursor invisible_cursor;
   X11Cursor current_cursor;
   bool cursor_hidden;
   Pixmap icon, icon_mask;
   int x, y;
   bool mouse_warp;
};
static _AL_MUTEX * al_display_mutex = NULL;

static void	al_display_lock() {
	if ( NULL != al_display_mutex ) {
		_al_mutex_lock( al_display_mutex );
	}
}

static void al_display_unlock() {
	if ( NULL != al_display_mutex ) {
		_al_mutex_unlock( al_display_mutex );
	}
}

#endif

#include <allegro5/allegro_opengl.h>
#include <allegro5/internal/aintern_system.h>
#include <allegro5/internal/aintern_thread.h>

#include "cclipboardal.hpp"
#include "cinputal.hpp"
#include "ccursormanageral.hpp"

namespace EE { namespace Window { namespace Backend { namespace Al {

cWindowAl::cWindowAl( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardAl, ( this ) ), eeNew( cInputAl, ( this ) ), eeNew( cCursorManagerAl, ( this ) ) ),
	mDisplay( NULL ),
	mActive( true )
{
	Create( Settings, Context );
}

cWindowAl::~cWindowAl() {
	DestroyDisplay();
}

void cWindowAl::DestroyDisplay() {
	if ( NULL != mDisplay ) {
		al_destroy_display( mDisplay );
		mDisplay = NULL;
	}
}

Uint32 cWindowAl::CreateFlags( const WindowSettings& Settings, const ContextSettings& Context ) {
	Uint32 Flags = ALLEGRO_OPENGL | ALLEGRO_GENERATE_EXPOSE_EVENTS;

	if ( GLv_3 == Context.Version ) {
		//Flags |= ALLEGRO_OPENGL_3_0;					/// Not needed if the forward compatible isn't working
		//Flags |= ALLEGRO_OPENGL_FORWARD_COMPATIBLE;	/// Not working yet
	}

	if ( Settings.Style & WindowStyle::Fullscreen ) {
		Flags |= ALLEGRO_FULLSCREEN;
	} else {
		Flags |= ALLEGRO_WINDOWED;

		if ( Settings.Style & WindowStyle::Resize )
			Flags |= ALLEGRO_RESIZABLE;

		if ( Settings.Style & WindowStyle::NoBorder )
			Flags |= ALLEGRO_NOFRAME;
	}

	return Flags;
}

bool cWindowAl::Create( WindowSettings Settings, ContextSettings Context ) {
	try {
		DestroyDisplay();

		mWindow.WindowConfig	= Settings;
		mWindow.ContextConfig	= Context;

		al_set_new_display_flags( CreateFlags( Settings, Context ) );
		al_set_new_display_option( ALLEGRO_STENCIL_SIZE	, Context.StencilBufferSize	, ALLEGRO_SUGGEST );
		al_set_new_display_option( ALLEGRO_DEPTH_SIZE	, Context.DepthBufferSize	, ALLEGRO_SUGGEST );

		if ( Context.VSync )
			al_set_new_display_option( ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST );

		ALLEGRO_MONITOR_INFO minfo;

		if ( al_get_monitor_info( 0, &minfo ) ) {
			mWindow.DesktopResolution = eeSize( minfo.x2, minfo.y2 );

			if ( mWindow.WindowConfig.Style & WindowStyle::UseDesktopResolution ) {
				mWindow.WindowConfig.Width	= mWindow.DesktopResolution.Width();
				mWindow.WindowConfig.Height	= mWindow.DesktopResolution.Height();
			}
		} else {
			mWindow.DesktopResolution = eeSize( Settings.Width, Settings.Height );
		}

		mDisplay = al_create_display( Settings.Width, Settings.Height );

		if ( NULL != mDisplay ) {
			al_inhibit_screensaver( true );

			Caption( mWindow.WindowConfig.Caption );

			if ( NULL == cGL::ExistsSingleton() ) {
				cGL::CreateSingleton( mWindow.ContextConfig.Version );
			}

			cGL::instance()->Init();

			CreatePlatform();

			GetMainContext();

			CreateView();

			Setup2D();

			mWindow.Created = true;

			if ( "" != mWindow.WindowConfig.Icon ) {
				Icon( mWindow.WindowConfig.Icon );
			}

			LogSuccessfulInit( "Allegro 5" );

			/// Init the clipboard after the window creation
			reinterpret_cast<cClipboardAl*> ( mClipboard )->Init();

			/// Init the input after the window creation
			reinterpret_cast<cInputAl*> ( mInput )->Init();

			return true;
		}
	} catch (...) {
		LogFailureInit( "cWindowAl", "Allegro 5" );
	}

	return false;
}

void cWindowAl::SetCurrent() {
#if defined( EE_X11_PLATFORM )
	al_display_mutex = &((ALLEGRO_SYSTEM_XGLX *)al_get_system_driver())->lock;
#endif
}

void cWindowAl::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );

#if defined( EE_X11_PLATFORM )
	SetCurrent();
	mPlatform = eeNew( Platform::cX11Impl, ( this, GetWindowHandler(), GetX11Window(), GetX11Window(), al_display_lock, al_display_unlock ) );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::cWinImpl, ( this, GetWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::cOSXImpl, ( this ) );
#else
	cWindow::CreatePlatform();
#endif
}

#if defined( EE_X11_PLATFORM )
X11Window cWindowAl::GetX11Window() {
	return ((ALLEGRO_DISPLAY_XGLX *)mDisplay)->window;
}
#endif

void cWindowAl::ToggleFullscreen() {
	bool WasMaximized = mWindow.Maximized;

	if ( Windowed() ) {
		Size( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, !Windowed() );
	} else {
		Size( mWindow.WindowSize.Width(), mWindow.WindowSize.Height(), !Windowed() );
	}

	if ( WasMaximized ) {
		Maximize();
	}
}

void cWindowAl::Caption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	al_set_window_title( mDisplay, Caption.c_str() );
}

bool cWindowAl::Active() {
	return mActive;
}

bool cWindowAl::Visible() {
	return !( 0 != ( al_get_display_flags( mDisplay ) & ALLEGRO_MINIMIZED ) );
}

eeVector2i cWindowAl::Position() {
	eeVector2i Pos;
	al_get_window_position( mDisplay, &Pos.x, &Pos.y );
	return Pos;
}

void cWindowAl::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.Width();
		Height	= mWindow.DesktopResolution.Height();
	}

	if ( this->Windowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	cLog::instance()->Writef( "Switching from %s to %s. Width: %d Height %d.", this->Windowed() ? "windowed" : "fullscreen", Windowed ? "windowed" : "fullscreen", Width, Height );

	try {
		bool Reload				= this->Windowed() != Windowed;
		bool WinFullscreen		= eeSize( Width, Height ) == mWindow.DesktopResolution && !Windowed;
		bool WinIsFullscreen	= eeSize( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) == mWindow.DesktopResolution && !this->Windowed();

		Uint32 oldWidth		= mWindow.WindowConfig.Width;
		Uint32 oldHeight	= mWindow.WindowConfig.Height;

		SetFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

		mWindow.WindowConfig.Width    = Width;
		mWindow.WindowConfig.Height   = Height;

		if ( Windowed ) {
			mWindow.WindowSize = eeSize( Width, Height );
		} else {
			mWindow.WindowSize = eeSize( oldWidth, oldHeight );
		}

		if ( !Reload || WinFullscreen || WinIsFullscreen ) {
			if ( !WinFullscreen ) {
				al_toggle_display_flag( mDisplay, ALLEGRO_FULLSCREEN_WINDOW, 0 );
				al_toggle_display_flag( mDisplay, ALLEGRO_FULLSCREEN, Windowed ? 0 : 1 );
			} else {
				al_toggle_display_flag( mDisplay, ALLEGRO_FULLSCREEN_WINDOW, 1 );
			}

			al_resize_display( mDisplay, Width, Height );
		} else {
			Graphics::cTextureFactory::instance()->GrabTextures();

			Create( mWindow.WindowConfig, mWindow.ContextConfig );

			Graphics::cTextureFactory::instance()->UngrabTextures();		// Reload all textures
			Graphics::cShaderProgramManager::instance()->Reload();			// Reload all shaders
			Graphics::Private::cFrameBufferManager::instance()->Reload(); 	// Reload all frame buffers
			Graphics::Private::cVertexBufferManager::instance()->Reload(); 	// Reload all vertex buffers
			GetMainContext();												// Recover the context
		}

		mDefaultView.SetView( 0, 0, Width, Height );

		Setup2D();

		SendVideoResizeCb();

		GetCursorManager()->Reload();
	} catch (...) {
		cLog::instance()->Write( "Unable to change resolution" );
		cLog::instance()->Save();
		mWindow.Created = false;
	}
}

void cWindowAl::SwapBuffers() {
	al_flip_display();
}

std::vector< std::pair<unsigned int, unsigned int> > cWindowAl::GetPossibleResolutions() const {
	std::vector< std::pair<unsigned int, unsigned int> > modes;

	Int32 Count = al_get_num_display_modes();

	ALLEGRO_DISPLAY_MODE dm;

	for ( Int32 i = 0; i < Count; i++ ) {
		al_get_display_mode( i, &dm );

		std::pair<unsigned int, unsigned int> res( dm.width, dm.height );

		bool exists = false;

		for ( Uint32 z = 0; z < modes.size(); z++ ) {
			if ( res == modes[i] ) {
				exists = true;
				break;
			}
		}

		if ( !exists )
			modes.push_back( res );
	}

	return modes;
}

void cWindowAl::SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue ) {
}

eeWindowHandler	cWindowAl::GetWindowHandler() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	return al_get_win_window_handle( mDisplay );
	#elif defined( EE_X11_PLATFORM )
	ALLEGRO_SYSTEM_XGLX * glx = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
	return glx->x11display;
	#else
	return 0;
	#endif
}

void cWindowAl::SetDefaultContext() {
	al_set_current_opengl_context( mDisplay );
}

ALLEGRO_DISPLAY * cWindowAl::GetDisplay() const {
	return mDisplay;
}

bool cWindowAl::Icon( const std::string& Path ) {
	int x, y, c;

	if ( !mWindow.Created ) {
		if ( stbi_info( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon 	= Path;

			return true;
		}

		return false;
	}

	cImage Img( Path );

	if ( NULL != Img.GetPixelsPtr() ) {
		const Uint8 * Ptr = Img.GetPixelsPtr();
		Int32 W = Img.Width();
		Int32 H = Img.Height();
		c = Img.Channels();

		if ( ( W  % 8 ) == 0 && ( H % 8 ) == 0 ) {
			int nbfl = al_get_new_bitmap_flags();

			al_set_new_bitmap_flags( ALLEGRO_MEMORY_BITMAP );

			ALLEGRO_BITMAP * icon;

			icon = al_create_bitmap( W, H );

			al_set_target_bitmap( icon );

			if ( icon && al_lock_bitmap( icon, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY ) ) {
				eeUint Pos;

				for ( y = 0; y < H; y++ ) {
					for ( x = 0; x < W; x++ ) {
						Pos = ( x + y * W ) * c;

						if ( 4 == c )
							al_put_pixel( x, y, al_map_rgba( Ptr[Pos], Ptr[Pos+1], Ptr[Pos+2], Ptr[Pos+3] ) );
						else
							al_put_pixel( x, y, al_map_rgb( Ptr[Pos], Ptr[Pos+1], Ptr[Pos+2] ) );
					}
				}

				al_unlock_bitmap( icon );

				al_set_display_icon( mDisplay, icon );
			}

			if ( icon )
				al_destroy_bitmap( icon );

			al_set_new_bitmap_flags( nbfl );
			al_set_target_backbuffer( mDisplay );

			mWindow.WindowConfig.Icon 	= Path;

			return true;
		}
	}

	return false;
}

}}}}

#endif
