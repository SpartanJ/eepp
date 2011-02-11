#include "cwindowal.hpp"
#include "cclipboardal.hpp"
#include "cinputal.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include "../../../graphics/cglobalbatchrenderer.hpp"
#include "../../../graphics/cshaderprogrammanager.hpp"
#include "../../../graphics/cvertexbuffermanager.hpp"
#include "../../../graphics/cframebuffermanager.hpp"
#include "../../../graphics/ctexturefactory.hpp"

#include <allegro5/allegro_opengl.h>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <allegro5/allegro_windows.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace Al {

cWindowAl::cWindowAl( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardAl, ( this ) ), eeNew( cInputAl, ( this ) ) ),
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

		GetMainContext();

		CreateView();

		Setup2D();

		mWindow.Created = true;

		LogSuccessfulInit( "Allegro 5" );

		/// Init the clipboard after the window creation
		reinterpret_cast<cClipboardAl*> ( mClipboard )->Init();

		/// Init the input after the window creation
		reinterpret_cast<cInputAl*> ( mInput )->Init();

		return true;
	}
	
	LogFailureInit( "cWindowAl", "Allegro 5" );

	return false;
}

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

bool cWindowAl::Icon( const std::string& Path ) {
	return false;
}

void cWindowAl::Minimize() {
}

void cWindowAl::Maximize() {
}

void cWindowAl::Hide() {
}

void cWindowAl::Raise() {
}

void cWindowAl::Show() {
}

void cWindowAl::Position( Int16 Left, Int16 Top ) {
	al_set_window_position( mDisplay, Left, Top );
}

bool cWindowAl::Active() {
	return mActive;
}

bool cWindowAl::Visible() {
	return true;
}

eeVector2i cWindowAl::Position() {
	eeVector2i Pos;

	al_get_window_position( mDisplay, &Pos.x, &Pos.y );

	return Pos;
}

void cWindowAl::Size( const Uint32& Width, const Uint32& Height ) {
	if ( Windowed() ) {
		Size( Width, Height, true );
	}
}

void cWindowAl::Size( const Uint16& Width, const Uint16& Height, const bool& Windowed ) {
	cLog::instance()->Writef( "Switching from %s to %s. Width: %d Height %d.", this->Windowed() ? "windowed" : "fullscreen", Windowed ? "windowed" : "fullscreen", Width, Height );

	if ( !Width || !Height )
		return;

	try {
		bool Reload = this->Windowed() != Windowed;

		if ( !Reload ) {
			SetFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

			mWindow.WindowConfig.Width    = Width;
			mWindow.WindowConfig.Height   = Height;

			if ( Windowed ) {
				mWindow.WindowSize = eeSize( Width, Height );
			}

			al_toggle_display_flag( mDisplay, ALLEGRO_FULLSCREEN, Windowed ? 0 : 1 );

			al_resize_display( mDisplay, Width, Height );

			mDefaultView.SetView( 0, 0, Width, Height );

			Setup2D();

			SendVideoResizeCb();
		} else {
			SetFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

			Uint32 oldWidth = mWindow.WindowConfig.Width;
			Uint32 oldHeight = mWindow.WindowConfig.Height;

			mWindow.WindowConfig.Width    = Width;
			mWindow.WindowConfig.Height   = Height;

			if ( Windowed ) {
				mWindow.WindowSize = eeSize( Width, Height );
			}

			Graphics::cTextureFactory::instance()->GrabTextures();

			Create( mWindow.WindowConfig, mWindow.ContextConfig );

			if ( !Windowed ) {
				mWindow.WindowSize = eeSize( oldWidth, oldHeight );
			}

			Graphics::cTextureFactory::instance()->UngrabTextures();		// Reload all textures
			Graphics::cShaderProgramManager::instance()->Reload();			// Reload all shaders
			Graphics::Private::cFrameBufferManager::instance()->Reload(); 	// Reload all frame buffers
			Graphics::Private::cVertexBufferManager::instance()->Reload(); 	// Reload all vertex buffers
			GetMainContext();												// Recover the context

			mDefaultView.SetView( 0, 0, Width, Height );

			Setup2D();

			SendVideoResizeCb();
		}
	} catch (...) {
		cLog::instance()->Write( "Unable to change resolution" );
		cLog::instance()->Save();
		mWindow.Created = false;
	}
}

void cWindowAl::ShowCursor( const bool& showcursor ) {
	if ( showcursor )
		al_show_mouse_cursor( mDisplay );
	else
		al_hide_mouse_cursor( mDisplay );
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

void cWindowAl::SetCurrentContext( eeWindowContex Context ) {
}

eeWindowHandler	cWindowAl::GetWindowHandler() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
	al_get_win_window_handle( mDisplay );
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

}}}}

#endif
